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

#ifndef __STREAM_H__
#define __STREAM_H__

#include "vban/vban.h"

/**
 * stream_config_t structure.
 * This structures helps to pass the stream important audio parameters around.
 */
struct stream_config_t
{
    unsigned int            nb_channels;
    unsigned int            sample_rate;
    enum VBanBitResolution  bit_fmt;
};

/**
 * Helper function to parse command line parameter.
 */
enum VBanBitResolution stream_parse_bit_fmt(char const* argv);
char const* stream_print_bit_fmt(enum VBanBitResolution bit_fmt);

/**
 * Helper function to get the help message.
 */
char const* stream_bit_fmt_help();

#endif /*__STREAM_H__*/

