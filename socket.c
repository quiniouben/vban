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

#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <unistd.h>

struct socket_t
{
    int     fd;
};

int socket_init(socket_handle_t* handle)
{
    if (handle == 0)
    {
        printf("socket_init: null handle pointer\n");
        return -EINVAL;
    }

    *handle = (struct socket_t*)malloc(sizeof(struct socket_t));
    if (*handle == 0)
    {
        printf("socket_init: could not allocate memory\n");
        return -ENOMEM;
    }

    return 0;
}

int socket_release(socket_handle_t* handle)
{
    int ret = 0;

    if (handle == 0)
    {
        printf("socket_release: null handle pointer\n");
        return -EINVAL;
    }

    if (*handle != 0)
    {
        ret = socket_close(*handle);
        free(*handle);
        *handle = 0;
    }

    return ret;
}

int socket_open(socket_handle_t handle, short port, unsigned int timeout)
{
    int ret = 0;
    struct sockaddr_in si_me;

    if (handle == 0)
    {
        printf("socket_open: one parameter is a null pointer\n");
        return -EINVAL;
    }

    if (handle->fd != 0)
    {
        ret = socket_close(handle);
        if (ret != 0)
        {
            printf("socket_open: socket was open and unable to close it\n");
            return ret;
        }
    }

    handle->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (handle->fd < 0)
    {
        printf("socket_open: unable to create socket\n");
        ret = handle->fd;
        handle->fd = 0;
        return ret;
    }

    memset(&si_me, 0, sizeof(si_me));
    si_me.sin_family        = AF_INET;
    si_me.sin_port          = htons(port);
    si_me.sin_addr.s_addr   = htonl(INADDR_ANY);
    ret = bind(handle->fd, (struct sockaddr const*)&si_me, sizeof(si_me));

    if (ret < 0)
    {
        printf("socket_open: unable to bind socket\n");
        socket_close(handle);
        return errno;
    }

    return 0;
}

int socket_close(socket_handle_t handle)
{
    int ret = 0;

    if (handle == 0)
    {
        printf("socket_close: handle parameter is a null pointer\n");
        return -EINVAL;
    }

    if (handle->fd != 0)
    {
        ret = close(handle->fd);
        handle->fd = 0;
        if (ret != 0)
        {
            printf("socket_close: unable to close socket\n");
            return ret;
        }
    }

    return 0;
}

int socket_recvfrom(socket_handle_t handle, char* buffer, size_t size, char* ipfrom)
{
    int ret = 0;
    struct sockaddr_in si_other;
    socklen_t slen = sizeof(si_other);

    if ((handle == 0) || (buffer == 0) || (ipfrom == 0))
    {
        printf("socket_readfrom: one parameter is a null pointer\n");
        return -EINVAL;
    }

    if (handle->fd == 0)
    {
        printf("socket_readfrom: socket is not open\n");
        return -ENODEV;
    }

    ret = recvfrom(handle->fd, buffer, size, 0, (struct sockaddr *) &si_other, &slen);
    if (ret < 0)
    {
        if (errno != EINTR)
        {
            printf("socket_readfrom: recvfrom error %d %s\n", errno, strerror(errno));
        }
        return ret;
    }

    strncpy(ipfrom, inet_ntoa(si_other.sin_addr), 32);

    return ret;
}

