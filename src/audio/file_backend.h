/*
 *  This file is part of vban.
 *  Copyright (c) 2015 by Benoît Quiniou <quiniouben@yahoo.fr>
 *  Copyright (c) 2017 by Markus Buschhoff <markus.buschhoff@tu-dortmund.de>
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

#ifndef __FILE_BACKEND_H__
#define __FILE_BACKEND_H__

#include "audio_backend.h"

#define FILE_BACKEND_NAME   "file"
int file_backend_init(audio_backend_handle_t* handle);

#endif /*__FILE_BACKEND_H__*/

