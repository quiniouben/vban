/*
 *  This file is part of vban.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
 *
 *  vban is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  vban is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vban.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "audio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backend/audio_backend.h"
#include "common/logger.h"

#define AUDIO_DEVICE        "default"

struct audio_t
{
    struct audio_config_t       config;
    struct stream_config_t      stream;
    struct audio_map_config_t   map;

    audio_backend_handle_t      backend;
    /* only used if there is a map configured */
    char                        buffer[VBAN_DATA_MAX_SIZE];
};

static void get_device_config(audio_handle_t handle, struct stream_config_t* device_config);

int audio_parse_map_config(struct audio_map_config_t* map_config, char* argv)
{
    size_t index = 0;
    unsigned int chan = 0;
    char* token;

    if ((map_config == 0) || (argv == 0))
    {
        logger_log(LOG_FATAL, "%s: null pointer argument", __func__);
        return -EINVAL;
    }

    token = strtok(argv, ",");

    while ((index < VBAN_CHANNELS_MAX_NB) && (token != 0))
    {
        if (sscanf(token, "%u", &chan) == EOF)
            break;

        if ((chan > VBAN_CHANNELS_MAX_NB) || (chan < 1))
        {
            logger_log(LOG_ERROR, "%s: invalid channel id %u, stop parsing", __func__, chan);
            break;
        }

        map_config->channels[index++] = (unsigned char)(chan - 1);
        token = strtok(0, ",");
    }

    map_config->nb_channels = index;

    return 0;
}

int audio_init(audio_handle_t* handle, struct audio_config_t const* config)
{
    int ret = 0;

    if ((handle == 0) || (config == 0))
    {
        logger_log(LOG_FATAL, "%s: null pointer argument", __func__);
        return -EINVAL;
    }

    *handle = (struct audio_t*)calloc(1, sizeof(struct audio_t));
    if (*handle == 0)
    {
        logger_log(LOG_FATAL, "%s: could not allocate memory", __func__);
        return -ENOMEM;
    }

    (*handle)->config       = *config;

    logger_log(LOG_INFO, "%s: config is direction %s, backend %s, device %s, buffer size %d",
        __func__, (config->direction == AUDIO_IN) ? "in" : "out", config->backend_name, config->device_name, config->buffer_size);
    
    ret = audio_backend_get_by_name(config->backend_name, &((*handle)->backend));
    if (ret != 0)
    {
        logger_log(LOG_FATAL, "%s: %s backend not available", __func__, config->backend_name);
        free(*handle);
        *handle = 0;
        return -ENODEV;
    }

    return ret;
}

int audio_release(audio_handle_t* handle)
{
    int ret = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: null handle pointer", __func__);
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

void get_device_config(audio_handle_t handle, struct stream_config_t* device_config)
{
    *device_config = handle->stream;

    if ((handle->config.direction == AUDIO_IN) && (handle->map.nb_channels != 0))
    {
        device_config->nb_channels = handle->map.nb_channels;
    }
}

int audio_set_stream_config(audio_handle_t handle, struct stream_config_t const* config)
{
    int ret = 0;
    struct stream_config_t device_config;

    if ((handle == 0) || (config == 0))
    {
        logger_log(LOG_FATAL, "%s: null pointer argument", __func__);
        return -EINVAL;
    }

    if ((handle->stream.nb_channels == config->nb_channels)
        && (handle->stream.sample_rate == config->sample_rate)
        && (handle->stream.bit_fmt == config->bit_fmt))
    {
        /* nothing to do */
        return ret;
    }

    logger_log(LOG_INFO, "%s: new stream config is nb channels %d, sample rate %d, bit_fmt %s",
        __func__, config->nb_channels, config->sample_rate, stream_print_bit_fmt(config->bit_fmt));

    ret = handle->backend->close(handle->backend);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s: could not close backend", __func__);
        return ret;
    }

    handle->stream = *config;
    get_device_config(handle, &device_config);

    ret = handle->backend->open(handle->backend, handle->config.device_name, handle->config.direction, handle->config.buffer_size, &device_config);
    if (ret < 0)
    {
        memset(&handle->stream, 0, sizeof(handle->stream));
        logger_log(LOG_ERROR, "%s: could not open backend with new config", __func__);
        return ret;
    }

    return ret;
}

int audio_get_stream_config(audio_handle_t handle, struct stream_config_t* config)
{
    int ret = 0;

    if ((handle == 0) || (config == 0))
    {
        logger_log(LOG_FATAL, "%s: null pointer argument", __func__);
        return -EINVAL;
    }

    *config = handle->stream;

    if ((handle->config.direction == AUDIO_OUT) && (handle->map.nb_channels != 0))
    {
        config->nb_channels = handle->map.nb_channels;
    }

    return ret;
}

int audio_set_map_config(audio_handle_t handle, struct audio_map_config_t const* config)
{
    int ret = 0;

    if ((handle == 0) || (config == 0))
    {
        logger_log(LOG_FATAL, "%s: null pointer argument", __func__);
        return -EINVAL;
    }

    logger_log(LOG_INFO, "%s: new map config is nb channels %d", __func__, config->nb_channels);

    handle->map = *config;

#if 0
    size_t nb_channels;
    nb_channels = handle->map.nb_channels;
    // probably we don't want this, as we want to do this before starting
    struct stream_config_t device_config;
    if ((nb_channels != config->nb_channels) && (handle->config.direction == AUDIO_OUT))
    {
        ret = handle->backend->close(handle->backend);
        if (ret < 0)
        {
            logger_log(LOG_ERROR, "%s: could not close backend", __func__);
            return ret;
        }

        get_device_config(handle, &device_config);
        ret = handle->backend->open(handle->backend, handle->config.device_name, handle->config.direction, handle->config.buffer_size, &device_config);
        if (ret < 0)
        {
            memset(&handle->stream, 0, sizeof(handle->stream));
            logger_log(LOG_ERROR, "%s: could not open backend with new config", __func__);
            return ret;
        }
    }
#endif
    return ret;
}

int audio_write(audio_handle_t handle, char const* buffer, size_t size)
{
    logger_log(LOG_DEBUG, "%s invoked with size %d", __func__, size);

    if ((handle == 0) || (buffer == 0))
    {
        logger_log(LOG_FATAL, "%s: null pointer argument", __func__);
        return -EINVAL;
    }

    //XXX map !

    return handle->backend->write(handle->backend, buffer, size);
}

int audio_read(audio_handle_t handle, char* buffer, size_t size)
{
    logger_log(LOG_DEBUG, "%s invoked with size %d", __func__, size);

    if ((handle == 0) || (buffer == 0))
    {
        logger_log(LOG_FATAL, "%s: null pointer argument", __func__);
        return -EINVAL;
    }

    //XXX map !

    return handle->backend->read(handle->backend, buffer, size);
}

#if 0
static int audio_extract_channels(audio_handle_t handle, char const* buffer, size_t size);

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
        logger_log(LOG_FATAL, "%s: handle or buffer pointer is null", __func__);
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

#endif
