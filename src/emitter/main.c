/*
 *  This file is part of vban_source.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
 *
 *  vban_source is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  vban_source is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vban_source.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include "vban/vban.h"
#include "net/socket.h"
#include "audio/audio.h"
#include "audio/audio_backend.h"
#include "util/logger.h"

#define VBAN_SOURCE_VERSION   "v0.8.6"
#define MAIN_IP_ADDRESS_SIZE    32

struct config_t
{
    char                    ip_address[MAIN_IP_ADDRESS_SIZE];
    unsigned short          port;
    char                    stream_name[VBAN_STREAM_NAME_SIZE];
    unsigned char           quality;
    char                    audio_backend_type[AUDIO_BACKEND_NAME_SIZE];
    char                    audio_output_name[AUDIO_OUTPUT_NAME_SIZE];
    int                     audio_channels_nb;
    int                     audio_sample_rate;
    int                     audio_bit_format;
};

struct main_t
{
    socket_handle_t         socket;
    audio_handle_t          audio;
    struct config_t         config;
    char                    buffer[VBAN_PROTOCOL_MAX_SIZE];
};

static int MainRun = 1;
void signalHandler(int signum)
{
    MainRun = 0;
}

void usage()
{
    printf("\nUsage: vban_source [OPTIONS]...\n\n");
    printf("-i, --ipaddress=IP      : MANDATORY. ipaddress to send stream to\n");
    printf("-p, --port=PORT         : MANDATORY. port to use\n");
    printf("-s, --streamname=NAME   : MANDATORY. streamname to use\n");
    printf("-b, --backend=TYPE      : audio backend to use. possible values: alsa, pulseaudio, jack and pipe (EXPERIMENTAL). default is first in this list that is actually compiled\n");
    printf("-a, --audiodevice=NAME  : Audio device (server for jack backend) name , (as given by \"arecord -L\" output for alsa). using backend's default by default. not used for jack or pipe\n");
    printf("-r, --samplerate=VALUE  : Audio device sample rate. default 44100\n");
    printf("-n, --nbchannels=VALUE  : Audio device number of channels. default 2\n");
    printf("-f, --bitformat=VALUE   : Audio device sample format. default 16bits\n");

    printf("-d, --debug=LEVEL       : Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)\n");
    printf("-h, --help              : display this message\n\n");
}

int get_options(struct config_t* config, int argc, char* const* argv)
{
    int c = 0;

    static const struct option options[] =
    {
        {"ipaddress",   required_argument,  0, 'i'},
        {"port",        required_argument,  0, 'p'},
        {"streamname",  required_argument,  0, 's'},
        {"backend",     required_argument,  0, 'b'},
        {"audiodevice", required_argument,  0, 'a'},
        {"samplerate",  required_argument,  0, 'r'},
        {"nbchannels",  required_argument,  0, 'n'},
        {"bitformat",   required_argument,  0, 'f'},
        {"debug",       required_argument,  0, 'd'},
        {"help",        no_argument,        0, 'h'},
        {0,             0,                  0,  0 }
    };

    // default values
    config->audio_channels_nb   = 2;
    config->audio_sample_rate   = 44100;
    config->audio_bit_format    = 16;

    /* yes, I assume config is not 0 */
    while (1)
    {
        c = getopt_long(argc, argv, "i:p:s:b:a:r:n:f:d:h", options, 0);
        if (c == -1)
            break;

        switch (c)
        {
            case 'i':
                strncpy(config->ip_address, optarg, MAIN_IP_ADDRESS_SIZE);
                break;

            case 'p':
                config->port = atoi(optarg);
                break;

            case 's':
                strncpy(config->stream_name, optarg, VBAN_STREAM_NAME_SIZE);
                break;

            case 'b':
                strncpy(config->audio_backend_type, optarg, AUDIO_BACKEND_NAME_SIZE);

            case 'a':
                strncpy(config->audio_output_name, optarg, AUDIO_OUTPUT_NAME_SIZE);
                break;

            case 'r':
                config->audio_sample_rate = atoi(optarg);
                break;

            case 'n':
                config->audio_channels_nb = atoi(optarg);
                break;

            case 'f':
                config->audio_bit_format = atoi(optarg);
                break;

            case 'd':
                logger_set_output_level(atoi(optarg));
                break;

            case 'h':
            default:
                usage();
                return 1;
        }
    }

    /** check if we got all arguments */
    if ((config->ip_address[0] == 0)
        || (config->port == 0)
        || (config->stream_name[0] == 0))
    {
        logger_log(LOG_FATAL, "Missing ip address, port or stream name");
        usage();
        return 1;
    }

    return 0;
}

/** This is where all VBan specific handling is done */
/** we need to cut the packets correctly */
int process_packet(struct main_t* main_s, int size)
{
    struct VBanHeader* const hdr = (struct VBanHeader*)main_s->buffer;
    hdr->vban       = VBAN_HEADER_FOURC;
    hdr->format_SR  = main_s->config.audio_sample_rate;
    hdr->format_nbs = size /*XXX*/;
    hdr->format_nbc = main_s->config.audio_channels_nb;
    hdr->format_bit = main_s->config.audio_bit_format;
    strncpy(hdr->streamname, main_s->config.stream_name, VBAN_STREAM_NAME_SIZE);
    ++hdr->nuFrame;

    return socket_sendto(main_s->socket, main_s->buffer, size /*XXX*/, main_s->config.ip_address);
}

int main(int argc, char* const* argv)
{
    int ret = 0;
    struct main_t main_s;

    printf("%s version %s\n\n", argv[0], VBAN_SOURCE_VERSION);

    memset(&main_s, 0, sizeof(struct main_t));
    if (get_options(&main_s.config, argc, argv))
    {
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    ret = socket_init(&main_s.socket);
    if (ret != 0)
    {
        return ret;
    }

    ret = audio_init(&main_s.audio, main_s.config.audio_backend_type, (char const*)main_s.config.audio_output_name, main_s.config.quality);
    if (ret != 0)
    {
        return ret;
    }

    ret = audio_open(main_s.audio, main_s.config.audio_bit_format, main_s.config.audio_channels_nb, main_s.config.audio_sample_rate);
    if (ret != 0)
    {
        return ret;
    }

    ret = socket_open(main_s.socket, main_s.config.port, 1);

    while ((ret >= 0) && MainRun)
    {
        //XXX I chose nb_sample and not size, is it that smart ?
        ret = audio_read(main_s.audio, (char*)main_s.buffer + VBAN_HEADER_SIZE, VBAN_DATA_MAX_SIZE); 
        if (ret > 0)
        {
            ret = process_packet(&main_s, ret);
        }
    }

    audio_release(&main_s.audio);
    socket_release(&main_s.socket);

    return 0;
}

