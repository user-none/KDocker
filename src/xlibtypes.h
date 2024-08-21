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

#include <QtGlobal>

// Types we use with XLibUtil to represent X11 types we can't / don't want
// to expose. See details on XLibUtil class about why. These are not a
// redefinition. Instead these are our types that can hold the X11 type's
// data. In the case of window and atom, the value may be marshaled within XLibUtil
// if pointers are invoked. These should only be used with XLibUtil functions and
// not X11 functions.

typedef quint32 atom_t;

// X11 defines `Window` as an `unsigned long` which is often 64 bit but not
// always. However, xcb defines it as a `uint32_t`. The documentation isn't
// fully clear but it's safe to assume windows will always be within the range
// of an unsigned 32 bit integer.
typedef quint32 windowid_t;

// Hiding XSizeHints. X11 uses a typdef'ed anonymous struct
// so we can't forward declare the type. Instead we'll define
// it as void and use pointers. Not ideal but it will work.
typedef void XLibUtilSizeHints;

#endif // _XLIBTYPES
