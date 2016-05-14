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
#include "logger.h"

#define AUDIO_DEVICE        "default"

struct audio_t
{
    snd_pcm_t*              alsa_handle;
    char const*             output_name;
    enum VBanBitResolution  bit_resolution;
    unsigned int            nb_channels;
    unsigned int            rate;
    size_t                  buffer_size;
};

static int audio_is_format_supported(audio_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
static int audio_open(audio_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
static int audio_close(audio_handle_t handle);
static int audio_write(audio_handle_t handle, char const* data, size_t size);
static size_t computeSize(unsigned char quality);

static snd_pcm_format_t vban_to_alsa_format(enum VBanBitResolution bit_resolution)
{
    switch (bit_resolution)
    {
        case VBAN_BITFMT_8_INT:
            return SND_PCM_FORMAT_S8;

        case VBAN_BITFMT_16_INT:
            return SND_PCM_FORMAT_S16;

        case VBAN_BITFMT_24_INT:
            return SND_PCM_FORMAT_S24;

        case VBAN_BITFMT_32_INT:
            return SND_PCM_FORMAT_S32;

        case VBAN_BITFMT_32_FLOAT:
            return SND_PCM_FORMAT_FLOAT;

        case VBAN_BITFMT_64_FLOAT:
            return SND_PCM_FORMAT_FLOAT64;

        default:
            return SND_PCM_FORMAT_UNKNOWN;
    }
}

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

int audio_init(audio_handle_t* handle, char const* output_name, unsigned char quality)
{
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

    return 0;
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
        ret = audio_close(*handle);
        free(*handle);
        *handle = 0;
    }

    return ret;
}

int audio_process_packet(audio_handle_t handle, char const* buffer, int size)
{
    struct VBanHeader const* const hdr = (struct VBanHeader const*)buffer;
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

    /** check that format is supported */
    sample_rate     = ((hdr->format_SR & VBAN_SR_MASK) < VBAN_SR_MAXNUMBER)
                        ? VBanSRList[(hdr->format_SR & VBAN_SR_MASK)]
                        : 0;
    ret = audio_is_format_supported(handle, bit_resolution, nb_channels, sample_rate);
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
    ret = audio_write(handle, (char const*)(hdr + 1), nb_samples);

end:
    return ret;
}

int audio_is_format_supported(audio_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate)
{
    /*XXX todo */
    return 1;
}

int audio_open(audio_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate)
{
    int ret = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "audio_open: handle pointer is null");
        return -EINVAL;
    }

    ret = audio_close(handle);
    if (ret < 0)
    {
        return ret;
    }

    logger_log(LOG_INFO, "audio_open with params:\n \
        \tdevice:\t%s\n \
        \tnb_channels:\t%d\n \
        \trate:\t%d", handle->output_name, nb_channels, rate);

    ret = snd_pcm_open(&handle->alsa_handle, handle->output_name, SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0)
    {
        logger_log(LOG_FATAL, "audio_open: open error: %s", snd_strerror(ret));
        handle->alsa_handle = 0;
        return ret;
    }

    logger_log(LOG_DEBUG, "audio_open: snd_pcm_open");

    ret = snd_pcm_set_params(handle->alsa_handle,
                                vban_to_alsa_format(bit_resolution),
                                SND_PCM_ACCESS_RW_INTERLEAVED,
                                nb_channels,
                                rate,
                                1,
                                ((unsigned int)handle->buffer_size * 1000000) / rate);

    if (ret < 0)
    {
        logger_log(LOG_ERROR, "audio_open: set_params error: %s", snd_strerror(ret));
        audio_close(handle);
        return ret;
    }

    ret = snd_pcm_prepare(handle->alsa_handle);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "audio_open: prepare error: %s", snd_strerror(ret));
        audio_close(handle);
        return ret;
    }

    handle->bit_resolution   = bit_resolution;
    handle->nb_channels      = nb_channels;
    handle->rate            = rate;

    return ret;
}

int audio_close(audio_handle_t handle)
{
    int ret = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "audio_close: handle pointer is null");
        return -EINVAL;
    }

    if (handle->alsa_handle == 0)
    {
        /** nothing to do */
        return 0;
    }

    ret = snd_pcm_close(handle->alsa_handle);
    handle->alsa_handle = 0;

    return ret;
}

int audio_write(audio_handle_t handle, char const* data, size_t size)
{
    int ret = 0;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "audio_write: handle or data pointer is null");
        return -EINVAL;
    }

    if (handle->alsa_handle == 0)
    {
        logger_log(LOG_ERROR, "audio_write: device not open");
        return -ENODEV;
    }
    
    ret = snd_pcm_writei(handle->alsa_handle, data, size);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "audio_write: snd_pcm_writei failed: %s", snd_strerror(ret));
        ret = snd_pcm_recover(handle->alsa_handle, ret, 0);
        if (ret < 0)
        {
            logger_log(LOG_ERROR, "audio_write: snd_pcm_writei failed: %s", snd_strerror(ret));
        }
    }
    else if (ret > 0 && ret < size)
    {
        logger_log(LOG_ERROR, "audio_write: short write (expected %lu, wrote %i)", size, ret);
    }

    return ret;
}

