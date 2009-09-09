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

#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QTextStream>
#include <QX11Info>
#include <qt4/QtGui/qpixmap.h>

#include "trayitemmanager.h"
#include "util.h"

#include <stdio.h>

TrayItemManager *TrayItemManager::g_trayItemManager = 0;

TrayItemManager *TrayItemManager::instance() {
    if (!g_trayItemManager) {
        g_trayItemManager = new TrayItemManager();
    }
    return g_trayItemManager;
}

TrayItemManager::TrayItemManager() {
    m_systemTray = 0;
    checkSystemTray();
    connect(this, SIGNAL(systemTrayDestroyEvent()), this, SLOT(checkSystemTray()));
}

TrayItemManager::~TrayItemManager() {
    while (!m_trayItems.isEmpty()) {
        TrayItem *t = m_trayItems.takeFirst();
        delete t;
        t = 0;
    }
}

/*
 * The X11 Event Filter. Pass on events to the QTrayLabels that we created.
 * The logic and the code below is a bit fuzzy.
 *  a) Events about windows that are being docked need to be processed only by
 *     the QTrayLabel object that is docking that window.
 *  b) Events about windows that are not docked but of interest (like
 *     SystemTray) need to be passed on to all QTrayLabel objects.
 *  c) When a QTrayLabel manages to find the window that is was looking for, we
 *     need not process the event further
 */
bool TrayItemManager::x11EventFilter(XEvent *ev) {
    XAnyEvent *event = (XAnyEvent *) ev;
    if (event->window == m_systemTray) {
        if (event->type != DestroyNotify) {
            return false; // not interested in others
        }
        m_systemTray = 0;
        emit(systemTrayDestroyEvent());
        return true;
    }

    QListIterator<TrayItem*> ti(m_trayItems);
    bool ret = false;

    // We pass on the event to all tray labels
    TrayItem *t;
    while (ti.hasNext()) {
        t = ti.next();
        Window w = t->dockedWindow();
        bool res = t->x11EventFilter(ev);
        if (w == event->window) {
            return res;
        }
        if (w != None) {
            ret |= res;
        } else if (res) {
            return true;
        }
    }

    return ret;
}

void TrayItemManager::restoreAllWindows() {
    QListIterator<TrayItem*> ti(m_trayItems);
    while (ti.hasNext()) {
        ti.next()->restoreWindow();
    }
}

bool TrayItemManager::selectAndIconify() {
    QTextStream out(stdout);
    out << tr("Select the application/window to dock with the left mouse button.") << endl;
    out << tr("Click any other mouse button to abort.") << endl;

    const char *error = NULL;
    Window window = selectWindow(QX11Info::display(), &error);
    if (window == None) {
        if (error) {
            QMessageBox::critical(NULL, tr("KDocker"), tr(error));
        }
        checkCount();
        return false;
    }

    if (!isNormalWindow(QX11Info::display(), window)) {
        if (QMessageBox::warning(NULL, tr("KDocker"), tr("The window you are attempting to dock does not seem to be a normal window."), QMessageBox::Abort) == QMessageBox::Abort) {
            checkCount();
            return false;
        }
    }

    if (!isWindowDocked(window)) {
        TrayItem *ti = new TrayItem(window);
        ti->show();
        ti->iconifyWindow();
        connect(ti, SIGNAL(selectAnother()), this, SLOT(selectAndIconify()));
        connect(ti, SIGNAL(itemClose(TrayItem*)), this, SLOT(itemClosed(TrayItem*)));
        m_trayItems.append(ti);
        return true;
    }

    QMessageBox::information(0, tr("KDocker"), tr("This window is already docked.\nClick on system tray icon to toggle docking."));

    return false;
}

void TrayItemManager::itemClosed(TrayItem *trayItem) {
    m_trayItems.removeAll(trayItem);
    delete trayItem;
    trayItem = 0;

    checkCount();
}

void TrayItemManager::checkSystemTray() {
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        if (!m_systemTray) {
            m_systemTray = systemTray(QX11Info::display());
            subscribe(QX11Info::display(), m_systemTray, StructureNotifyMask, true);
        }
    } else {
        QMessageBox::critical(NULL, tr("KDocker"), tr("There is no system tray. Exiting."));
        restoreAllWindows();
        ::exit(0);
    }
}

void TrayItemManager::checkCount() {
    if (m_trayItems.isEmpty()) {
        ::exit(0);
    }
}

bool TrayItemManager::isWindowDocked(Window window) {
    QListIterator<TrayItem*> ti(m_trayItems);
    while (ti.hasNext()) {
        if (ti.next()->dockedWindow() == window) {
            return true;
        }
    }

    return false;
}
