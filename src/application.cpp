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

#include "application.h"
#include <sys/socket.h>
#include <unistd.h>

int Application::m_closeSignalFd[2];

Application::Application(const QString &appId, int &argc, char **argv) : QtSingleApplication(appId, argc, argv) {
    m_kdocker = 0;

    // Translate UNIX signals to Qt signals (See https://doc.qt.io/qt-5/unix-signals.html)
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, m_closeSignalFd))
       qFatal("Couldn't create signal handling socketpair");

    m_closeSignalSocketNotifier = new QSocketNotifier(m_closeSignalFd[1], QSocketNotifier::Read, this);
    connect(m_closeSignalSocketNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(handleCloseSignal()));
}

void Application::setKDockerInstance(KDocker *kdocker) {
    m_kdocker = kdocker;
}

void Application::notifyCloseSignal() {
    char tmp = 1;
    ::write(m_closeSignalFd[0], &tmp, sizeof(tmp));
}

void Application::handleCloseSignal() {
    m_closeSignalSocketNotifier->setEnabled(false);
    char tmp;
    ::read(m_closeSignalFd[1], &tmp, sizeof(tmp));

    if (m_kdocker) {
        m_kdocker->undockAll();
    }
    quit();

    m_closeSignalSocketNotifier->setEnabled(true);
}
