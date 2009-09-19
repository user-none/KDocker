/*
 *  Copyright (C) 2009 John Schember <john@nachtimwald.com>
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

#ifndef _CONSTANTS_H
#define	_CONSTANTS_H

#include <QDir>
#include <QObject>
#include <QString>

#define ORG_NAME "net.launchpad.kdocker"
#define DOM_NAME "launchpad.net/kdocker"
#define APP_NAME "KDocker"
#define APP_VERSION "4.0"

#define OPTIONSTRING "+abcd:fhi:lmn:op:qrstvuw:x:y"
#define TMPFILE_PREFIX QDir::homePath() + "/.kdocker."

const QString ABOUT = QObject::tr("KDocker will help you dock any application into the system tray. This means you can dock openoffice, xmms, firefox, thunderbird, anything! Just point and click. Works for all NET WM compliant window managers - that includes KDE, GNOME, Xfce, Fluxbox and many more.\n\nCreated by %1. Updated and maintained by %2.").arg("Girish Ramakrishnan").arg("John Schember");

#endif	/* _CONSTANTS_H */

