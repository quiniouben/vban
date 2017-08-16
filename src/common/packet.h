/*
 *  This file is part of vban.
 *  Copyright (c) 2017 by Benoît Quiniou <quiniouben@yahoo.fr>
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

#ifndef __PACKET_H__
#define __PACKET_H__

#include <stddef.h>
#include "vban/vban.h"
#include "stream.h"

/**
 * Check packet content and only return valid return value if this is an audio pcm packet
 * @param streamname string pointer holding streamname
 * @param buffer pointer to data to check
 * @param size of the data in buffer;
 * @return 0 if packet is valid, negative value otherwise
 */
int packet_check(char const* streamname, char const* buffer, size_t size);

/** Return VBanHeader pointer from buffer */
#define PACKET_HEADER_PTR(_buffer) ((struct VBanHeader*)_buffer)

/** Return payload pointer of a Vban packet from buffer pointer */
#define PACKET_PAYLOAD_PTR(_buffer) ((char*)(PACKET_HEADER_PTR(_buffer)+1))

/** Return paylod size from total packet size */
#define PACKET_PAYLOAD_SIZE(_size) (_size - sizeof(struct VBanHeader))

/**
 * Fill @p stream_config with the values corresponding to the data ini the packet
 * @param buffer pointer to data
 * @param stream_config pointer
 * @return 0 upon success, negative value otherwise
 */
int packet_get_stream_config(char const* buffer, struct stream_config_t* stream_config);

/**
 * Get max payload_size from packet header
 * @param buffer pointer to packet
 * @return size upon success, negative value otherwise
 */
int packet_get_max_payload_size(char const* buffer);

/**
 * Init header content.
 * @param buffer pointer to data
 * @param stream_config pointer
 * @return 0 upon success, negative value otherwise
 */
int packet_init_header(char* buffer, struct stream_config_t const* stream_config, char const* streamname);

/**
 * Fill the packet withe values corresponding to stream_config
 * @param buffer pointer to data
 * @param payload_size size of the payload
 * @return 0 upon success, negative value otherwise
 */
int packet_set_new_content(char* buffer, size_t payload_size);

#endif /*__PACKET_H__*/

