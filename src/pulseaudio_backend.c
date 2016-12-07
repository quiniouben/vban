#include "pulseaudio_backend.h"
#include <pulse/simple.h>
#include <pulse/error.h>
#include <errno.h>
#include "logger.h"

struct pulseaudio_backend_t
{
    struct audio_backend_t  parent;
    pa_simple*              pulseaudio_handle;
    unsigned int            nb_sample_to_size;
};

static int pulseaudio_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
static int pulseaudio_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size);
static int pulseaudio_close(audio_backend_handle_t handle);
static int pulseaudio_write(audio_backend_handle_t handle, char const* data, size_t nb_sample);

static enum pa_sample_format vban_to_pulseaudio_format(enum VBanBitResolution bit_resolution)
{
    switch (bit_resolution)
    {
        case VBAN_BITFMT_8_INT:
            return PA_SAMPLE_U8;

        case VBAN_BITFMT_16_INT:
            return PA_SAMPLE_S16LE;

        case VBAN_BITFMT_24_INT:
            return PA_SAMPLE_S24LE;

        case VBAN_BITFMT_32_INT:
            return PA_SAMPLE_S32LE;

        case VBAN_BITFMT_32_FLOAT:
            return PA_SAMPLE_FLOAT32LE;

        case VBAN_BITFMT_64_FLOAT:
        default:
            return PA_SAMPLE_INVALID;
    }
}

int pulseaudio_backend_init(audio_backend_handle_t* handle)
{
    struct pulseaudio_backend_t* pulseaudio_backend = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "pulseaudio_backend_init: null handle pointer");
        return -EINVAL;
    }

    pulseaudio_backend = (struct pulseaudio_backend_t*)malloc(sizeof(struct pulseaudio_backend_t));
    if (pulseaudio_backend == 0)
    {
        logger_log(LOG_FATAL, "pulseaudio_backend_init: could not allocate memory");
        return -ENOMEM;
    }

    pulseaudio_backend->parent.is_fmt_supported   = pulseaudio_is_fmt_supported;
    pulseaudio_backend->parent.open               = pulseaudio_open;
    pulseaudio_backend->parent.close              = pulseaudio_close;
    pulseaudio_backend->parent.write              = pulseaudio_write;

    *handle = (audio_backend_handle_t)pulseaudio_backend;

    return 0;
    
}

int pulseaudio_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate)
{
    /*XXX*/
    return 1;
}

int pulseaudio_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size)
{
    int ret;
    struct pulseaudio_backend_t* const pulseaudio_backend = (struct pulseaudio_backend_t*)handle;
    pa_sample_spec const ss = 
    {
        .format     = vban_to_pulseaudio_format(bit_resolution),
        .rate       = rate,
        .channels   = nb_channels,
    };

    pa_buffer_attr const ba =
    {
        .maxlength  = (unsigned int)(-1),
        .tlength    = buffer_size,
        .prebuf     = buffer_size / 2,
        .minreq     = (unsigned int)(-1),
        .fragsize   = (unsigned int)(-1)
    };

    pulseaudio_backend->nb_sample_to_size = VBanBitResolutionSize[bit_resolution] * nb_channels;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "pulseaudio_open: handle pointer is null");
        return -EINVAL;
    }

    pulseaudio_backend->pulseaudio_handle = pa_simple_new(0, "vban_receptor", PA_STREAM_PLAYBACK, (output_name[0] == '\0') ? 0 : output_name, "", &ss, 0, &ba, &ret);
    if (pulseaudio_backend->pulseaudio_handle == 0)
    {
        logger_log(LOG_FATAL, "pulseaudio_open: open error: %s", pa_strerror(ret));
        return ret;
    }

    return 0;
}

int pulseaudio_close(audio_backend_handle_t handle)
{
    int ret = 0;
    struct pulseaudio_backend_t* const pulseaudio_backend = (struct pulseaudio_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "pulseaudio_close: handle pointer is null");
        return -EINVAL;
    }

    if (pulseaudio_backend->pulseaudio_handle == 0)
    {
        /** nothing to do */
        return 0;
    }

    pa_simple_free(pulseaudio_backend->pulseaudio_handle);
    pulseaudio_backend->pulseaudio_handle = 0;

    return ret;
}

int pulseaudio_write(audio_backend_handle_t handle, char const* data, size_t nb_sample)
{
    int ret = 0;
    int error;
    struct pulseaudio_backend_t* const pulseaudio_backend = (struct pulseaudio_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "pulseaudio_write: handle or data pointer is null");
        return -EINVAL;
    }

    if (pulseaudio_backend->pulseaudio_handle == 0)
    {
        logger_log(LOG_ERROR, "pulseaudio_write: device not open");
        return -ENODEV;
    }

    ret = pa_simple_write(pulseaudio_backend->pulseaudio_handle, data, nb_sample * pulseaudio_backend->nb_sample_to_size, &error);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "pulseaudio_write: pa_simple_write failed: %s", pa_strerror(error));
    }

    return ret;
}

