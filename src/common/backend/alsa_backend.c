#include "alsa_backend.h"
#include <alsa/asoundlib.h>
#include "common/logger.h"

#define ALSA_DEVICE_NAME_DEFAULT   "default"

struct alsa_backend_t
{
    struct audio_backend_t  parent;
    snd_pcm_t*              alsa_handle;
    size_t                  frame_size;
};

static int alsa_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config);
static int alsa_close(audio_backend_handle_t handle);
static int alsa_write(audio_backend_handle_t handle, char const* data, size_t size);
static int alsa_read(audio_backend_handle_t handle, char* data, size_t size);

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
        logger_log(LOG_FATAL, "%s: null handle pointer", __func__);
        return -EINVAL;
    }

    alsa_backend = calloc(1, sizeof(struct alsa_backend_t));
    if (alsa_backend == 0)
    {
        logger_log(LOG_FATAL, "%s: could not allocate memory", __func__);
        return -ENOMEM;
    }

    alsa_backend->parent.open               = alsa_open;
    alsa_backend->parent.close              = alsa_close;
    alsa_backend->parent.write              = alsa_write;
    alsa_backend->parent.read               = alsa_read;

    *handle = (audio_backend_handle_t)alsa_backend;

    return 0;
    
}

int alsa_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config)
{
    int ret;
    struct alsa_backend_t* const alsa_backend = (struct alsa_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
        return -EINVAL;
    }

    alsa_backend->frame_size = VBanBitResolutionSize[config->bit_fmt] * config->nb_channels;

    ret = snd_pcm_open(&alsa_backend->alsa_handle, (output_name[0] == '\0') ? ALSA_DEVICE_NAME_DEFAULT : output_name, 
        (direction == AUDIO_OUT) ? SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE, 0);
    if (ret < 0)
    {
        logger_log(LOG_FATAL, "%s: open error: %s", __func__, snd_strerror(ret));
        alsa_backend->alsa_handle = 0;
        return ret;
    }

    logger_log(LOG_DEBUG, "%s: snd_pcm_open", __func__);

    ret = snd_pcm_set_params(alsa_backend->alsa_handle,
                                vban_to_alsa_format(config->bit_fmt),
                                SND_PCM_ACCESS_RW_INTERLEAVED,
                                config->nb_channels,
                                config->sample_rate,
                                1,
                                //XXX this is wrong !
                                ((unsigned int)buffer_size * 1000000) / config->sample_rate);

    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s: set_params error: %s", __func__, snd_strerror(ret));
        alsa_close(handle);
        return ret;
    }

    ret = snd_pcm_prepare(alsa_backend->alsa_handle);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s: prepare error: %s", __func__, snd_strerror(ret));
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
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
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

int alsa_write(audio_backend_handle_t handle, char const* data, size_t size)
{
    int ret = 0;
    struct alsa_backend_t* const alsa_backend = (struct alsa_backend_t*)handle;
    size_t nb_frame = 0;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "%s: handle or data pointer is null", __func__);
        return -EINVAL;
    }

    if (alsa_backend->alsa_handle == 0)
    {
        logger_log(LOG_ERROR, "%s: device not open", __func__);
        return -ENODEV;
    }

    nb_frame = size / alsa_backend->frame_size;
    
    ret = snd_pcm_writei(alsa_backend->alsa_handle, data, nb_frame);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s: snd_pcm_writei failed: %s", __func__, snd_strerror(ret));
        ret = snd_pcm_recover(alsa_backend->alsa_handle, ret, 0);
        if (ret < 0)
        {
            logger_log(LOG_ERROR, "%s: snd_pcm_recover failed: %s", __func__, snd_strerror(ret));
        }
    }
    else if (ret > 0 && ret < nb_frame)
    {
        logger_log(LOG_ERROR, "%s: short write (expected %lu, wrote %i)", __func__, nb_frame, ret);
    }

    return ret * alsa_backend->frame_size;
}

int alsa_read(audio_backend_handle_t handle, char* data, size_t size)
{
    int ret = 0;
    struct alsa_backend_t* const alsa_backend = (struct alsa_backend_t*)handle;
    size_t nb_frame = 0;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "%s: handle or data pointer is null", __func__);
        return -EINVAL;
    }

    if (alsa_backend->alsa_handle == 0)
    {
        logger_log(LOG_ERROR, "%s: device not open", __func__);
        return -ENODEV;
    }

    nb_frame = size / alsa_backend->frame_size;

    ret = snd_pcm_readi(alsa_backend->alsa_handle, data, nb_frame);
    if (ret < 0)
    {
        logger_log(LOG_ERROR, "%s: snd_pcm_writei failed: %s", __func__, snd_strerror(ret));
        ret = snd_pcm_recover(alsa_backend->alsa_handle, ret, 0);
        if (ret < 0)
        {
            logger_log(LOG_ERROR, "%s: snd_pcm_writei failed: %s", __func__, snd_strerror(ret));
        }
    }
    else if (ret > 0 && ret < nb_frame)
    {
        logger_log(LOG_ERROR, "%s: short read (expected %lu, wrote %i)", __func__, nb_frame, ret);
    }

    return ret * alsa_backend->frame_size;
}

