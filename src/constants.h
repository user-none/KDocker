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

#include <QObject>
#include <QString>


class Constants : QObject {
    Q_OBJECT

public:
    static const QString APP_NAME;
    static const QString ORG_NAME;
    static const QString DOM_NAME;
    static const QString APP_VERSION;
    static const QString WEBSITE;

    static const QString DBUS_NAME;
    static const QString DBUS_PATH;
};

#endif	/* _CONSTANTS_H */
