/*
 *  This file is part of vban_emitter.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
 *
 *  vban_emitter is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  vban_emitter is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vban_emitter.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include "common/version.h"
#include "common/socket.h"
#include "common/audio.h"
#include "common/logger.h"
#include "common/packet.h"
#include "common/backend/audio_backend.h"

struct config_t
{
    struct socket_config_t      socket;
    struct audio_config_t       audio;
    struct stream_config_t      stream;
    struct audio_map_config_t   map;
    char                        stream_name[VBAN_STREAM_NAME_SIZE];
};

struct main_t
{
    socket_handle_t             socket;
    audio_handle_t              audio;
    char                        buffer[VBAN_PROTOCOL_MAX_SIZE];
};

static int MainRun = 1;
void signalHandler(int signum)
{
    MainRun = 0;
}

void usage()
{
    printf("\nUsage: vban_emitter [OPTIONS]...\n\n");
    printf("-i, --ipaddress=IP      : MANDATORY. ipaddress to send stream to\n");
    printf("-p, --port=PORT         : MANDATORY. port to use\n");
    printf("-s, --streamname=NAME   : MANDATORY. streamname to use\n");
//XXX    printf("-b, --backend=TYPE      : audio backend to use. %s\n", audio_backend_get_help());
    printf("-b, --backend=TYPE      : TEMPORARY DISABLED. audio backend to use. Only alsa backend is working at this time\n");
    printf("-d, --device=NAME       : Audio device name. This is file name for file backend, server name for jack backend, device for alsa, stream_name for pulseaudio.\n");
    printf("-r, --rate=VALUE        : Audio device sample rate. default 44100\n");
    printf("-n, --nbchannels=VALUE  : Audio device number of channels. default 2\n");
    printf("-f, --format=VALUE      : Audio device sample format (see below). default is 16I (16bits integer)\n");
//XXX    printf("-c, --channels=LIST     : channels from the audio device to use. LIST is of form x,y,z,... default is to forward the stream as it is\n");
    printf("-c, --channels=LIST     : TEMPORARY DISABLED.\n");

    printf("-l, --loglevel=LEVEL    : Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)\n");
    printf("-h, --help              : display this message\n\n");
    printf("%s\n\n", stream_bit_fmt_help());
}

int get_options(struct config_t* config, int argc, char* const* argv)
{
    int c = 0;
    int ret = 0;

    static const struct option options[] =
    {
        {"ipaddress",   required_argument,  0, 'i'},
        {"port",        required_argument,  0, 'p'},
        {"streamname",  required_argument,  0, 's'},
        {"backend",     required_argument,  0, 'b'},
        {"device",      required_argument,  0, 'd'},
        {"rate",        required_argument,  0, 'r'},
        {"nbchannels",  required_argument,  0, 'n'},
        {"format",      required_argument,  0, 'f'},
        {"channels",    required_argument,  0, 'c'},
        {"loglevel",    required_argument,  0, 'l'},
        {"help",        no_argument,        0, 'h'},
        {0,             0,                  0,  0 }
    };

    // default values
    config->stream.nb_channels  = 2;
    config->stream.sample_rate  = 44100;
    config->stream.bit_fmt      = VBAN_BITFMT_16_INT;
    config->audio.buffer_size   = 1024; /*XXX Why ?*/

    config->socket.direction    = SOCKET_OUT;

    /* yes, I assume config is not 0 */
    while (1)
    {
        c = getopt_long(argc, argv, "i:p:s:b:d:r:n:f:c:l:h", options, 0);
        if (c == -1)
            break;

        switch (c)
        {
            case 'i':
                strncpy(config->socket.ip_address, optarg, SOCKET_IP_ADDRESS_SIZE);
                break;

            case 'p':
                config->socket.port = atoi(optarg);
                break;

            case 's':
                strncpy(config->stream_name, optarg, VBAN_STREAM_NAME_SIZE);
                break;

/*XXX            case 'b':
                strncpy(config->audio.backend_name, optarg, AUDIO_BACKEND_NAME_SIZE);*/

            case 'd':
                strncpy(config->audio.device_name, optarg, AUDIO_DEVICE_NAME_SIZE);
                break;

            case 'r':
                config->stream.sample_rate = atoi(optarg);
                break;

            case 'n':
                config->stream.nb_channels = atoi(optarg);
                break;

            case 'f':
                config->stream.bit_fmt = stream_parse_bit_fmt(optarg);
                break;

/*XXX            case 'c':
                ret = audio_parse_map_config(&config->map, optarg);
                break;*/

            case 'l':
                logger_set_output_level(atoi(optarg));
                break;

            case 'h':
            default:
                usage();
                return 1;
        }

        if (ret)
        {
            return ret;
        }
    }

    /** check if we got all arguments */
    if ((config->socket.ip_address[0] == 0)
        || (config->socket.port == 0)
        || (config->stream_name[0] == 0))
    {
        logger_log(LOG_FATAL, "Missing ip address, port or stream name");
        usage();
        return 1;
    }

    return 0;
}


int main(int argc, char* const* argv)
{
    int ret = 0;
    int size = 0;
    struct config_t config;
    struct stream_config_t stream_config;
    struct main_t   main_s;
    int max_size = 0;

    printf("%s version %s\n\n", argv[0], VBAN_VERSION);

    memset(&config, 0, sizeof(struct config_t));
    memset(&main_s, 0, sizeof(struct main_t));

    ret = get_options(&config, argc, argv);
    if (ret != 0)
    {
        return ret;
    }

    ret = socket_init(&main_s.socket, &config.socket);
    if (ret != 0)
    {
        return ret;
    }

    ret = audio_init(&main_s.audio, &config.audio);
    if (ret != 0)
    {
        return ret;
    }

    ret = audio_set_map_config(main_s.audio, &config.map);
    if (ret != 0)
    {
        return ret;
    }

    ret = audio_set_stream_config(main_s.audio, &config.stream);
    if (ret != 0)
    {
        return ret;
    }

    audio_get_stream_config(main_s.audio, &stream_config);
    packet_init_header(main_s.buffer, &stream_config, config.stream_name);
    max_size = packet_get_max_payload_size(main_s.buffer);

    while (MainRun)
    {
        size = audio_read(main_s.audio, PACKET_PAYLOAD_PTR(main_s.buffer), max_size);
        if (size < 0)
        {
            MainRun = 0;
            break;
        }

        packet_set_new_content(main_s.buffer, size);
        ret = packet_check(config.stream_name, main_s.buffer, size + sizeof(struct VBanHeader));
        if (ret != 0)
        {
            logger_log(LOG_ERROR, "%s: packet prepared is invalid", __func__);
            break;
        }

        ret = socket_write(main_s.socket, main_s.buffer, size + sizeof(struct VBanHeader));
        if (ret < 0)
        {
            MainRun = 0;
            break;
        }
    }

    audio_release(&main_s.audio);
    socket_release(&main_s.socket);

    return ret;
}

