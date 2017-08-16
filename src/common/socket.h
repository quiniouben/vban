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

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <stddef.h>

/**
 * Number of characters for ip address
 */
#define SOCKET_IP_ADDRESS_SIZE    32

enum socket_direction
{
    SOCKET_IN,
    SOCKET_OUT,
};


/**
 * Socket configuration structure.
 * To be used at init time
 */
struct socket_config_t
{
    enum socket_direction   direction;
    char                    ip_address[SOCKET_IP_ADDRESS_SIZE];
    short                   port;
};

/**
 * Opaque handle type
 */
struct socket_t;
typedef struct socket_t* socket_handle_t;

/**
 * Allocate and initialize the socket with the appropriate configuration
 * @param handle handle pointer that will be allocated
 * @param config configuration structure
 * @return 0 upon success, negative value otherwise
 */
int socket_init(socket_handle_t* handle, struct socket_config_t const* config);

/**
 * Release the socket
 * @param handle handle pointer that will be released
 * @return 0 upon success, negative value otherwise
 */
int socket_release(socket_handle_t* handle);

/**
 * Read data from the socket
 * @param handle object handle
 * @param buffer pointer where to put the data read
 * @param size size of @p buffer data 
 * @return size read upon success, negative value otherwise
 */
int socket_read(socket_handle_t handle, char* buffer, size_t size);

/**
 * Write data to the socket
 * @param handle object handle
 * @param buffer pointer holding data to write
 * @param size size of @p buffer data 
 * @return size written upon success, negative value otherwise
 */
int socket_write(socket_handle_t handle, char const* buffer, size_t size);

#endif /*__SOCKET_H__*/

