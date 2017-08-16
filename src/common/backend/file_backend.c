/*
 *  This file is part of vban.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
 *  Copyright (c) 2017 by Markus Buschhoff <markus.buschhoff@tu-dortmund.de>
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

#include "file_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "common/logger.h"

struct file_backend_t
{
    struct audio_backend_t  parent;
    int	fd;
};

static int file_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config);
static int file_close(audio_backend_handle_t handle);
static int file_write(audio_backend_handle_t handle, char const* data, size_t size);
static int file_read(audio_backend_handle_t handle, char* data, size_t size);

int file_backend_init(audio_backend_handle_t* handle)
{
    struct file_backend_t* file_backend = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: null handle pointer", __func__);
        return -EINVAL;
    }

    file_backend = calloc(1, sizeof(struct file_backend_t));
    if (file_backend == 0)
    {
        logger_log(LOG_FATAL, "%s: could not allocate memory", __func__);
        return -ENOMEM;
    }

    file_backend->parent.open               = file_open;
    file_backend->parent.close              = file_close;
    file_backend->parent.write              = file_write;
    file_backend->parent.read               = file_read;

    *handle = (audio_backend_handle_t)file_backend;

    return 0;
    
}

int file_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config)
{
    struct file_backend_t* const file_backend = (struct file_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
        return -EINVAL;
    }

    if(strcmp("", output_name))
        file_backend->fd = open(output_name, (direction == AUDIO_OUT) ? (O_CREAT|O_WRONLY|O_TRUNC) : O_RDONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    else
        file_backend->fd = STDOUT_FILENO; 

        
    if (file_backend->fd == -1)
    {
        logger_log(LOG_FATAL, "%s: open error", __func__); //
	perror("open");
        return -errno;
    }
    
    return 0;
}

int file_close(audio_backend_handle_t handle)
{
    int ret = 0;
    struct file_backend_t* const file_backend = (struct file_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
        return -EINVAL;
    }

    if (file_backend== 0)
    {
        /** nothing to do */
        return 0;
    }

    if (file_backend->fd != STDOUT_FILENO)
        ret = close(file_backend->fd);
        
    return ret;
}

int file_write(audio_backend_handle_t handle, char const* data, size_t size)
{
    int ret = 0;
    struct file_backend_t* const file_backend = (struct file_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "%s: handle or data pointer is null", __func__);
        return -EINVAL;
    }

    ret = write(file_backend->fd, (const void *)data, size);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s:", __func__);
        perror("write");
    }
    return ret;
}

int file_read(audio_backend_handle_t handle, char* data, size_t size)
{
    int ret = 0;
    struct file_backend_t* const file_backend = (struct file_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "%s: handle or data pointer is null", __func__);
        return -EINVAL;
    }

    ret = read(file_backend->fd, (void *)data, size);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s:", __func__);
        perror("write");
    }

    return ret;
}

