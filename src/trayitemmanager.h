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

#include <QList>
#include <QObject>
#include <QStringList>

#include "scanner.h"
#include "trayitem.h"

#include <sys/types.h>

#include <X11/Xlib.h>

class Scanner;

class TrayItemManager : public QObject {
    Q_OBJECT

public:
    TrayItemManager();
    ~TrayItemManager();
    bool x11EventFilter(XEvent *ev);
    void processCommand(const QStringList &args);
    QList<Window> dockedWindows();

public slots:
    void dockWindow(Window window, TrayItemSettings settings);
    Window userSelectWindow(bool checkNormality = true);
    void remove(TrayItem *trayItem);
    void undock(TrayItem *trayItem);
    void undockAll();

private slots:
    void selectAndIconify();
    void checkCount();

private:
    bool isWindowDocked(Window window);

    Scanner *m_scanner;
    QList<TrayItem*> m_trayItems;
};

#endif	/* _TRAYITEMMANAGER_H */
