/*
 *  This file is part of vban_receptor.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
 *
 *  vban_receptor is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  vban_receptor is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with vban_receptor.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include "vban.h"
#include "socket.h"
#include "audio.h"
#include "logger.h"

#define VBAN_RECEPTOR_VERSION   "v0.8.0"
#define MAIN_IP_ADDRESS_SIZE    32

struct config_t
{
    char                    ip_address[MAIN_IP_ADDRESS_SIZE];
    unsigned short          port;
    char                    stream_name[VBAN_STREAM_NAME_SIZE];
    unsigned char           quality;
    int                     audio_backend_type;
    char                    audio_output_name[AUDIO_OUTPUT_NAME_SIZE];
    unsigned char           audio_channels[VBAN_CHANNELS_MAX_NB];
    int                     audio_channels_nb;
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
    printf("\nUsage: vban_receptor [OPTIONS]...\n\n");
    printf("-i, --ipaddress=IP      : MANDATORY. ipaddress to get stream from\n");
    printf("-p, --port=PORT         : MANDATORY. port to listen to\n");
    printf("-s, --streamname=NAME   : MANDATORY. streamname to play\n");
    printf("-b, --backend=TYPE      : audio backend to use. possible values: alsa and pulseaudio. default is alsa\n");
    printf("-q, --quality=ID        : network quality indicator from 0 (low latency) to 4. default is 1\n");
    printf("-c, --channels=LIST     : channels from the stream to use. LIST is of form x,y,z,... default is to forward the stream as it is\n");
    printf("-o, --output=NAME       : Alsa output device name, as given by \"aplay -L\" output. using backend's default by default\n");
    printf("-d, --debug=LEVEL       : Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)\n");
    printf("-h, --help              : display this message\n\n");
}

void parse_audio_backend_type(int* type, char const* args)
{
    if (!strcmp(args, "alsa"))
    {
        *type = AUDIO_BACKEND_ALSA;
    }
    else if (!strcmp(args, "pulseaudio"))
    {
        *type = AUDIO_BACKEND_PULSEAUDIO;
    }
    else
    {
        *type = -1;
    }
}

int parse_channel_list(unsigned char* channels, char* args)
{
    unsigned int chan = 0;
    size_t index = 0;
    char* token = strtok(args, ",");

    while ((index < VBAN_CHANNELS_MAX_NB) && (token != 0))
    {
        if (sscanf(token, "%u", &chan) == EOF)
            break;

        if ((chan > VBAN_CHANNELS_MAX_NB) || (chan < 1))
        {
            logger_log(LOG_ERROR, "parse_channel_list: invalid channel id %u, stop parsing", chan);
            break;
        }

        channels[index++] = (unsigned char)(chan - 1);
        token = strtok(0, ",");
    }

    return index;
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
        {"quality",     required_argument,  0, 'q'},
        {"channels",    required_argument,  0, 'c'},
        {"output",      required_argument,  0, 'o'},
        {"debug",       required_argument,  0, 'd'},
        {"help",        no_argument,        0, 'h'},
        {0,             0,                  0,  0 }
    };

    /* yes, I assume config is not 0 */
    while (1)
    {
        c = getopt_long(argc, argv, "i:p:s:b:q:c:o:d:h", options, 0);
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
                parse_audio_backend_type(&config->audio_backend_type, optarg);

            case 'q':
                config->quality = atoi(optarg);
                break;

            case 'c':
                config->audio_channels_nb = parse_channel_list(config->audio_channels, optarg);
                break;

            case 'o':
                strncpy(config->audio_output_name, optarg, AUDIO_OUTPUT_NAME_SIZE);
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
int process_packet(struct main_t* main_s, int size, char const* ipfrom)
{
    int ret = 0;
    enum VBanProtocol protocol = VBAN_PROTOCOL_UNDEFINED_4;
    enum VBanCodec codec = VBAN_BIT_RESOLUTION_MAX;

    /* yes, I assume main_s is not 0, neither main_s->buffer */
    struct VBanHeader const* const hdr = (struct VBanHeader const*)main_s->buffer;

    /** check size */
    if (size <= VBAN_HEADER_SIZE)
    {
        goto end;
    }

    /** check header magic bytes */
    if (hdr->vban != VBAN_HEADER_FOURC)
    {
        goto end;
    }

    /** check stream identity */
    if (strcmp(ipfrom, main_s->config.ip_address)
        || strcmp(main_s->config.stream_name, hdr->streamname))
    {
        goto end;
    }

    /** check the reserved bit : it must be 0 */
    if (hdr->format_bit & VBAN_RESERVED_MASK)
    {
        goto end;
    }

    /** check protocol and codec */
    protocol        = hdr->format_SR & VBAN_PROTOCOL_MASK;
    codec           = hdr->format_bit & VBAN_CODEC_MASK;

    switch (protocol)
    {
        case VBAN_PROTOCOL_AUDIO:
            if (codec == VBAN_CODEC_PCM)
            {
                ret = audio_process_packet(main_s->audio, main_s->buffer, size);
            }
            break;

        case VBAN_PROTOCOL_SERIAL:
        case VBAN_PROTOCOL_TXT:
        case VBAN_PROTOCOL_UNDEFINED_1:
        case VBAN_PROTOCOL_UNDEFINED_2:
        case VBAN_PROTOCOL_UNDEFINED_3:
        case VBAN_PROTOCOL_UNDEFINED_4:
            /** not supported yet */
            break;

        default:
            logger_log(LOG_ERROR, "process_packet: packet with unknown protocol");
            ret = -EINVAL;
            break;
    }

end:
    return (ret < 0) ? ret : size;
}

int main(int argc, char* const* argv)
{
    int ret = 0;
    struct main_t main_s;
    char ipfrom[MAIN_IP_ADDRESS_SIZE];

    printf("vban_receptor version %s\n\n", VBAN_RECEPTOR_VERSION);

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

    if (main_s.config.audio_channels_nb != 0)
    {
        ret = audio_set_channels(main_s.audio, main_s.config.audio_channels, main_s.config.audio_channels_nb);
    }

    ret = socket_open(main_s.socket, main_s.config.port, 0);

    while ((ret >= 0) && MainRun)
    {
        ret = socket_recvfrom(main_s.socket, main_s.buffer, VBAN_PROTOCOL_MAX_SIZE, ipfrom);
        if (ret > 0)
        {
            ret = process_packet(&main_s, ret, ipfrom);
        }
    }

    audio_release(&main_s.audio);
    socket_release(&main_s.socket);

    return 0;
}

