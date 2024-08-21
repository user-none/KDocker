/*
 *  Copyright (C) 2024 John Schember <john@nachtimwald.com>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

#ifndef _XLIBTYPES
#define _XLIBTYPES

// Due to isolation of X11 headers (see xlibutil.h for why),
// we need to define some types from X11 we're using outside
// of the helper class.

typedef uint64_t Atom;
typedef uint64_t Window;

// Hiding XSizeHints. X11 uses a typdef'ed anonymous struct
// so we can't forward declare the type. Instead we'll define
// it as void and use pointers. Not ideal but it will work.
typedef void XLibUtilSizeHints;

#endif // _XLIBTYPES
