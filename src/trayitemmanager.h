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

#ifndef _TRAYITEMMANAGER_H
#define	_TRAYITEMMANAGER_H

#include <QObject>
#include <QStringList>

#include "trayitem.h"

#include <X11/Xlib.h>

class TrayItemManager : public QObject {
    Q_OBJECT

public:
    static TrayItemManager *instance();

    TrayItemManager();
    ~TrayItemManager();
    bool x11EventFilter(XEvent *ev);
    void restoreAllWindows();
    void processCommand(const QStringList &args);

public slots:
    Window userSelectWindow(bool checkNormality = true);
    void undock(TrayItem *trayItem);
    void undockAll();

private slots:
    void selectAndIconify();

private:
    void checkCount();
    bool isWindowDocked(Window window);

    QList<TrayItem*> m_trayItems;

    static TrayItemManager *g_trayItemManager;
};

#endif	/* _TRAYITEMMANAGER_H */
