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

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QX11Info>

#include "constants.h"
#include "kdocker.h"
#include "trayitemmanager.h"
#include "util.h"

#include <stdio.h>
#include <unistd.h>

#include <X11/Xlib.h>

KDocker::KDocker(int &argc, char **argv) : QApplication(argc, argv) {
    setOrganizationName(ORG_NAME);
    setOrganizationDomain(DOM_NAME);
    setApplicationName(APP_NAME);

    QStringList args = QCoreApplication::arguments();
    checkArguments(args);

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
        trayItemManager()->processCommand(args);
    } else {
        notifyPreviousInstance(prevInstance, args); // does not return
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
        if (!(client->message_type == 0x220679 && client->data.l[0] == 0xBABE)) {
            return false;
        }

        QString argsPath = TMPFILE_PREFIX + QString::number(client->data.l[1]);
        QFileInfo fileInfo(argsPath);
        if (getuid() != fileInfo.ownerId()) {
            /*
             * We make sure that the owner of this process and the owner of the file
             * are the same. This will prevent someone from executing arbitrary
             * programs by sending client message. Of course, you can send a message
             * only if you are authenticated to the X session and have permission to
             * create files in TMPFILE_PREFIX. So this code is there just for the
             * heck of it.
             */
            QFile::remove(argsPath);
            return true;
        }
        QFile argsFile(argsPath);
        if (!argsFile.open(QIODevice::ReadOnly)) {
            return true;
        }
        QTextStream argsStream(&argsFile);
        QStringList args;
        while (!argsStream.atEnd()) {
            args << argsStream.readLine();
        }
        argsFile.close();
        QFile::remove(argsPath); // delete the tmp file

        trayItemManager()->processCommand(args);
        return true;
    } else {
        return trayItemManager()->x11EventFilter(ev);
    }
}

void KDocker::notifyPreviousInstance(Window prevInstance, QStringList args) {
    Display *display = QX11Info::display();

    // Dump all arguments in temporary file
    QFile argsFile(TMPFILE_PREFIX + QString::number(getpid()));
    if (!argsFile.open(QIODevice::WriteOnly)) {
        return;
    }
    QTextStream argsStream(&argsFile);

    Q_FOREACH(QString arg, args) {
        argsStream << arg << endl;
    }
    argsFile.close();

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

/*
 * handle arguments that output information to the user. We want to handle
 * them here so they are printed on the tty that the application is run from.
 * If we left it up to TrayItemManager with the rest of the arguments these
 * would be printed on the tty the instace was started on not the instance the
 * user is calling from.
 */
void KDocker::checkArguments(QStringList args) {
    if (args.contains("-a")) {
        TrayItemManager::printAbout();
        ::exit(0);
    }
    if (args.contains("-h")) {
        TrayItemManager::printHelp();
        ::exit(0);
    }
    if (args.contains("-u")) {
        TrayItemManager::printUsage();
        ::exit(0);
    }
    if (args.contains("-v")) {
        TrayItemManager::printVersion();
        ::exit(0);
    }
}
