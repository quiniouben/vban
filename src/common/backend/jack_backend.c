#include "jack_backend.h"
#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "common/logger.h"

#define NB_BUFFERS  2

struct jack_backend_t
{
    struct audio_backend_t  parent;
    jack_client_t*          jack_client;
    jack_port_t*            ports[VBAN_CHANNELS_MAX_NB];
    jack_ringbuffer_t*      ring_buffer;
    size_t                  buffer_size;
    enum VBanBitResolution  bit_fmt;
    unsigned int            nb_channels;
    int                     active;
};

static int jack_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config);
static int jack_close(audio_backend_handle_t handle);
static int jack_write(audio_backend_handle_t handle, char const* data, size_t nb_sample);

static int jack_process_cb(jack_nframes_t nframes, void* arg);
static void jack_shutdown_cb(void* arg);
static jack_default_audio_sample_t jack_convert_sample(char const* ptr, enum VBanBitResolution bit_fmt);
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

    jack_backend->parent.open               = jack_open;
    jack_backend->parent.close              = jack_close;
    jack_backend->parent.write              = jack_write;

    *handle = (audio_backend_handle_t)jack_backend;

    return 0;
}

int jack_open(audio_backend_handle_t handle, char const* output_name, enum audio_direction direction, size_t buffer_size, struct stream_config_t const* config)
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

    if (jack_backend->jack_client == 0)
    {
        jack_backend->jack_client = jack_client_open((output_name[0] == '\0') ? "vban" : output_name, 0, 0);
        if (jack_backend->jack_client == 0)
        {
            logger_log(LOG_ERROR, "%s: could not open jack client", __func__);
            return -ENODEV;
        }
    }

    jack_buffer_size            = jack_get_buffer_size(jack_backend->jack_client) * jack_backend->nb_channels * VBanBitResolutionSize[config->bit_fmt];
    buffer_size                 = ((buffer_size > jack_buffer_size) ? buffer_size : jack_buffer_size) * NB_BUFFERS;
    jack_backend->nb_channels   = config->nb_channels;
    jack_backend->bit_fmt= config->bit_fmt;

    jack_backend->ring_buffer   = jack_ringbuffer_create(buffer_size);

    char* const zeros = calloc(1, buffer_size / NB_BUFFERS);
    jack_ringbuffer_write(jack_backend->ring_buffer, zeros, buffer_size / NB_BUFFERS);
    free(zeros);

    ret = jack_set_process_callback(jack_backend->jack_client, jack_process_cb, jack_backend);
    if (ret)
    {
        logger_log(LOG_ERROR, "%s: impossible to set jack process callback", __func__);
        jack_close(handle);
        return ret;
    }

    jack_on_shutdown(jack_backend->jack_client, jack_shutdown_cb, jack_backend);

    ret = jack_start(jack_backend);

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

    jack_backend->active = 0;

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

    if (jack_backend->ring_buffer != 0)
    {
        jack_ringbuffer_free(jack_backend->ring_buffer);
    }

    jack_backend->jack_client = 0;
    memset(jack_backend->ports, 0, VBAN_CHANNELS_MAX_NB * sizeof(jack_port_t*));

    return ret;
}

int jack_write(audio_backend_handle_t handle, char const* data, size_t size)
{
    int ret = 0;
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)handle;

    logger_log(LOG_DEBUG, "%s", __func__);

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

    if (jack_backend->active == 0)
    {
        logger_log(LOG_DEBUG, "%s: server not active yet", __func__);
        return size;
    }

    if (jack_ringbuffer_write_space(jack_backend->ring_buffer) < size)
    {
        logger_log(LOG_WARNING, "%s: short write", __func__);
        return 0;
    }
    
    jack_ringbuffer_write(jack_backend->ring_buffer, data, size);

    return (ret < 0) ? ret : size;
}

int jack_process_cb(jack_nframes_t nframes, void* arg)
{
    struct jack_backend_t* const jack_backend = (struct jack_backend_t*)arg;
    size_t channel;
    size_t sample;
    jack_ringbuffer_data_t rb_data[2];
    static jack_default_audio_sample_t* buffers[VBAN_CHANNELS_MAX_NB];
    char const* ptr;
    size_t len;
    size_t sampleSize;
    size_t index = 0;
    size_t part;
    char sampleParts[8];

    if (arg == 0)
    {
        logger_log(LOG_ERROR, "%s: handle pointer is null", __func__);
        return -EINVAL;
    }

    logger_log(LOG_DEBUG, "%s", __func__);

    sampleSize = VBanBitResolutionSize[jack_backend->bit_fmt];
    jack_backend->active = 1;

    for (channel = 0; channel != jack_backend->nb_channels; ++channel)
    {
        buffers[channel] = (jack_default_audio_sample_t*)jack_port_get_buffer(jack_backend->ports[channel], nframes);
    }

    jack_ringbuffer_get_read_vector(jack_backend->ring_buffer, rb_data);

    if ((rb_data[0].len + rb_data[1].len) < (nframes * jack_backend->nb_channels * sampleSize))
    {
        logger_log(LOG_WARNING, "%s: short read", __func__);
        return 0;
    }

    ptr = rb_data[0].buf;
    len = 0;
    for (sample = 0; sample != nframes; ++sample)
    {
        for (channel = 0; channel != jack_backend->nb_channels; ++channel)
        {
            if (index == 0)
            {
                if ((len + sampleSize) > rb_data[0].len)
                {
                    part = 0;
                    while (ptr != (rb_data[0].buf + rb_data[0].len))
                    {
                        sampleParts[part++] = *(ptr++);
                    }
                    ptr = rb_data[1].buf;
                    for (; part < VBanBitResolutionSize[jack_backend->bit_fmt]; ++part, ++ptr)
                    {
                        sampleParts[part] = *ptr;
                    }

                    buffers[channel][sample] = jack_convert_sample((char const*)sampleParts, jack_backend->bit_fmt);
                    len += sampleSize;
                    index = 1;
                    continue;
                }
                if (len == rb_data[0].len)
                {
                    ptr = rb_data[1].buf;
                    index = 1;
                }
            }

            buffers[channel][sample] = jack_convert_sample(ptr, jack_backend->bit_fmt);
            ptr += sampleSize;
            len += sampleSize;
        }
    }

    jack_ringbuffer_read_advance(jack_backend->ring_buffer, len);

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

jack_default_audio_sample_t jack_convert_sample(char const* ptr, enum VBanBitResolution bit_fmt)
{
    int value;

    switch (bit_fmt)
    {
        case VBAN_BITFMT_8_INT:
            return (float)(*((int8_t const*)ptr)) / (float)(1 << 7);

        case VBAN_BITFMT_16_INT:
            return (float)(*((int16_t const*)ptr)) / (float)(1 << 15);

        case VBAN_BITFMT_24_INT:
            value = (ptr[2] << 16) | ((unsigned char)ptr[1] << 8) | (unsigned char)ptr[0];
            return (float)value / (float)(1 << 23);

        case VBAN_BITFMT_32_INT:
            return (float)*((int32_t const*)ptr) / (float)(1 << 31);

        case VBAN_BITFMT_32_FLOAT:
            return *(float const*)ptr;

        case VBAN_BITFMT_64_FLOAT:
        default:
            return 0.0;
    }
}
