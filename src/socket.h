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

#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <stddef.h>

struct socket_t;
typedef struct socket_t* socket_handle_t;

int socket_init(socket_handle_t* handle);
int socket_release(socket_handle_t* handle);

int socket_open(socket_handle_t handle, short port, unsigned int timeout);
int socket_close(socket_handle_t handle);
int socket_recvfrom(socket_handle_t handle, char* buffer, size_t size, char* ipfrom);

#endif /*__SOCKET_H__*/

