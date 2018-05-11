/*
 *  This file is part of vban.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
 *
 *  vban is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  vban is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vban.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include "vban/vban.h"
#include "common/socket.h"
#include "common/audio.h"
#include "common/logger.h"
#include "common/packet.h"
#include "common/version.h"
#include "common/backend/audio_backend.h"

struct config_t
{
    struct socket_config_t      socket;
    struct audio_config_t       audio;
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
    printf("\nUsage: vban_receptor [OPTIONS]...\n\n");
    printf("-i, --ipaddress=IP      : MANDATORY. ipaddress to get stream from\n");
    printf("-p, --port=PORT         : MANDATORY. port to listen to\n");
    printf("-s, --streamname=NAME   : MANDATORY. streamname to play\n");
    printf("-b, --backend=TYPE      : audio backend to use. %s\n", audio_backend_get_help());
    printf("-q, --quality=ID        : network quality indicator from 0 (low latency) to 4. default is 1\n");
    printf("-c, --channels=LIST     : channels from the stream to use. LIST is of form x,y,z,... default is to forward the stream as it is\n");
    printf("-o, --output=NAME       : DEPRECATED. please use -d\n");
    printf("-d, --device=NAME       : Audio device name. This is file name for file backend, server name for jack backend, device for alsa, stream_name for pulseaudio.\n");
    printf("-l, --loglevel=LEVEL    : Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)\n");
    printf("-h, --help              : display this message\n\n");
}

static size_t computeSize(unsigned char quality)
{
    size_t const nmin = VBAN_PROTOCOL_MAX_SIZE;
    size_t nnn = 0;

    switch(quality)
    {
        case 0:
            nnn = 512;
            break;

        case 1:
            nnn = 1024;
            break;

        case 2:
            nnn = 2048;
            break;

        case 3:
            nnn = 4096;
            break;

        case 4:
            nnn = 8192;
            break;

        default:
            break;
    }

    nnn=nnn*3;

    if (nnn < nmin)
    {
        nnn = nmin;
    }

    return nnn;
}

int get_options(struct config_t* config, int argc, char* const* argv)
{
    int c = 0;
    int quality = 1;
    int ret = 0;

    static const struct option options[] =
    {
        {"ipaddress",   required_argument,  0, 'i'},
        {"port",        required_argument,  0, 'p'},
        {"streamname",  required_argument,  0, 's'},
        {"backend",     required_argument,  0, 'b'},
        {"quality",     required_argument,  0, 'q'},
        {"channels",    required_argument,  0, 'c'},
        {"output",      required_argument,  0, 'o'},
        {"device",      required_argument,  0, 'd'},
        {"loglevel",    required_argument,  0, 'l'},
        {"help",        no_argument,        0, 'h'},
        {0,             0,                  0,  0 }
    };

    /* yes, I assume config is not 0 */
    while (1)
    {
        c = getopt_long(argc, argv, "i:p:s:b:q:c:o:d:l:h", options, 0);
        if (c == -1)
            break;

        switch (c)
        {
            case 'i':
                strncpy(config->socket.ip_address, optarg, SOCKET_IP_ADDRESS_SIZE -1);
                break;

            case 'p':
                config->socket.port = atoi(optarg);
                break;

            case 's':
                strncpy(config->stream_name, optarg, VBAN_STREAM_NAME_SIZE -1);
                break;

            case 'b':
                strncpy(config->audio.backend_name, optarg, AUDIO_BACKEND_NAME_SIZE-1);
                break;

            case 'q':
                quality = atoi(optarg);
                break;

            case 'c':
                ret = audio_parse_map_config(&config->map, optarg);
                break;

            case 'o':
            case 'd':
                strncpy(config->audio.device_name, optarg, AUDIO_DEVICE_NAME_SIZE-1);
                break;

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

    config->audio.direction     = AUDIO_OUT;
    config->audio.buffer_size   = computeSize(quality);
    config->socket.direction    = SOCKET_IN;

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

    printf("vban_receptor version %s\n\n", VBAN_VERSION);

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

    while (MainRun)
    {
        size = socket_read(main_s.socket, main_s.buffer, VBAN_PROTOCOL_MAX_SIZE);
        if (size < 0)
        {
            MainRun = 0;
            break;
        }

        if (packet_check(config.stream_name, main_s.buffer, size) == 0)
        {
            packet_get_stream_config(main_s.buffer, &stream_config);

            ret = audio_set_stream_config(main_s.audio, &stream_config);
            if (ret < 0)
            {
                MainRun = 0;
                break;
            }

            ret = audio_write(main_s.audio, PACKET_PAYLOAD_PTR(main_s.buffer), PACKET_PAYLOAD_SIZE(size));
            if (ret != PACKET_PAYLOAD_SIZE(size))
            {
                logger_log(LOG_WARNING, "%s: wrote %d bytes, expected %d bytes", __func__, ret, PACKET_PAYLOAD_SIZE(size));
            }
            else if (ret < 0)
            {
                MainRun = 0;
                break;
            }
        }
    }

    audio_release(&main_s.audio);
    socket_release(&main_s.socket);

    return 0;
}

