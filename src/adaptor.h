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


#ifndef _ADAPTOR
#define	_ADAPTOR

// this file is included by the auto generated dbus adaptor .h file.
// All types that are referenced in the dbus xml need to be included
// here for the auto generated code.

#include "trayitemoptions.h"

#include <QMap>

// Needed for TrayItemManager::listWindows.
// The QMetaObject system can't pass a two item template to Q_ARG or Q_RETURN_ARG
// so it needs to be typedef'ed to compile.
typedef QMap<uint32_t, QString> WindowNameMap;

#endif // _ADAPTOR
