/*
 *  This file is part of vban_receptor.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
 *  Copyright (c) 2017 by Markus Buschhoff <markus.buschhoff@tu-dortmund.de>
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

#include "file_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "util/logger.h"

struct file_backend_t
{
    struct audio_backend_t  parent;
    unsigned int sampleSize;
    int	fd;
};

static int file_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
static int file_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size);
static int file_close(audio_backend_handle_t handle);
static int file_write(audio_backend_handle_t handle, char const* data, size_t nb_sample);

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

    file_backend->parent.is_fmt_supported   = file_is_fmt_supported;
    file_backend->parent.open               = file_open;
    file_backend->parent.close              = file_close;
    file_backend->parent.write              = file_write;

    *handle = (audio_backend_handle_t)file_backend;

    return 0;
    
}

int file_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate)
{
    /*XXX*/
    return 1;
}

int file_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size)
{
    struct file_backend_t* const file_backend = (struct file_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
        return -EINVAL;
    }

    if(strcmp("", output_name))
        file_backend->fd = open(output_name, O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);
    else
        file_backend->fd = STDOUT_FILENO; 

        
    if (file_backend->fd == -1)
    {
        logger_log(LOG_FATAL, "%s: open error", __func__); //
	perror("open");
        return -errno;
    }

    file_backend->sampleSize=VBanBitResolutionSize[bit_resolution] * nb_channels;
    
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

int file_write(audio_backend_handle_t handle, char const* data, size_t nb_sample)
{
    int ret = 0;
    struct file_backend_t* const file_backend = (struct file_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "%s: handle or data pointer is null", __func__);
        return -EINVAL;
    }

    ret = write(file_backend->fd, (const void *)data, nb_sample * file_backend->sampleSize);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s:", __func__);
        perror("write");
    }
    return ret;
}

