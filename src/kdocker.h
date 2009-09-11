/*
 *  Copyright (C) 2009 John Schember <john@nachtimwald.com>
 *  Copyright (C) 2004 Girish Ramakrishnan All Rights Reserved.
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

#ifndef _KDOCKER_H
#define	_KDOCKER_H

#include <QApplication>
#include <QStringList>
#include "trayitemmanager.h"

class KDocker : public QApplication {
    Q_OBJECT

public:
    KDocker(int &argc, char **argv);
    ~KDocker();

    TrayItemManager *trayItemManager();
    bool x11EventFilter(XEvent *event);

private:
    void notifyPreviousInstance(Window prevInstance, QStringList args = QStringList());
    void checkArguments(QStringList args);

    TrayItemManager *m_trayItemManager;

};

#endif	/* _KDOCKER_H */
