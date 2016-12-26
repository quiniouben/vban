/*
 *  This file is part of vban_receptor.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
 *
 *  vban_receptor is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  vban_receptor is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vban_receptor.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "audio_backend.h"
#include "logger.h"

#define AUDIO_DEVICE        "default"

struct audio_t
{
    audio_backend_handle_t  backend;
    char const*             output_name;
    enum VBanBitResolution  bit_resolution;
    unsigned int            nb_channels;
    unsigned int            rate;
    size_t                  buffer_size;
    unsigned char const*    channels;
    int                     channels_size;
    char*                   channels_buffer;
};

static int audio_open(audio_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
static int audio_extract_channels(audio_handle_t handle, char const* buffer, size_t size);
static size_t computeSize(unsigned char quality);

size_t computeSize(unsigned char quality)
{
    size_t const nmin = VBAN_PROTOCOL_MAX_SIZE;
    size_t nnn = 0;

    switch(quality)
    {
        case 0:
            nnn = 512;
            break;

        case 1:
            nnn = 1024;
            break;

        case 2:
            nnn = 2048;
            break;

        case 3:
            nnn = 4096;
            break;

        case 4:
            nnn = 8192;
            break;

        default:
            break;
    }

    nnn=nnn*3;

    if (nnn < nmin)
    {
        nnn = nmin;
    }

    return nnn;
}

int audio_init(audio_handle_t* handle, char const* backend_type, char const* output_name, unsigned char quality)
{
    int ret = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "audio_init: null handle pointer");
        return -EINVAL;
    }

    if (output_name == 0)
    {
        logger_log(LOG_FATAL, "audio_init: null output_name pointer");
        return -EINVAL;
    }

    *handle = (struct audio_t*)malloc(sizeof(struct audio_t));
    if (*handle == 0)
    {
        logger_log(LOG_FATAL, "audio_init: could not allocate memory");
        return -ENOMEM;
    }

    (*handle)->buffer_size = computeSize(quality);
    (*handle)->output_name = output_name;
    
    ret = audio_backend_get_by_name(backend_type, &((*handle)->backend));
    if (ret != 0)
    {
        logger_log(LOG_FATAL, "audio_init: %s backend not available", backend_type);
    }

    return ret;
}

int audio_release(audio_handle_t* handle)
{
    int ret = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "audio_release: null handle pointer");
        return -EINVAL;
    }

    if (*handle != 0)
    {
        ret = (*handle)->backend->close((*handle)->backend);
        free(*handle);
        *handle = 0;
    }

    return ret;
}

int audio_set_channels(audio_handle_t handle, unsigned char const* channels, int channels_size)
{
    int ret = 0;

    if ((handle == 0) || (channels == 0))
    {
        logger_log(LOG_ERROR, "audio_set_channels: handle pointer or channels buffer pointer is null");
        return -EINVAL;
    }

    if (handle->channels_buffer != 0)
    {
        free(handle->channels_buffer);
    }

    handle->channels_buffer = malloc(sizeof(struct VBanHeader) + VBAN_PROTOCOL_MAX_SIZE);
    if (handle->channels_buffer == 0)
    {
        logger_log(LOG_FATAL, "audio_set_channels: could not allocate memory");
        return -ENOMEM;
    }

    handle->channels = channels;
    handle->channels_size = channels_size;

    logger_log(LOG_INFO, "audio_set_channels: setting up channels selection for %u channels", handle->channels_size);

    return ret;
}

int audio_process_packet(audio_handle_t handle, char const* buffer, int size)
{
    struct VBanHeader const* hdr = (struct VBanHeader const*)buffer;
    int payload_size = 0;
    int ret = 0;
    long sample_rate = 0;
    int nb_samples = 0;
    int nb_channels = 0;
    int sample_size = 0;
    enum VBanBitResolution bit_resolution = VBAN_BIT_RESOLUTION_MAX;

    if ((handle == 0) || (buffer == 0))
    {
        logger_log(LOG_ERROR, "audio_process_packet: handle pointer or buffer pointer is null");
        return -EINVAL;
    }

    /** extract audio parameters from header */
    nb_samples      = hdr->format_nbs + 1;
    nb_channels     = hdr->format_nbc + 1;
    bit_resolution  = (hdr->format_bit & VBAN_BIT_RESOLUTION_MASK);

    /** error in bit_resolution */
    if (bit_resolution >= VBAN_BIT_RESOLUTION_MAX)
    {
        goto end;
    }

    sample_size     = VBanBitResolutionSize[bit_resolution];
    payload_size    = nb_samples * sample_size * nb_channels;

    /** check expected payload size */
    if (payload_size != (size - VBAN_HEADER_SIZE))
    {
        goto end;
    }

    if (handle->channels != 0)
    {
        ret = audio_extract_channels(handle, buffer, size);
        if (ret != 0)
        {
            logger_log(LOG_ERROR, "audio_process_packet: unable to extract selected channels");
            goto end;
        }
        hdr = (struct VBanHeader const*)handle->channels_buffer;
        nb_channels     = hdr->format_nbc + 1;
    }

    /** check that format is supported */
    sample_rate     = ((hdr->format_SR & VBAN_SR_MASK) < VBAN_SR_MAXNUMBER)
                        ? VBanSRList[(hdr->format_SR & VBAN_SR_MASK)]
                        : 0;
    ret = handle->backend->is_fmt_supported(handle->backend, bit_resolution, nb_channels, sample_rate);
    if (ret <= 0)
    {
        goto end;
    }

    /** check that audio device is configured at correct format */
    if ((bit_resolution != handle->bit_resolution) 
        || (nb_channels != handle->nb_channels) 
        || (sample_rate != handle->rate))
    {
        /* we have to (re-)open the device with correct format */
        ret = audio_open(handle, bit_resolution, nb_channels, sample_rate);
        if (ret < 0)
        {
            /* some error occured */
            goto end;
        }
    }

    /** write payload to the audio device */
    ret = handle->backend->write(handle->backend, (char const*)(hdr + 1), nb_samples);

end:
    return ret;
}

int audio_extract_channels(audio_handle_t handle, char const* buffer, size_t size)
{
    int ret = 0;
    struct VBanHeader const* const orig_hdr = (struct VBanHeader const*)buffer;
    char const* const orig_data = (char const*)(orig_hdr + 1);
    struct VBanHeader* const dest_hdr = (struct VBanHeader*)handle->channels_buffer;
    char* const dest_data = (char*)(dest_hdr + 1);
    size_t const sample_size = VBanBitResolutionSize[(dest_hdr->format_bit & VBAN_BIT_RESOLUTION_MASK)];
    char* dest_ptr;
    char const* orig_ptr;

    size_t chan = 0;
    size_t sample = 0;

    if ((handle == 0) || (buffer == 0))
    {
        logger_log(LOG_FATAL, "audio_extract_channels: handle or buffer pointer is null");
        return -EINVAL;
    }

    *dest_hdr               = *orig_hdr;
    dest_hdr->format_nbc    = handle->channels_size - 1;

    memset(dest_data, 0, (dest_hdr->format_nbc + 1)* (dest_hdr->format_nbs + 1) * sample_size);

    /* TODO: can this be optimized ? */
    for (chan = 0; chan != dest_hdr->format_nbc + 1; ++chan)
    {
        if (handle->channels[chan] < (orig_hdr->format_nbc + 1))
        {
            for (sample = 0; sample != dest_hdr->format_nbs + 1; ++sample)
            {
                dest_ptr = dest_data + (((sample * (dest_hdr->format_nbc + 1)) + chan) * sample_size);
                orig_ptr = orig_data + (((sample * (orig_hdr->format_nbc + 1)) + handle->channels[chan]) * sample_size);
                memcpy(dest_ptr, orig_ptr, sample_size);
            }
        }
    }
    
    return ret;
}

int audio_open(audio_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate)
{
    int ret = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "audio_open: handle pointer is null");
        return -EINVAL;
    }

    ret = handle->backend->close(handle->backend);
    if (ret < 0)
    {
        return ret;
    }

    ret = handle->backend->open(handle->backend, handle->output_name, bit_resolution, nb_channels, rate, handle->buffer_size);

    handle->bit_resolution   = bit_resolution;
    handle->nb_channels      = nb_channels;
    handle->rate            = rate;

    return ret;
}

