#include "alsa_backend.h"
#include <alsa/asoundlib.h>
#include "logger.h"

#define ALSA_OUTPUT_NAME_DEFAULT   "default"

struct alsa_backend_t
{
    struct audio_backend_t  parent;
    snd_pcm_t*              alsa_handle;
};

static int alsa_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
static int alsa_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size);
static int alsa_close(audio_backend_handle_t handle);
static int alsa_write(audio_backend_handle_t handle, char const* data, size_t nb_sample);

static snd_pcm_format_t vban_to_alsa_format(enum VBanBitResolution bit_resolution)
{
    switch (bit_resolution)
    {
        case VBAN_BITFMT_8_INT:
            return SND_PCM_FORMAT_S8;

        case VBAN_BITFMT_16_INT:
            return SND_PCM_FORMAT_S16;

        case VBAN_BITFMT_24_INT:
            return SND_PCM_FORMAT_S24;

        case VBAN_BITFMT_32_INT:
            return SND_PCM_FORMAT_S32;

        case VBAN_BITFMT_32_FLOAT:
            return SND_PCM_FORMAT_FLOAT;

        case VBAN_BITFMT_64_FLOAT:
            return SND_PCM_FORMAT_FLOAT64;

        default:
            return SND_PCM_FORMAT_UNKNOWN;
    }
}

int alsa_backend_init(audio_backend_handle_t* handle)
{
    struct alsa_backend_t* alsa_backend = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "alsa_backend_init: null handle pointer");
        return -EINVAL;
    }

    alsa_backend = (struct alsa_backend_t*)malloc(sizeof(struct alsa_backend_t));
    if (alsa_backend == 0)
    {
        logger_log(LOG_FATAL, "alsa_backend_init: could not allocate memory");
        return -ENOMEM;
    }

    alsa_backend->parent.is_fmt_supported   = alsa_is_fmt_supported;
    alsa_backend->parent.open               = alsa_open;
    alsa_backend->parent.close              = alsa_close;
    alsa_backend->parent.write              = alsa_write;

    *handle = (audio_backend_handle_t)alsa_backend;

    return 0;
    
}

int alsa_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate)
{
    /*XXX*/
    return 1;
}

int alsa_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size)
{
    int ret;
    struct alsa_backend_t* const alsa_backend = (struct alsa_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "alsa_open: handle pointer is null");
        return -EINVAL;
    }


    ret = snd_pcm_open(&alsa_backend->alsa_handle, (output_name[0] == '\0') ? ALSA_OUTPUT_NAME_DEFAULT : output_name, SND_PCM_STREAM_PLAYBACK, 0);
    if (ret < 0)
    {
        logger_log(LOG_FATAL, "alsa_open: open error: %s", snd_strerror(ret));
        alsa_backend->alsa_handle = 0;
        return ret;
    }

    logger_log(LOG_DEBUG, "alsa_open: snd_pcm_open");

    ret = snd_pcm_set_params(alsa_backend->alsa_handle,
                                vban_to_alsa_format(bit_resolution),
                                SND_PCM_ACCESS_RW_INTERLEAVED,
                                nb_channels,
                                rate,
                                1,
                                ((unsigned int)buffer_size * 1000000) / rate);

    if (ret < 0)
    {
        logger_log(LOG_ERROR, "alsa_open: set_params error: %s", snd_strerror(ret));
        alsa_close(handle);
        return ret;
    }

    ret = snd_pcm_prepare(alsa_backend->alsa_handle);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "alsa_open: prepare error: %s", snd_strerror(ret));
        alsa_close(handle);
        return ret;
    }

    return 0;
}

int alsa_close(audio_backend_handle_t handle)
{
    int ret = 0;
    struct alsa_backend_t* const alsa_backend = (struct alsa_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "alsa_close: handle pointer is null");
        return -EINVAL;
    }

    if (alsa_backend->alsa_handle == 0)
    {
        /** nothing to do */
        return 0;
    }

    ret = snd_pcm_close(alsa_backend->alsa_handle);
    alsa_backend->alsa_handle = 0;

    return ret;
}

int alsa_write(audio_backend_handle_t handle, char const* data, size_t nb_sample)
{
    int ret = 0;
    struct alsa_backend_t* const alsa_backend = (struct alsa_backend_t*)handle;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "alsa_write: handle or data pointer is null");
        return -EINVAL;
    }

    if (alsa_backend->alsa_handle == 0)
    {
        logger_log(LOG_ERROR, "alsa_write: device not open");
        return -ENODEV;
    }
    
    ret = snd_pcm_writei(alsa_backend->alsa_handle, data, nb_sample);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "alsa_write: snd_pcm_writei failed: %s", snd_strerror(ret));
        ret = snd_pcm_recover(alsa_backend->alsa_handle, ret, 0);
        if (ret < 0)
        {
            logger_log(LOG_ERROR, "alsa_write: snd_pcm_writei failed: %s", snd_strerror(ret));
        }
    }
    else if (ret > 0 && ret < nb_sample)
    {
        logger_log(LOG_ERROR, "alsa_write: short write (expected %lu, wrote %i)", nb_sample, ret);
    }

    return ret;
}

