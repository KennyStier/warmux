/******************************************************************************
 *  Warmux is a convivial mass murder game.
 *  Copyright (C) 2001-2011 Warmux Team.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 ******************************************************************************
 * The version players, servers and the index server compare: the release
 * number, plus "-testing" for testing builds.
 *****************************************************************************/

#ifndef WARMUX_VERSION_H
#define WARMUX_VERSION_H

#include "WARMUX_net_defaults.h"

#ifndef WARMUX_TESTING
#  define WARMUX_TESTING 0
#endif
#ifndef WARMUX_VERSION_SUFFIX
#  define WARMUX_VERSION_SUFFIX ""
#endif

#define WARMUX_FULL_VERSION PACKAGE_VERSION WARMUX_VERSION_SUFFIX

#endif
