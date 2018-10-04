/*
 *  This file is part of vban_emitter.
 *  Copyright (c) 2018 by Benoît Quiniou <quiniouben@yahoo.fr>
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
#include "vban/vban.h"
#include "common/version.h"
#include "common/socket.h"
#include "common/logger.h"

struct config_t
{
    struct socket_config_t      socket;
    char                        stream_name[VBAN_STREAM_NAME_SIZE];
    int                         bps;
    int                         ident;
    int                         format;
};

struct main_t
{
    socket_handle_t             socket;
    char                        buffer[VBAN_PROTOCOL_MAX_SIZE];
};

void usage()
{
    printf("\nUsage: vban_sendtext [OPTIONS] MESSAGE\n\n");
    printf("-i, --ipaddress=IP      : MANDATORY. ipaddress to send stream to\n");
    printf("-p, --port=PORT         : MANDATORY. port to use\n");
    printf("-s, --streamname=NAME   : MANDATORY. streamname to use\n");
    printf("-b, --bps=VALUE         : Data bitrate indicator. default 0 (no special bitrate)\n");
    //XXX give allowed value

    printf("-n, --ident=VALUE       : Subchannel identification. default 0\n");
    printf("-f, --format=VALUE      : Text format used. can be: 0 (ASCII), 1 (UTF8), 2 (WCHAR), 240 (USER). default 1\n");
    printf("-l, --loglevel=LEVEL    : Log level, from 0 (FATAL) to 4 (DEBUG). default is 1 (ERROR)\n");
    printf("-h, --help              : display this message\n\n");
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
        {"bps",         required_argument,  0, 'b'},
        {"ident",       required_argument,  0, 'n'},
        {"format",      required_argument,  0, 'f'},
        {"loglevel",    required_argument,  0, 'l'},
        {"help",        no_argument,        0, 'h'},
        {0,             0,                  0,  0 }
    };

    // default values
    config->bps     = 0;
    config->ident   = 0;
    config->format  = 1;
    config->socket.direction    = SOCKET_OUT;

    /* yes, I assume config is not 0 */
    while (1)
    {
        c = getopt_long(argc, argv, "i:p:s:b:n:f:l:h", options, 0);
        if (c == -1)
            break;

        switch (c)
        {
            case 'i':
                strncpy(config->socket.ip_address, optarg, SOCKET_IP_ADDRESS_SIZE-1);
                break;

            case 'p':
                config->socket.port = atoi(optarg);
                break;

            case 's':
                strncpy(config->stream_name, optarg, VBAN_STREAM_NAME_SIZE-1);
                break;

            case 'b':
                config->bps = atoi(optarg);
                break;

            case 'n':
                config->ident = atoi(optarg);
                break;

            case 'f':
                config->format = atoi(optarg);
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

    /** check if we got all arguments */
    if ((config->socket.ip_address[0] == 0)
        || (config->socket.port == 0)
        || (config->stream_name[0] == 0))
    {
        logger_log(LOG_FATAL, "Missing ip address, port or stream name");
        usage();
        return 1;
    }

    if (optind == argc)
    {
        logger_log(LOG_FATAL, "Missing message argument");
        usage();
        return 1;
    }
    else if (optind < argc - 1)
    {
        logger_log(LOG_FATAL, "Too many arguments");
        usage();
        return 1;
    }

    return 0;
}


int main(int argc, char* const* argv)
{
    int ret = 0;
    struct config_t config;
    struct main_t   main_s;
    char const* msg = 0;
    size_t len = 0;
    struct VBanHeader* const hdr = (struct VBanHeader*)&main_s.buffer;

    printf("%s version %s\n\n", argv[0], VBAN_VERSION);

    memset(&config, 0, sizeof(struct config_t));
    memset(&main_s, 0, sizeof(struct main_t));

    ret = get_options(&config, argc, argv);
    if (ret != 0)
    {
        return ret;
    }

    msg = argv[argc-1];
    len = strlen(msg);
    if (len > VBAN_DATA_MAX_SIZE-1)
    {
        logger_log(LOG_FATAL, "Message too long. max lenght is %d", VBAN_DATA_MAX_SIZE-1);
        usage();
        return 1;
    }

    strncpy((char*)&main_s.buffer + sizeof(struct VBanHeader), msg, VBAN_DATA_MAX_SIZE-1);

    ret = socket_init(&main_s.socket, &config.socket);
    if (ret != 0)
    {
        return ret;
    }

    hdr->vban       = VBAN_HEADER_FOURC;
    hdr->format_SR  = config.bps | VBAN_PROTOCOL_TXT;
    hdr->format_nbs = 0;
    hdr->format_nbc = config.ident;
    hdr->format_bit = config.format;
    strncpy(hdr->streamname, config.stream_name, VBAN_STREAM_NAME_SIZE);
    hdr->nuFrame    = 0;

    logger_log(LOG_DEBUG, "%s: packet is vban: %u, sr: %d, nbs: %d, nbc: %d, bit: %d, name: %s, nu: %u, msg: %s",
        __func__, hdr->vban, hdr->format_SR, hdr->format_nbs, hdr->format_nbc, hdr->format_bit, hdr->streamname, hdr->nuFrame, (char*)&main_s.buffer + sizeof(struct VBanHeader));

    ret = socket_write(main_s.socket, main_s.buffer, len + sizeof(struct VBanHeader));

    socket_release(&main_s.socket);

    return ret;
}

