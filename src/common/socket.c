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

#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#else // _WIN32
#include <winsock2.h>
#endif
#include <unistd.h>
#include "common/logger.h"

struct socket_t
{
    struct socket_config_t  config;
#ifndef _WIN32
    int                     fd;
#else // _WIN32
    SOCKET fd;
#endif
};

static int socket_open(socket_handle_t handle);
static int socket_close(socket_handle_t handle);
static int socket_is_broadcast_address(char const* ip);

int socket_init(socket_handle_t* handle, struct socket_config_t const* config)
{
    int ret = 0;

    if ((handle == 0) || (config == 0))
    {
        logger_log(LOG_FATAL, "%s: null handle or config pointer", __func__);
        return -EINVAL;
    }

    *handle = calloc(1, sizeof(struct socket_t));
    if (*handle == 0)
    {
        logger_log(LOG_FATAL, "%s: could not allocate memory", __func__);
        return -ENOMEM;
    }
#ifdef _WIN32
    WSADATA wsadata;
    ret = WSAStartup(MAKEWORD(2,2), &wsadata);
    if (ret != 0)
    {
        logger_log(LOG_FATAL, "%s: WSAStartup failed: %d", __func__, ret);
        return -EINVAL;
    }
    (*handle)->fd = INVALID_SOCKET;
#endif

    (*handle)->config = *config;

    ret = socket_open(*handle);
    if (ret != 0)
    {
        socket_release(handle);
    }

    return ret;
}

int socket_release(socket_handle_t* handle)
{
    int ret = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: null handle pointer", __func__);
        return -EINVAL;
    }

    if (*handle != 0)
    {
        ret = socket_close(*handle);
        free(*handle);
        *handle = 0;
    }
#ifdef _WIN32
    WSACleanup();
#endif

    return ret;
}

int socket_is_broadcast_address(char const* ip)
{
    return strncmp(ip + strlen(ip) - 3, "255", 3) == 0;
}

int socket_open(socket_handle_t handle)
{
    int ret = 0;
#ifndef _WIN32
    int optflag = 0;
#else // _WIN32
    // optflag is used as optval for setsockopt(), windows expects it to be a char
    // posix expects a void *
    // from https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-setsockopt
    // int setsockopt(
    //   SOCKET     s,
    //   int        level,
    //   int        optname,
    //   const char *optval,
    //   int        optlen
    // );
    char optflag = 0;
#endif
    struct sockaddr_in si_me;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: one parameter is a null pointer", __func__);
        return -EINVAL;
    }

    logger_log(LOG_INFO, "%s: opening socket with port %d", __func__, handle->config.port);

#ifndef _WIN32
    if (handle->fd != 0)
#else // _WIN32
    if (handle->fd != INVALID_SOCKET)
#endif
    {
        ret = socket_close(handle);
        if (ret != 0)
        {
            logger_log(LOG_ERROR, "%s: socket was open and unable to close it", __func__);
            return ret;
        }
    }

    handle->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#ifndef _WIN32
    if (handle->fd < 0)
    {
        logger_log(LOG_ERROR, "%s: unable to create socket", __func__);
        ret = handle->fd;
        handle->fd = 0;
        return ret;
    }
#else // _WIN32
    if (handle->fd == INVALID_SOCKET)
    {
        logger_log(LOG_ERROR, "%s: unable to create socket", __func__);
        ret = -1;
        handle->fd = INVALID_SOCKET;
        return ret;
    }
#endif
    
    if (handle->config.direction == SOCKET_IN)
    {
        memset(&si_me, 0, sizeof(si_me));
        si_me.sin_family        = AF_INET;
        si_me.sin_port          = htons(handle->config.port);
        si_me.sin_addr.s_addr   = htonl(INADDR_ANY);
        ret = bind(handle->fd, (struct sockaddr const*)&si_me, sizeof(si_me));

#ifndef _WIN32
        if (ret < 0)
#else // _WIN32
        if (ret == SOCKET_ERROR)
#endif
        {
            logger_log(LOG_ERROR, "%s: unable to bind socket", __func__);
            socket_close(handle);
#ifndef _WIN32
            return errno;
#else // _WIN32
            return WSAGetLastError();
#endif
        }
    }
    else
    {
        if (socket_is_broadcast_address(handle->config.ip_address))
        {
            logger_log(LOG_DEBUG, "%s: broadcast address detected", __func__);
            optflag = 1;
            ret = setsockopt(handle->fd, SOL_SOCKET, SO_BROADCAST, &optflag, sizeof(optflag));
            if (ret < 0)
            {
                logger_log(LOG_ERROR, "%s: unable to set broadcast option", __func__);
                socket_close(handle);
                return errno;
            }
        }
    }

    logger_log(LOG_INFO, "%s with port: %d", __func__, handle->config.port);

    return 0;
}

int socket_close(socket_handle_t handle)
{
    int ret = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: handle parameter is a null pointer", __func__);
        return -EINVAL;
    }

    logger_log(LOG_INFO, "%s: closing socket with port %d", __func__, handle->config.port);

#ifndef _WIN32
    if (handle->fd != 0)
#else // _WIN32
    if (handle->fd == INVALID_SOCKET)
#endif
    {
        ret = close(handle->fd);
        handle->fd = 0;
#ifndef _WIN32
        if (ret != 0)
#else // _WIN32
        if (ret == SOCKET_ERROR)
#endif
        {
            logger_log(LOG_ERROR, "%s: unable to close socket", __func__);
            return ret;
        }
    }

    return 0;
}

int socket_read(socket_handle_t handle, char* buffer, size_t size)
{
    int ret = 0;
    struct sockaddr_in si_other;
#ifndef _WIN32
    socklen_t slen = sizeof(si_other);
#else // _WIN32
    int slen = sizeof(si_other);
#endif

    logger_log(LOG_DEBUG, "%s invoked", __func__);

    if ((handle == 0) || (buffer == 0))
    {
        logger_log(LOG_ERROR, "%s: one parameter is a null pointer", __func__);
        return -EINVAL;
    }

    logger_log(LOG_DEBUG, "%s ip %s", __func__, handle->config.ip_address);

#ifndef _WIN32
    if (handle->fd == 0)
#else // _WIN32
    if (handle->fd == INVALID_SOCKET)
#endif
    {
        logger_log(LOG_ERROR, "%s: socket is not open", __func__);
        return -ENODEV;
    }

again:
    ret = recvfrom(handle->fd, buffer, size, 0, (struct sockaddr *) &si_other, &slen);
    if (ret < 0)
    {
        if (errno != EINTR)
        {
            logger_log(LOG_ERROR, "%s: recvfrom error %d %s", __func__, errno, strerror(errno));
        }
        return ret;
    }

    if (strncmp(handle->config.ip_address, inet_ntoa(si_other.sin_addr), SOCKET_IP_ADDRESS_SIZE))
    {
        logger_log(LOG_DEBUG, "%s: packet received from wrong ip", __func__);
        goto again;
    }

    return ret;
}

int socket_write(socket_handle_t handle, char const* buffer, size_t size)
{
    int ret = 0;
    struct sockaddr_in si_other;
#ifndef _WIN32
    socklen_t slen = sizeof(si_other);
#else // _WIN32
    int slen = sizeof(si_other);
#endif

    logger_log(LOG_DEBUG, "%s invoked", __func__);

    if ((handle == 0) || (buffer == 0))
    {
        logger_log(LOG_ERROR, "%s: one parameter is a null pointer", __func__);
        return -EINVAL;
    }

    if (handle->fd == 0)
    {
        logger_log(LOG_ERROR, "%s: socket is not open", __func__);
        return -ENODEV;
    }

    memset(&si_other, 0, sizeof(si_other));
    si_other.sin_family        = AF_INET;
    si_other.sin_port          = htons(handle->config.port);
    si_other.sin_addr.s_addr   = inet_addr(handle->config.ip_address);

    ret = sendto(handle->fd, buffer, size, 0, (struct sockaddr*)&si_other, slen);
    if (ret < 0)
    {
        if (errno != EINTR)
        {
            logger_log(LOG_ERROR, "%s: sendto error %d %s", __func__, errno, strerror(errno));
        }
        return ret;
    }

    return ret;
}

