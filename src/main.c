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
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include "vban.h"
#include "socket.h"
#include "audio.h"

#define VBAN_RECEPTOR_VERSION   "v0.5"
#define MAIN_IP_ADDRESS_SIZE    32

struct config_t
{
    char                    ip_address[MAIN_IP_ADDRESS_SIZE];
    unsigned short          port;
    char                    stream_name[VBAN_STREAM_NAME_SIZE];
    unsigned char           quality;
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
    printf("Usage: vban_receptor [OPTIONS]...\n\n");
    printf("-i, --ipaddress=IP    : ipaddress to get stream from\n");
    printf("-p, --port=PORT       : port to listen to\n");
    printf("-s, --streamname=NAME : streamname to play\n");
    printf("-q, --quality=ID      : network quality indicator from 0 (low latency) to 4. default is 1\n");
    printf("-h, --help            : display this message\n\n");
}

int get_options(struct config_t* config, int argc, char* const* argv)
{
    int c = 0;
    int index = 0;
    static const struct option options[] =
    {
        {"ipaddress",   required_argument,  0, 'i'},
        {"port",        required_argument,  0, 'p'},
        {"streamname",  required_argument,  0, 's'},
        {"quality",     required_argument,  0, 'd'},
        {"help",        no_argument,        0, 'h'}
    };

    /* yes, I assume config is not 0 */
    while (1)
    {
        c = getopt_long(argc, argv, "i:p:s:q:h", options, &index);
        if (c == -1)
            break;

        switch (c)
        {
            case 'i':
                memcpy(config->ip_address, optarg, MAIN_IP_ADDRESS_SIZE);
                break;

            case 'p':
                config->port = atoi(optarg);
                break;

            case 's':
                memcpy(config->stream_name, optarg, VBAN_STREAM_NAME_SIZE);
                break;

            case 'q':
                config->quality = atoi(optarg);
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
        usage();
        return 1;
    }

    return 0;
}

/** This is where all VBan specific handling is done */
int process_packet(struct main_t* s_main, int size, char const* ipfrom)
{
    int ret = 0;
    enum VBanProtocol protocol = VBAN_PROTOCOL_UNDEFINED_4;
    enum VBanCodec codec = VBAN_BIT_RESOLUTION_MAX;

    /* yes, I assume s_main is not 0, neither s_main->buffer */
    struct VBanHeader const* const hdr = (struct VBanHeader const*)s_main->buffer;

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
    if (strcmp(ipfrom, s_main->config.ip_address)
        || strcmp(s_main->config.stream_name, hdr->streamname))
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
                ret = audio_process_packet(s_main->audio, s_main->buffer, size);
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
            printf("process_packet: packet with unknown protocol\n");
            ret = -EINVAL;
            break;
    }

end:
    return (ret < 0) ? ret : size;
}

int main(int argc, char* const* argv)
{
    int ret = 0;
    struct main_t s_main;
    char ipfrom[MAIN_IP_ADDRESS_SIZE];

    printf("vban_receptor version %s\n", VBAN_RECEPTOR_VERSION);

    memset(&s_main, 0, sizeof(struct main_t));
    if (get_options(&s_main.config, argc, argv))
    {
        return 1;
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    ret = socket_init(&s_main.socket);
    if (ret != 0)
    {
        return ret;
    }

    ret = audio_init(&s_main.audio, s_main.config.quality);
    if (ret != 0)
    {
        return ret;
    }

    ret = socket_open(s_main.socket, s_main.config.port, 0);

    while ((ret >= 0) && MainRun)
    {
        ret = socket_recvfrom(s_main.socket, s_main.buffer, VBAN_PROTOCOL_MAX_SIZE, ipfrom);
        if (ret > 0)
        {
            ret = process_packet(&s_main, ret, ipfrom);
        }
    }

    audio_release(&s_main.audio);
    socket_release(&s_main.socket);

    return 0;
}

