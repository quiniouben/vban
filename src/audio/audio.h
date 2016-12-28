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

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include "vban/vban.h"
#include <stddef.h>
#include <errno.h>

#define AUDIO_OUTPUT_NAME_SIZE      64

struct audio_t;
typedef struct audio_t* audio_handle_t;

/*XXX I would like to have a audio_get_options and a audio_parse_options */

int audio_init(audio_handle_t* handle, char const* backend_type, char const* output_name, unsigned char quality);
int audio_release(audio_handle_t* handle);

int audio_set_channels(audio_handle_t handle, unsigned char const* channels, int channels_size);
int audio_process_packet(audio_handle_t handle, char const* buffer, int size);

#endif /*__AUDIO_H__*/
