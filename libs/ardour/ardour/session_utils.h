/*
 * Copyright (C) 2007-2012 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2008-2015 Paul Davis <paul@linuxaudiosystems.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#ifndef __ardour_session_utils_h__
#define __ardour_session_utils_h__

#include <string>

#include "ardour/libardour_visibility.h"

namespace ARDOUR {

	LIBARDOUR_API extern int find_session (std::string str, std::string& path, std::string& snapshot, bool& isnew);
	LIBARDOUR_API extern int inflate_session (const std::string& zipfile, const std::string& target_dir, std::string& path, std::string& snapshot);
	LIBARDOUR_API extern std::string inflate_error (int);

};

#endif
