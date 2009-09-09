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
#include <QTextStream>
#include <QX11Info>
#include <QDebug>

#include "kdocker.h"
#include "trayitemmanager.h"
#include "util.h"

#include <stdio.h>

#include <X11/Xlib.h>

KDocker::KDocker(int &argc, char **argv) : QApplication(argc, argv) {
    setOrganizationName(ORG_NAME);
    setOrganizationDomain(DOM_NAME);
    setApplicationName(APP_NAME);

    /*
     * Detect and transfer control to previous instance (if one exists)
     * _KDOCKER_RUNNING is a X Selection. We start out by trying to locate the
     * selection owner. If someone else owns it, transfer control to that
     * instance of KDocker
     */
    Display *display = QX11Info::display();
    Atom kdocker = XInternAtom(display, "_KDOCKER_RUNNING", False);
    Window prevInstance = XGetSelectionOwner(display, kdocker);

    if (prevInstance == None) {
        Window selectionOwner = XCreateSimpleWindow(display, QX11Info::appRootWindow(), 1, 1, 1, 1, 1, 1, 1);
        XSetSelectionOwner(display, kdocker, selectionOwner, CurrentTime);
        m_trayItemManager = TrayItemManager::instance();
        trayItemManager()->selectAndIconify();
    } else {
        notifyPreviousInstance(prevInstance); // does not return
    }
}

KDocker::~KDocker() {
    if (m_trayItemManager) {
        delete m_trayItemManager;
        m_trayItemManager = 0;
    }
}

TrayItemManager *KDocker::trayItemManager() {
    return m_trayItemManager;
}

bool KDocker::x11EventFilter(XEvent *ev) {
    if (ev->type == ClientMessage) {
        // look for requests from a new instance of kdocker
        XClientMessageEvent *client = (XClientMessageEvent *) ev;
        if (!(client->message_type == 0x220679 && client->data.l[0] == 0xBABE))
            return false;
        trayItemManager()->selectAndIconify();
        return true;
    } else {
        return trayItemManager()->x11EventFilter(ev);
    }
}

void KDocker::notifyPreviousInstance(Window prevInstance) {
    Display *display = QX11Info::display();

    /*
     * Tell our previous instance that we came to pass. Actually, it can
     * figure it out itself using PropertyNotify events but this is a lot nicer
     */
    XClientMessageEvent dock_event;
    memset(&dock_event, 0, sizeof (XClientMessageEvent));
    dock_event.display = display;
    dock_event.window = prevInstance;
    dock_event.send_event = True;
    dock_event.type = ClientMessage;
    dock_event.message_type = 0x220679; // it all started this day
    dock_event.format = 8;
    dock_event.data.l[0] = 0xBABE; // love letter ;)
    dock_event.data.l[1] = getpid();
    XSendEvent(display, prevInstance, False, 0, (XEvent *) & dock_event);
    XSync(display, False);

    ::exit(0);
}

void KDocker::printVersion() {
    QTextStream out(stdout);
    out << "KDocker version: " << APP_VERSION << endl;
    out << "Using Qt version: " << qVersion() << endl;
}

void KDocker::printUsage() {
    QTextStream out(stdout);
    out << QString("%1: invalid option -- %2").arg(APP_NAME).arg(optopt) << endl;
    out << tr("Try `%1 -h' for more information").arg(QString(APP_NAME).toLower()) << endl;
}

void KDocker::printHelp() {
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
    out << tr("Bugs and wishes to ") << endl;
    out << tr("Project information at ") << endl;
}
