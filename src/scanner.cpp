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

#include <QMutableListIterator>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QX11Info>
#include <QDebug>

#include "scanner.h"
#include "util.h"

Scanner::Scanner() {
    m_timer = new QTimer();
    m_timer->setInterval(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(check()));
}

Scanner::~Scanner() {
    delete m_timer;
}

void Scanner::enqueue(const QString &command, const QStringList &arguments, TrayItemSettings settings) {
    qint64 pid;
    if (QProcess::startDetached(command, arguments, "", &pid)) {
        ProcessId processId = {(int)pid, settings, 0};
        m_processes.append(processId);
        m_timer->start();
    }
    if (m_processes.isEmpty()) {
        emit(stopping());
    }
}

bool Scanner::isRunning() {
    return !m_processes.isEmpty();
}

void Scanner::check() {
    QMutableListIterator<ProcessId> pi(m_processes);
    while (pi.hasNext()) {
        ProcessId id = pi.next();
        // if pid is not running remove and move to next
        id.count++;
        Window w = pidToWid(QX11Info::display(), id.pid, QX11Info::appRootWindow());
        if (w != None) {
            emit(windowFound(w, id.settings));
            pi.remove();
        } else {
            // 180 seconds == 3 minutes
            if (id.count >= 180) {
                // Could not find window.
                pi.remove();
            }
        }
    }
    if (m_processes.isEmpty()) {
        m_timer->stop();
        emit(stopping());
    }
}
