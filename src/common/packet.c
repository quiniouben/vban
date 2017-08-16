/*
 *  This file is part of vban.
 *  Copyright (c) 2017 by Benoît Quiniou <quiniouben@yahoo.fr>
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

#include "packet.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "common/logger.h"

static int packet_pcm_check(char const* buffer, size_t size);
static size_t vban_sr_from_value(unsigned int value);

int packet_check(char const* streamname, char const* buffer, size_t size)
{
    struct VBanHeader const* const hdr = PACKET_HEADER_PTR(buffer);
    enum VBanProtocol protocol = VBAN_PROTOCOL_UNDEFINED_4;
    enum VBanCodec codec = VBAN_BIT_RESOLUTION_MAX;

    if ((streamname == 0) || (buffer == 0))
    {
        logger_log(LOG_FATAL, "%s: null pointer argument", __func__);
        return -EINVAL;
    }

    if (size <= VBAN_HEADER_SIZE)
    {
        logger_log(LOG_WARNING, "%s: packet too small", __func__);
        return -EINVAL;
    }

    if (hdr->vban != VBAN_HEADER_FOURC)
    {
        logger_log(LOG_WARNING, "%s: invalid vban magic fourc", __func__);
        return -EINVAL;
    }

    if (strncmp(streamname, hdr->streamname, VBAN_STREAM_NAME_SIZE))
    {
        logger_log(LOG_DEBUG, "%s: different streamname", __func__);
        return -EINVAL;
    }

    /** check the reserved bit : it must be 0 */
    if (hdr->format_bit & VBAN_RESERVED_MASK)
    {
        logger_log(LOG_WARNING, "%s: reserved format bit invalid value", __func__);
        return -EINVAL;
    }

    /** check protocol and codec */
    protocol        = hdr->format_SR & VBAN_PROTOCOL_MASK;
    codec           = hdr->format_bit & VBAN_CODEC_MASK;

    switch (protocol)
    {
        case VBAN_PROTOCOL_AUDIO:
            return (codec == VBAN_CODEC_PCM) ? packet_pcm_check(buffer, size) : -EINVAL;

        case VBAN_PROTOCOL_SERIAL:
        case VBAN_PROTOCOL_TXT:
        case VBAN_PROTOCOL_UNDEFINED_1:
        case VBAN_PROTOCOL_UNDEFINED_2:
        case VBAN_PROTOCOL_UNDEFINED_3:
        case VBAN_PROTOCOL_UNDEFINED_4:
            /** not supported yet */
            return -EINVAL;

        default:
            logger_log(LOG_ERROR, "%s: packet with unknown protocol", __func__);
            return -EINVAL;
    }
}

static int packet_pcm_check(char const* buffer, size_t size)
{
    /** the packet is already a valid vban packet and buffer already checked before */

    struct VBanHeader const* const hdr = PACKET_HEADER_PTR(buffer);
    enum VBanBitResolution const bit_resolution = hdr->format_bit & VBAN_BIT_RESOLUTION_MASK;
    int const sample_rate   = hdr->format_SR & VBAN_SR_MASK;
    int const nb_samples    = hdr->format_nbs + 1;
    int const nb_channels   = hdr->format_nbc + 1;
    size_t sample_size      = 0;
    size_t payload_size     = 0;

    if (bit_resolution >= VBAN_BIT_RESOLUTION_MAX)
    {
        logger_log(LOG_WARNING, "%s: invalid bit resolution", __func__);
        return -EINVAL;
    }

    if (sample_rate >= VBAN_SR_MAXNUMBER)
    {
        logger_log(LOG_WARNING, "%s: invalid sample rate", __func__);
        return -EINVAL;
    }

    sample_size = VBanBitResolutionSize[bit_resolution];
    payload_size = nb_samples * sample_size * nb_channels;

    if (payload_size != (size - VBAN_HEADER_SIZE))
    {
        logger_log(LOG_WARNING, "%s: invalid payload size", __func__);
        return -EINVAL;
    }
    
    return 0;
}

int packet_get_stream_config(char const* buffer, struct stream_config_t* stream_config)
{
    struct VBanHeader const* const hdr = PACKET_HEADER_PTR(buffer);

    memset(stream_config, 0, sizeof(struct stream_config_t));

    if ((buffer == 0) || (stream_config == 0))
    {
        logger_log(LOG_FATAL, "%s: null argument", __func__);
        return -EINVAL;
    }

    /** no, I don't check again if this is a valid audio pcm packet...*/

    stream_config->nb_channels  = hdr->format_nbc + 1;
    stream_config->sample_rate  = VBanSRList[hdr->format_SR & VBAN_SR_MASK];
    stream_config->bit_fmt      = hdr->format_bit & VBAN_BIT_RESOLUTION_MASK;

    return 0;
}

int packet_set_stream_config(char* buffer, struct stream_config_t const* stream_config, size_t size)
{
    struct VBanHeader* const hdr = PACKET_HEADER_PTR(buffer);

    if ((buffer == 0) || (stream_config == 0))
    {
        logger_log(LOG_FATAL, "%s: null argument", __func__);
        return -EINVAL;
    }

    /** no, I don't check again if this is a valid audio pcm packet...*/

    hdr->format_nbc = stream_config->nb_channels - 1;
    hdr->format_SR  = (hdr->format_SR & ~VBAN_SR_MASK) | vban_sr_from_value(stream_config->sample_rate);
    hdr->format_bit = (hdr->format_SR & ~VBAN_BIT_RESOLUTION_MASK) | stream_config->bit_fmt;
    hdr->format_nbs = size / (stream_config->nb_channels * VBanBitResolutionSize[stream_config->bit_fmt]);

    return 0;
}

/** should better be in vban.h header ?*/
size_t vban_sr_from_value(unsigned int value)
{
    size_t index = 0;
    while ((index < VBAN_SR_MAXNUMBER) && (value != VBanSRList[index]))
    {
        ++index;
    }

    return index;
}
