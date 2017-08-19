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

#include "stream.h"
#include <stddef.h>
#include <string.h>

/**
 * @warning: MUST BE ADAPTED WITH VBAN PROTOCOL EVOLUTIONS
 */
static char* const bit_fmt_names [VBAN_BIT_RESOLUTION_MAX] = 
{
    "8I", "16I", "24I", "32I", "32F", "64F", "12I", "10I"
};

#define HELP_MESSAGE    "Recognized bit format are 8I, 16I, 24I, 32I, 32F, 64F, 12I, 10I"

enum VBanBitResolution stream_parse_bit_fmt(char const* argv)
{
    size_t index = 0;
    while ((index < VBAN_BIT_RESOLUTION_MAX) && strcmp(argv, bit_fmt_names[index]))
    {
        ++index;
    }

    return index;
}

char const* stream_print_bit_fmt(enum VBanBitResolution bit_fmt)
{
    return (bit_fmt < VBAN_BIT_RESOLUTION_MAX) ? bit_fmt_names[bit_fmt] : "Invalid";
}

char const* stream_bit_fmt_help()
{
    return HELP_MESSAGE;
}


