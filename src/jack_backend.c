#include "jack_backend.h"
#include <jack/jack.h>
#include <errno.h>
#include "logger.h"

struct jack_backend_t
{
    struct audio_backend_t  parent;
    jack_client_t*          jack_handle;
    char*                   buffer;
};

static int jack_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
static int jack_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size);
static int jack_close(audio_backend_handle_t handle);
static int jack_write(audio_backend_handle_t handle, char const* data, size_t nb_sample);

int jack_backend_init(audio_backend_handle_t* handle)
{
    struct jack_backend_t* jack_backend = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "jack_backend_init: null handle pointer");
        return -EINVAL;
    }

    jack_backend = (struct jack_backend_t*)malloc(sizeof(struct jack_backend_t));
    if (jack_backend == 0)
    {
        logger_log(LOG_FATAL, "jack_backend_init: could not allocate memory");
        return -ENOMEM;
    }

    jack_backend->parent.is_fmt_supported   = jack_is_fmt_supported;
    jack_backend->parent.open               = jack_open;
    jack_backend->parent.close              = jack_close;
    jack_backend->parent.write              = jack_write;

    *handle = (audio_backend_handle_t)jack_backend;

    /*XXX no implemented yet */
    logger_log(LOG_FATAL, "jack_backend_init: backend not yet implemented");

    return -EINVAL;
    
}

int jack_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate)
{
    //XXX it is up to this implementation to provide format conversion, as jack server only expects 32bit floats single channels
    /*XXX*/
    return 1;
}

int jack_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size)
{
/*    int ret;
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)handle;*/

    //XXX

    return 0;
}

int jack_close(audio_backend_handle_t handle)
{
    int ret = 0;
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "jack_close: handle pointer is null");
        return -EINVAL;
    }

    if (jack_backend->jack_handle == 0)
    {
        /** nothing to do */
        return 0;
    }

    //XXX

    return ret;
}

int jack_write(audio_backend_handle_t handle, char const* data, size_t nb_sample)
{
    int ret = 0;
/*    int error;*/
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "jack_write: handle or data pointer is null");
        return -EINVAL;
    }

    if (jack_backend->jack_handle == 0)
    {
        logger_log(LOG_ERROR, "jack_write: device not open");
        return -ENODEV;
    }

    //XXX convert incoming data

    return ret;
}

