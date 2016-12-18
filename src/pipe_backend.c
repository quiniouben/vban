#include "pipe_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "logger.h"

#define FIFO_FILENAME   "/tmp/vban_receptor_0"

struct pipe_backend_t
{
    struct audio_backend_t  parent;
    int	fd;
};

static int pipe_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
static int pipe_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size);
static int pipe_close(audio_backend_handle_t handle);
static int pipe_write(audio_backend_handle_t handle, char const* data, size_t nb_sample);

int pipe_backend_init(audio_backend_handle_t* handle)
{
    struct pipe_backend_t* pipe_backend = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "pipe_backend_init: null handle pointer");
        return -EINVAL;
    }

    pipe_backend = (struct pipe_backend_t*)malloc(sizeof(struct pipe_backend_t));
    if (pipe_backend == 0)
    {
        logger_log(LOG_FATAL, "pipe_backend_init: could not allocate memory");
        return -ENOMEM;
    }

    pipe_backend->parent.is_fmt_supported   = pipe_is_fmt_supported;
    pipe_backend->parent.open               = pipe_open;
    pipe_backend->parent.close              = pipe_close;
    pipe_backend->parent.write              = pipe_write;

    *handle = (audio_backend_handle_t)pipe_backend;

    return 0;
    
}

int pipe_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate)
{
    /*XXX*/
    return 1;
}

int pipe_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size)
{
    int ret;
    struct pipe_backend_t* const pipe_backend = (struct pipe_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "pipe_open: handle pointer is null");
        return -EINVAL;
    }

    ret = mkfifo(FIFO_FILENAME, 0666);
    if (ret < 0)
    {
	logger_log(LOG_FATAL, "pipe_open: unable to mknod(): ???");
	//todo:strerror
	perror("mknod");
	pipe_backend->fd = 0;
	return ret;
    }

    pipe_backend->fd = open(FIFO_FILENAME, O_WRONLY);
    if (pipe_backend->fd == -1)
    {
        logger_log(LOG_FATAL, "pipe_open: open error"); //
	perror("open");
        return ret;
    }

    return 0;
}

int pipe_close(audio_backend_handle_t handle)
{
    int ret = 0;
    struct pipe_backend_t* const pipe_backend = (struct pipe_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "pipe_close: handle pointer is null");
        return -EINVAL;
    }

    if (pipe_backend== 0)
    {
        /** nothing to do */
        return 0;
    }

    ret = close(pipe_backend->fd);
    unlink(FIFO_FILENAME);
    return ret;
}

int pipe_write(audio_backend_handle_t handle, char const* data, size_t nb_sample)
{
    int ret = 0;
    struct pipe_backend_t* const pipe_backend = (struct pipe_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "pipe_write: handle or data pointer is null");
        return -EINVAL;
    }

    ret = write(pipe_backend->fd, (const void *)data, nb_sample);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "pipe_write: write failed:");
        perror("write");
    }
    return ret;
}

