#include "pulseaudio_backend.h"
#include <pulse/simple.h>
#include <pulse/error.h>
#include <errno.h>
#include "common/logger.h"

struct pulseaudio_backend_t
{
    struct audio_backend_t  parent;
    pa_simple*              pulseaudio_handle;
};

static int pulseaudio_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config);
static int pulseaudio_close(audio_backend_handle_t handle);
static int pulseaudio_write(audio_backend_handle_t handle, char const* data, size_t size);

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
        logger_log(LOG_FATAL, "%s: null handle pointer", __func__);
        return -EINVAL;
    }

    pulseaudio_backend = calloc(1, sizeof(struct pulseaudio_backend_t));
    if (pulseaudio_backend == 0)
    {
        logger_log(LOG_FATAL, "%s: could not allocate memory", __func__);
        return -ENOMEM;
    }

    pulseaudio_backend->parent.open               = pulseaudio_open;
    pulseaudio_backend->parent.close              = pulseaudio_close;
    pulseaudio_backend->parent.write              = pulseaudio_write;

    *handle = (audio_backend_handle_t)pulseaudio_backend;

    return 0;
    
}

int pulseaudio_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config)
{
    int ret;
    struct pulseaudio_backend_t* const pulseaudio_backend = (struct pulseaudio_backend_t*)handle;
    pa_sample_spec const ss = 
    {
        .format     = vban_to_pulseaudio_format(config->bit_fmt),
        .rate       = config->sample_rate,
        .channels   = config->nb_channels,
    };

    pa_buffer_attr const ba =
    {
        .maxlength  = (unsigned int)(-1),
        .tlength    = buffer_size,
        .prebuf     = buffer_size / 2,
        .minreq     = (unsigned int)(-1),
        .fragsize   = (unsigned int)(-1)
    };

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
        return -EINVAL;
    }

    pulseaudio_backend->pulseaudio_handle = pa_simple_new(0, "vban", PA_STREAM_PLAYBACK, (output_name[0] == '\0') ? 0 : output_name, "", &ss, 0, &ba, &ret);
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
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
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

int pulseaudio_write(audio_backend_handle_t handle, char const* data, size_t size)
{
    int ret = 0;
    int error;
    struct pulseaudio_backend_t* const pulseaudio_backend = (struct pulseaudio_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "%s: handle or data pointer is null", __func__);
        return -EINVAL;
    }

    if (pulseaudio_backend->pulseaudio_handle == 0)
    {
        logger_log(LOG_ERROR, "%s: device not open", __func__);
        return -ENODEV;
    }

    ret = pa_simple_write(pulseaudio_backend->pulseaudio_handle, data, size, &error);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s: pa_simple_write failed: %s", __func__, pa_strerror(error));
    }

    return (ret < 0) ? ret : size;
}

