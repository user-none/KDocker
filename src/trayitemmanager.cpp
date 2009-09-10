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

#include "constants.h"
#include "trayitemmanager.h"
#include "util.h"

#include <stdio.h>
#include <getopt.h>

TrayItemManager *TrayItemManager::g_trayItemManager = 0;
const char *TrayItemManager::m_optionString = "+abfhmop:qtvw:";

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

void TrayItemManager::processCommand(const QStringList &args) {
    int option;
    Window window = 0;
    int balloonTimeout = 4000;
    bool iconify = true;
    bool skipTaskbar = false;
    bool dockObscure = false;
    bool checkNormality = true;

    // Turn the QStringList of arguments into something getopt can use.
    int argc = args.count();
    char *argv[argc + 1];
    for (int i = 0; i < argc; i++) {
        argv[i] = args[i].toLatin1().data();
    }
    argv[argc + 1] = NULL; // null terminate the array

    optind = 0; // initialise the getopt static
    while ((option = getopt(argc, argv, m_optionString)) != -1) {
        switch (option) {
            case '?':
                printUsage();
                checkCount();
                return;
            case 'a':
                printAbout();
                checkCount();
                return;
            case 'b':
                checkNormality = false;
                break;
            case 'f':
                window = activeWindow(QX11Info::display());
                if (!window) {
                    // error no window active
                    checkCount();
                    return;
                }
                break;
            case 'h':
                printHelp();
                checkCount();
                return;
            case 'm':
                iconify = false;
                break;
            case 'o':
                dockObscure = true;
                break;
            case 'p':
                balloonTimeout = atoi(optarg) * 1000; // convert to ms
                break;
            case 'q':
                balloonTimeout = 0; // same as '-p 0'
                break;
            case 't':
                skipTaskbar = true;
                break;
            case 'v':
                printVersion();
                checkCount();
                return;
            case 'w':
                if ((optarg[1] == 'x') || (optarg[1] == 'X'))
                    sscanf(optarg, "%x", (unsigned *) & window);
                else
                    window = (Window) atoi(optarg);
                if (!isValidWindowId(QX11Info::display(), window)) {
                    //qDebug() << QString("Window 0x%x invalid").arg((unsigned) w);
                    checkCount();
                    return;
                }
                break;
        } // switch (option)
    } // while (getopt)

    if (!window) {
        window = userSelectWindow();
    }
    // No window was selected or set.
    if (!window) {
        checkCount();
        return;
    }

    if (isWindowDocked(window)) {
        QMessageBox::information(0, tr("KDocker"), tr("This window is already docked.\nClick on system tray icon to toggle docking."));
        checkCount();
        return;
    }

    TrayItem *ti = new TrayItem(window);
    connect(ti, SIGNAL(selectAnother()), this, SLOT(selectAndIconify()));
    connect(ti, SIGNAL(itemClose(TrayItem*)), this, SLOT(itemClosed(TrayItem*)));
    ti->show();
    if (iconify) {
        ti->iconifyWindow();
    }
    m_trayItems.append(ti);
}

Window TrayItemManager::userSelectWindow() {
    QTextStream out(stdout);
    out << tr("Select the application/window to dock with the left mouse button.") << endl;
    out << tr("Click any other mouse button to abort.") << endl;

    const char *error = NULL;
    Window window = selectWindow(QX11Info::display(), &error);
    if (!window) {
        if (error) {
            QMessageBox::critical(NULL, tr("KDocker"), tr(error));
        }
        checkCount();
        return 0;
    }

    if (!isNormalWindow(QX11Info::display(), window)) {
        if (QMessageBox::warning(NULL, tr("KDocker"), tr("The window you are attempting to dock does not seem to be a normal window."), QMessageBox::Abort) == QMessageBox::Abort) {
            checkCount();
            return 0;
        }
    }

    return window;
}

void TrayItemManager::itemClosed(TrayItem *trayItem) {
    m_trayItems.removeAll(trayItem);
    delete trayItem;
    trayItem = 0;

    checkCount();
}

void TrayItemManager::selectAndIconify() {
    Window window = userSelectWindow();

    if (window) {
        TrayItem *ti = new TrayItem(window);
        connect(ti, SIGNAL(selectAnother()), this, SLOT(selectAndIconify()));
        connect(ti, SIGNAL(itemClose(TrayItem*)), this, SLOT(itemClosed(TrayItem*)));
        ti->show();
        ti->iconifyWindow();
        m_trayItems.append(ti);
    }
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

void TrayItemManager::printAbout() {

}

void TrayItemManager::printHelp() {
    QTextStream out(stdout);

    out << tr("Usage: %1 [options] command\n").arg(QString(APP_NAME).toLower()) << endl;
    out << tr("Docks any application into the system tray\n") << endl;
    out << tr("command \tCommand to execute\n") << endl;
    out << tr("Options") << endl;
    out << tr("-a     \tShow author information") << endl;
    out << tr("-b     \tDont warn about non-normal windows (blind mode)") << endl;
    out << tr("-f     \tDock window that has the focus(active window)") << endl;
    out << tr("-h     \tDisplay this help") << endl;
    out << tr("-m     \tKeep application window mapped (dont hide on dock)") << endl;
    out << tr("-o     \tDock when obscured") << endl;
    out << tr("-p secs\tSet ballooning timeout (popup time)") << endl;
    out << tr("-q     \tDisable ballooning title changes (quiet)") << endl;
    out << tr("-t     \tRemove this application from the task bar") << endl;
    out << tr("-v     \tDisplay version") << endl;
    out << tr("-w wid \tWindow id of the application to dock\n") << endl;
    out << endl;
    out << tr("Bugs and wishes to https://bugs.launchpad.net/kdocker") << endl;
    out << tr("Project information at https://launchpad.net/kdocker") << endl;
}

void TrayItemManager::printUsage() {
    QTextStream out(stdout);
    out << QString("%1: invalid option -- %2").arg(APP_NAME).arg(optopt) << endl;
    out << tr("Try `%1 -h' for more information").arg(QString(APP_NAME).toLower()) << endl;
}

void TrayItemManager::printVersion() {
    QTextStream out(stdout);
    out << "KDocker version: " << APP_VERSION << endl;
    out << "Using Qt version: " << qVersion() << endl;
}
