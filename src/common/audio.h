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

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "vban/vban.h"
#include "stream.h"
#include <stddef.h>
#include <errno.h>

/**
 * Maximum number of characters of audio device name
 */
#define AUDIO_DEVICE_NAME_SIZE      64

/**
 * Maximum number of characters of audio backend name
 */
#define AUDIO_BACKEND_NAME_SIZE     32

/**
 * Channel map config
 */
struct audio_map_config_t
{
    unsigned char           channels[VBAN_CHANNELS_MAX_NB];
    size_t                  nb_channels;
};

/**
 * Helper function to parse command line parameter to audio map config
 * @param map_config pointer to the map configuration to fill
 * @param argv pointer to command line parameter
 * @return 0 upon success, negative value otherwise
 */
int audio_parse_map_config(struct audio_map_config_t* map_config, char* argv);

/**
 * Direction of the audio device
 */
enum audio_direction
{
    AUDIO_IN,
    AUDIO_OUT,
};

/**
 * Configuration structure used at init
 */
struct audio_config_t
{
    enum audio_direction            direction;
    char                            backend_name[AUDIO_BACKEND_NAME_SIZE];
    char                            device_name[AUDIO_DEVICE_NAME_SIZE];
    size_t                          buffer_size;
};

/**
 * Opaque handle definitioin
 */
struct audio_t;
typedef struct audio_t* audio_handle_t;

/**
 * Init audio
 * @param handle object pointer initialized
 * @param config configuration to use
 * @return 0 upon success, negative value otherwise
 */
int audio_init(audio_handle_t* handle, struct audio_config_t const* config);

/**
 * Release audio object
 * @param handle pointer to the objet handle
 * @return 0 upon success, negative value otherwise
 */
int audio_release(audio_handle_t* handle);

/**
 * Set the stream configuration.
 * The stream configuration is what comes from vban, before the channel map, or what comes from audio 
 * before the channel map, depending on the direction used.
 * @param handle object handle
 * @param config stream configuration to use
 * @return 0 upon success, negative value otherwise
 */
int audio_set_stream_config(audio_handle_t handle, struct stream_config_t const* config);

/**
 * Get the current stream configuration
 * The stream configuration is what comes from vban, before the channel map, or what comes from audio 
 * before the channel map, depending on the direction used.
 * @param handle object handle
 * @param config stream configuration to fill
 * @return 0 upon success, negative value otherwise
 */
int audio_get_stream_config(audio_handle_t handle, struct stream_config_t* config);

/**
 * Set the channel map configuration
 * @param handle object handle
 * @param config channel map configuration
 * @return 0 upon success, negative value otherwise
 */
int audio_set_map_config(audio_handle_t handle, struct audio_map_config_t const* config);

/**
 * Write data to audio
 * @param handle object handle
 * @param buffer data to write
 * @param size size of the data to write
 * @return size written upon success, negative value otherwise
 */
int audio_write(audio_handle_t handle, char const* buffer, size_t size);

/**
 * Read data dwifrom to audio
 * @param handle object handle
 * @param buffer data to read to 
 * @param size size of the data to read
 * @return size read upon success, negative value otherwise
 */
int audio_read(audio_handle_t handle, char* buffer, size_t size);

#endif /*__AUDIO_H__*/
