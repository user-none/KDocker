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
#include <QTextStream>

#include "constants.h"
#include "kdocker.h"
#include "trayitemmanager.h"

#include <getopt.h>

#include <X11/Xlib.h>

KDocker::KDocker() {
    m_trayItemManager = new TrayItemManager();
}

KDocker::~KDocker() {
    if (m_trayItemManager) {
        delete m_trayItemManager;
        m_trayItemManager = 0;
    }
}

void KDocker::undockAll() {
    if (m_trayItemManager) {
        m_trayItemManager->undockAll();
    }
}

bool KDocker::x11EventFilter(XEvent *ev) {
    if (m_trayItemManager) {
        return m_trayItemManager->x11EventFilter(ev);
    }
    return false;
}

void KDocker::run() {
    if (m_trayItemManager) {
        m_trayItemManager->processCommand(QCoreApplication::arguments());
    }
}

void KDocker::handleMessage(const QString &args) {
    if (m_trayItemManager) {
        m_trayItemManager->processCommand(args.split("\n"));
    }
}

/*
 * handle arguments that output information to the user. We want to handle
 * them here so they are printed on the tty that the application is run from.
 * If we left it up to TrayItemManager with the rest of the arguments these
 * would be printed on the tty the instace was started on not the instance the
 * user is calling from.
 */
void KDocker::preProcessCommand(int argc, char **argv) {
    int option;
    optind = 0; // initialise the getopt static
    while ((option = getopt(argc, argv, Constants::OPTIONSTRING)) != -1) {
        switch (option) {
            case '?':
                printUsage();
                ::exit(1);
                break;
            case 'a':
                printAbout();
                ::exit(0);
                break;
            case 'h':
                printHelp();
                ::exit(0);
                break;
            case 'u':
                printUsage();
                ::exit(0);
                break;
            case 'v':
                printVersion();
                ::exit(0);
                break;
        }
    }
}

void KDocker::printAbout() {
    QTextStream out(stdout);

    out << Constants::ABOUT_MESSAGE << endl;
}

void KDocker::printHelp() {
    QTextStream out(stdout);

    out << tr("Usage: %1 [options] [command] -- [command options]").arg(qApp->applicationName().toLower()) << endl;
    out << tr("Docks any application into the system tray") << endl;
    out << endl;
    out << tr("Command") << endl;
    out << tr("Run command and dock window") << endl;
    out << tr("Use -- after command to specify options for command") << endl;
    out << endl;
    out << tr("Options") << endl;
    out << "-a     \t" << tr("Show author information") << endl;
    out << "-b     \t" << tr("Don't warn about non-normal windows (blind mode)") << endl;
    out << "-c     \t" << tr("Don't iconify on close") << endl;
    out << "-d secs\t" << tr("Maximum time in seconds to allow for command to start and open a window (defaults to 5 sec)") << endl;
    out << "-f     \t" << tr("Dock window that has focus (active window)") << endl;
    out << "-h     \t" << tr("Display this help") << endl;
    out << "-l     \t" << tr("Iconify when focus lost") << endl;
    out << "-m     \t" << tr("Keep application window showing (dont hide on dock)") << endl;
    out << "-n name\t" << tr("Name used for matching when running command (fall back in case command is a launcher so pid and command won't match the window tile or class)") << endl;
    out << "-o     \t" << tr("Iconify when obscured") << endl;
    out << "-p secs\t" << tr("Set ballooning timeout (popup time)") << endl;
    out << "-q     \t" << tr("Disable ballooning title changes (quiet)") << endl;
    out << "-r     \t" << tr("Remove this application from the pager") << endl;
    out << "-s     \t" << tr("Make the window sticky (appears on all desktops)") << endl;
    out << "-t     \t" << tr("Remove this application from the taskbar") << endl;
    out << "-v     \t" << tr("Display version") << endl;
    out << "-w wid \t" << tr("Window id of the application to dock") << endl;
    out << "-x pid \t" << tr("Process id of the application to dock") << endl;
    out << "-y     \t" << tr("Force matching of command by using name instead of pid") << endl;
    out << endl;
    out << tr("Bugs and wishes to https://bugs.launchpad.net/kdocker") << endl;
    out << tr("Project information at https://launchpad.net/kdocker") << endl;
}

void KDocker::printUsage() {
    QTextStream out(stdout);
    out << tr("Usage: %1 [options] command").arg(qApp->applicationName().toLower()) << endl;
    out << tr("Try `%1 -h' for more information").arg(qApp->applicationName().toLower()) << endl;
}

void KDocker::printVersion() {
    QTextStream out(stdout);
    out << tr("%1 version: %2").arg(qApp->applicationName()).arg(qApp->applicationVersion()) << endl;
    out << tr("Using Qt version: %1").arg(qVersion()) << endl;
}
