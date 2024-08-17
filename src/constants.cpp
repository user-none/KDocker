/*
 *  Copyright (C) 2009, 2012, 2015 John Schember <john@nachtimwald.com>
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

#include "constants.h"


const QString Constants::APP_NAME    = "KDocker";
const QString Constants::ORG_NAME    = "com.kdocker";
const QString Constants::DOM_NAME    = "kdocker.com";
const QString Constants::WEBSITE     = "https://github.com/user-none/KDocker";
const QString Constants::APP_VERSION = "5.99";

const char *Constants::OPTIONSTRING  = "+abd:e:fhi:I:jklmn:op:qrstvuw:x:z";

const QString Constants::ABOUT_MESSAGE = QString("%1 %2\n\n%3").arg(Constants::APP_NAME).arg(Constants::APP_VERSION).arg(Constants::WEBSITE);

const QString Constants::DBUS_NAME   = "com.kdocker.kdocker";
const QString Constants::DBUS_PATH   = "/controller";
