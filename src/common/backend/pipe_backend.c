#include "pipe_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "common/logger.h"

#ifndef _WIN32
#define FIFO_FILENAME   "/tmp/vban_0"
#else
// name of named pipe is \\.\pipe\vban_0
#include <windef.h> // for DWORD and others
#include <winbase.h>
#define FIFO_FILENAME   "\\\\.\\pipe\\vban_0"
#endif

struct pipe_backend_t
{
    struct audio_backend_t  parent;
    int fd;
};

static int pipe_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config);
static int pipe_close(audio_backend_handle_t handle);
static int pipe_write(audio_backend_handle_t handle, char const* data, size_t size);
static int pipe_read(audio_backend_handle_t handle, char* data, size_t size);

int pipe_backend_init(audio_backend_handle_t* handle)
{
    struct pipe_backend_t* pipe_backend = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: null handle pointer", __func__);
        return -EINVAL;
    }

    pipe_backend = calloc(1, sizeof(struct pipe_backend_t));
    if (pipe_backend == 0)
    {
        logger_log(LOG_FATAL, "%s: could not allocate memory", __func__);
        return -ENOMEM;
    }

    pipe_backend->parent.open               = pipe_open;
    pipe_backend->parent.close              = pipe_close;
    pipe_backend->parent.write              = pipe_write;
    pipe_backend->parent.read               = pipe_read;

    *handle = (audio_backend_handle_t)pipe_backend;

    return 0;

}

int pipe_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config)
{
    int ret;
    struct pipe_backend_t* const pipe_backend = (struct pipe_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
        return -EINVAL;
    }

#ifndef _WIN32
    ret = mkfifo(FIFO_FILENAME, 0666);
    if (ret < 0)
    {
        logger_log(LOG_FATAL, "%s: ???", __func__);
        //todo:strerror
        perror("mknod");
        pipe_backend->fd = 0;
        return ret;
    }
#else
    // Windows has no mkfifo function. Use named pipes instead
    HANDLE named_pipe = CreateNamedPipeA(
        /* lpName */               FIFO_FILENAME,
        /* dwOpenMode */           PIPE_ACCESS_DUPLEX,
        /* dwPipeMode */           PIPE_TYPE_MESSAGE, // or maybe PIPE_TYPE_BYTE, let's see
        /* nMaxInstances */        1,
        /* nOutBufferSize */       0,
        /* nInBufferSize */        0,
        /* nDefaultTimeOut */      0,
        /* lpSecurityAttributes */ 0);
    if (named_pipe == INVALID_HANDLE_VALUE)
    {
        logger_log(LOG_FATAL, "%s: ???", __func__);
        //todo:strerror
        perror("mknod");
        pipe_backend->fd = 0;
        ret = GetLastError();
        return ret;
    }
#endif // _WIN32

    pipe_backend->fd = open((output_name[0] == 0) ? FIFO_FILENAME : output_name, (direction == AUDIO_OUT) ? O_WRONLY : O_RDONLY);
    if (pipe_backend->fd == -1)
    {
        logger_log(LOG_FATAL, "%s: open error", __func__); //
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
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
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

int pipe_write(audio_backend_handle_t handle, char const* data, size_t size)
{
    int ret = 0;
    struct pipe_backend_t* const pipe_backend = (struct pipe_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "%s: handle or data pointer is null", __func__);
        return -EINVAL;
    }

    ret = write(pipe_backend->fd, (const void *)data, size);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s:", __func__);
        perror("write");
    }
    return ret;
}

int pipe_read(audio_backend_handle_t handle, char* data, size_t size)
{
    int ret = 0;
    struct pipe_backend_t* const pipe_backend = (struct pipe_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "%s: handle or data pointer is null", __func__);
        return -EINVAL;
    }

    ret = read(pipe_backend->fd, (void *)data, size);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s:", __func__);
        perror("read");
    }
    return ret;
}
