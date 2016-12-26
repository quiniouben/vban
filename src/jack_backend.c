#include "jack_backend.h"
#include <jack/jack.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "logger.h"

struct jack_backend_t
{
    struct audio_backend_t  parent;
    jack_client_t*          jack_client;
    jack_port_t*            ports[VBAN_CHANNELS_MAX_NB];
    char*                   buffer;
    char const*             rd_ptr;
    char*                   wr_ptr;
    char const*             trigger_ptr;
    size_t                  buffer_size;
    enum VBanBitResolution  bit_resolution;
    unsigned int            nb_channels;
    int                     active;
};

static int jack_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate);
static int jack_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size);
static int jack_close(audio_backend_handle_t handle);
static int jack_write(audio_backend_handle_t handle, char const* data, size_t nb_sample);

static int jack_process_cb(jack_nframes_t nframes, void* arg);
static void jack_shutdown_cb(void* arg);
static jack_default_audio_sample_t jack_convert_sample(char const* ptr, enum VBanBitResolution bit_resolution);
static int jack_start(struct jack_backend_t* jack_backend)
{
    int ret = 0;
    size_t port;
    char port_name[32];
    char const** ports;
    size_t port_id;

    ret = jack_activate(jack_backend->jack_client);
    if (ret)
    {
        logger_log(LOG_ERROR, "%s: can't activate client", __func__);
        return ret;
    }

    logger_log(LOG_DEBUG, "%s: jack activated", __func__);

    jack_backend->active = 1;

    for (port = 0; port != jack_backend->nb_channels; ++port)
    {
        snprintf(port_name, sizeof(port_name)-1, "playback_%u", (unsigned int)(port+1));
        jack_backend->ports[port] = jack_port_register(jack_backend->jack_client
            , port_name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        if (jack_backend->ports[port] == 0)
        {
            logger_log(LOG_ERROR, "%s: impossible to set jack port for channel %d", __func__, port);
            jack_close((audio_backend_handle_t)jack_backend);
            return -ENODEV;
        }
    }

    //XXX do we really want to autoconnect ? this should be an option
    ports = jack_get_ports(jack_backend->jack_client, 0, 0,
                JackPortIsPhysical|JackPortIsInput);

    if (ports != 0)
    {
        port_id = 0;
        while ((ports[port_id] != 0) && (port_id != jack_backend->nb_channels))
        {
            ret = jack_connect(jack_backend->jack_client, jack_port_name(jack_backend->ports[port_id]), ports[port_id]);
            if (ret)
            {
                logger_log(LOG_WARNING, "%s: could not autoconnect channel %d", __func__, port_id);
            }
            else
            {
                logger_log(LOG_DEBUG, "%s: channel %d autoconnected", __func__, port_id);
            }
            ++port_id;
        }

        jack_free(ports);
    }
    else
    {
        logger_log(LOG_WARNING, "%s: could not autoconnect channels", __func__);
    }

    return ret;
}

int jack_backend_init(audio_backend_handle_t* handle)
{
    struct jack_backend_t* jack_backend = 0;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: null handle pointer", __func__);
        return -EINVAL;
    }

    jack_backend = calloc(1, sizeof(struct jack_backend_t));
    if (jack_backend == 0)
    {
        logger_log(LOG_FATAL, "%s: could not allocate memory", __func__);
        return -ENOMEM;
    }

    jack_backend->parent.is_fmt_supported   = jack_is_fmt_supported;
    jack_backend->parent.open               = jack_open;
    jack_backend->parent.close              = jack_close;
    jack_backend->parent.write              = jack_write;

    *handle = (audio_backend_handle_t)jack_backend;

    return 0;
}

int jack_is_fmt_supported(audio_backend_handle_t handle, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate)
{
    int jack_sample_rate;
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_ERROR, "%s: handle is null", __func__);
        return -EINVAL;
    }

    if (jack_backend->jack_client == 0)
    {
        jack_backend->jack_client = jack_client_open("vban_receptor", 0, 0);
        if (jack_backend->jack_client == 0)
        {
            logger_log(LOG_ERROR, "%s: could not open jack client", __func__);
            return -ENODEV;
        }
    }

    jack_sample_rate = jack_get_sample_rate(jack_backend->jack_client);
    if (jack_sample_rate != rate)
    {
        logger_log(LOG_ERROR, "%s: jack server sample rate and requested sample rate differs.", __func__);
        return 0;
    }

    // we don't support 64 bit float
    return (bit_resolution != VBAN_BITFMT_64_FLOAT);
}

int jack_open(audio_backend_handle_t handle, char const* output_name, enum VBanBitResolution bit_resolution, unsigned int nb_channels, unsigned int rate, size_t buffer_size)
{
    int ret;
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)handle;
    jack_nframes_t jack_buffer_size;

    logger_log(LOG_DEBUG, "%s", __func__);

    if (handle == 0)
    {
        logger_log(LOG_ERROR, "%s: handle is null", __func__);
        return -EINVAL;
    }

    /*XXX could be used to select jack server name*/
    (void)output_name;

    if (jack_backend->jack_client == 0)
    {
        jack_backend->jack_client = jack_client_open("vban_receptor", 0, 0);
        if (jack_backend->jack_client == 0)
        {
            logger_log(LOG_ERROR, "%s: could not open jack client", __func__);
            return -ENODEV;
        }
    }

    jack_buffer_size = jack_get_buffer_size(jack_backend->jack_client);
    jack_backend->buffer_size = jack_buffer_size;
    if (buffer_size > jack_buffer_size)
    {
        jack_backend->buffer_size = buffer_size;
    }

    //XXX why the hell do I need such a huge buffer oversizing to make it work ?
    // the latency is not bad (it is supposed to be 2 jack frames, as we activate the
    // client when we reach trigger_ptr) but woah, what is going on ?
    jack_backend->buffer_size *= 10;
    jack_backend->buffer = calloc(1, jack_backend->buffer_size);
    if (jack_backend->buffer == 0)
    {
        logger_log(LOG_ERROR, "%s: memory allocation failed", __func__);
        return -ENOMEM;
    }

    logger_log(LOG_INFO, "%s: jack bufsize %lu, vban bufsize %lu, backend bufsize %lu", __func__,
        jack_buffer_size, buffer_size, jack_backend->buffer_size);

    jack_backend->rd_ptr            = jack_backend->buffer;
    jack_backend->wr_ptr            = jack_backend->buffer;
    jack_backend->trigger_ptr       = jack_backend->buffer + (jack_buffer_size * 2);
    jack_backend->nb_channels       = nb_channels;
    jack_backend->bit_resolution    = bit_resolution;

    ret = jack_set_process_callback(jack_backend->jack_client, jack_process_cb, jack_backend);
    if (ret)
    {
        logger_log(LOG_ERROR, "%s: impossible to set jack process callback", __func__);
        jack_close(handle);
        return ret;
    }

    jack_on_shutdown(jack_backend->jack_client, jack_shutdown_cb, jack_backend);

//    ret = jack_start(jack_backend);

    return ret;
}

int jack_close(audio_backend_handle_t handle)
{
    int ret = 0;
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)handle;

    if (handle == 0)
    {
        logger_log(LOG_FATAL, "%s: handle pointer is null", __func__);
        return -EINVAL;
    }

    if (jack_backend->jack_client == 0)
    {
        /** nothing to do */
        return 0;
    }

    ret = jack_deactivate(jack_backend->jack_client);
    if (ret)
    {
        logger_log(LOG_FATAL, "%s: jack_deactivate failed with error %d", __func__);
    }

    ret = jack_client_close(jack_backend->jack_client);
    if (ret)
    {
        logger_log(LOG_FATAL, "%s: jack_client_close failed with error %d", __func__);
    }

    free(jack_backend->buffer);
    jack_backend->jack_client = 0;
    memset(jack_backend->ports, 0, VBAN_CHANNELS_MAX_NB);

    return ret;
}

int jack_write(audio_backend_handle_t handle, char const* data, size_t nb_sample)
{
    int ret = 0;
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)handle;
    size_t incoming_size = 0;
    ptrdiff_t xtra_size = 0;

    if ((handle == 0) || (data == 0))
    {
        logger_log(LOG_ERROR, "%s: handle or data pointer is null", __func__);
        return -EINVAL;
    }

    if (jack_backend->jack_client == 0)
    {
        logger_log(LOG_ERROR, "%s: device not open", __func__);
        return -ENODEV;
    }

    incoming_size = (VBanBitResolutionSize[jack_backend->bit_resolution] * jack_backend->nb_channels * nb_sample);
    xtra_size = (jack_backend->wr_ptr + incoming_size) - (jack_backend->buffer + jack_backend->buffer_size);
    if (xtra_size > 0)
    {
        memcpy(jack_backend->wr_ptr, data, incoming_size - xtra_size);
        jack_backend->wr_ptr = jack_backend->buffer;
        memcpy(jack_backend->wr_ptr, data + (incoming_size - xtra_size), xtra_size);
        jack_backend->wr_ptr += xtra_size;
    }
    else
    {
        memcpy(jack_backend->wr_ptr, data, incoming_size);
        jack_backend->wr_ptr += incoming_size;
    }

    if (!jack_backend->active && (jack_backend->wr_ptr > jack_backend->trigger_ptr))
    {
        ret = jack_start(jack_backend);
    }

    return ret;
}

int jack_process_cb(jack_nframes_t nframes, void* arg)
{
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)arg;
    size_t channel;
    size_t sample;
    static jack_default_audio_sample_t* buffers[VBAN_CHANNELS_MAX_NB];

    if (arg == 0)
    {
        logger_log(LOG_ERROR, "%s: handle pointer is null", __func__);
        return -EINVAL;
    }

    for (channel = 0; channel != jack_backend->nb_channels; ++channel)
    {
        buffers[channel] = (jack_default_audio_sample_t*)jack_port_get_buffer(jack_backend->ports[channel], nframes);
    }

    for (sample = 0; sample != nframes; ++sample)
    {
        for (channel = 0; channel != jack_backend->nb_channels; ++channel)
        {
            buffers[channel][sample] = jack_convert_sample(jack_backend->rd_ptr, jack_backend->bit_resolution);
            jack_backend->rd_ptr += VBanBitResolutionSize[jack_backend->bit_resolution];
            if (jack_backend->rd_ptr >= (jack_backend->buffer + jack_backend->buffer_size))
            {
                jack_backend->rd_ptr = jack_backend->buffer;
            }
        }

    }

    return 0;
}

void jack_shutdown_cb(void* arg)
{
    audio_backend_handle_t const jack_backend = (audio_backend_handle_t)arg;

    if (arg == 0)
    {
        logger_log(LOG_ERROR, "%s: handle pointer is null");
        return ;
    }

    jack_close(jack_backend);

    /*XXX how to notify upper layer that we are done ?*/
}

inline jack_default_audio_sample_t jack_convert_sample(char const* ptr, enum VBanBitResolution bit_resolution)
{
    int value;

    switch (bit_resolution)
    {
        case VBAN_BITFMT_8_INT:
            return (float)*((int8_t const*)ptr);

        case VBAN_BITFMT_16_INT:
            return (float)(*((int16_t const*)ptr)) / 32768;

        case VBAN_BITFMT_24_INT:
            memcpy(&value, ptr, 3);
            return (float)value;

        case VBAN_BITFMT_32_INT:
            return (float)*((int32_t const*)ptr);

        case VBAN_BITFMT_32_FLOAT:
            return *(float const*)ptr;

        case VBAN_BITFMT_64_FLOAT:
        default:
            return 0.0;
    }
}
