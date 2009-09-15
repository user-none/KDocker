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
#include <QMutableListIterator>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QX11Info>
#include <QDebug>

#include "scanner.h"
#include "util.h"

#include "signal.h"

Scanner::Scanner() {
    m_timer = new QTimer();
    m_timer->setInterval(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(check()));
}

Scanner::~Scanner() {
    delete m_timer;
}

void Scanner::enqueue(const QString &command, const QStringList &arguments, TrayItemSettings settings, int maxTime) {
    qint64 pid;
    if (maxTime <= 0) {
        maxTime = 1;
    }
    if (QProcess::startDetached(command, arguments, "", &pid)) {
        ProcessId processId = {command, (int) pid, settings, 0, maxTime};
        m_processes.append(processId);
        m_timer->start();
    } else {
        QMessageBox::information(0, tr("KDocker"), tr("%1 did not start properly.").arg(command));
    }
}

bool Scanner::isRunning() {
    return !m_processes.isEmpty();
}

void Scanner::check() {
    QMutableListIterator<ProcessId> pi(m_processes);
    while (pi.hasNext()) {
        ProcessId id = pi.next();
        // if pid is not running remove and move to next.
        // kill with a signum of 0 is used to determine if the process is still
        // running.
        if (kill(id.pid, 0) == -1) {
            QMessageBox::information(0, tr("KDocker"), tr("%1 id dead.").arg(id.command));
            pi.remove();
            break;
        }
        id.count++;
        Window w = pidToWid(QX11Info::display(), id.pid, QX11Info::appRootWindow());
        if (w != None) {
            emit(windowFound(w, id.settings));
            pi.remove();
        } else {
            if (id.count >= id.maxCount) {
                QMessageBox::information(0, tr("KDocker"), tr("%1 did not open a window in the specified time %2 seconds.").arg(id.command).arg(QString::number(id.maxCount)));
                pi.remove();
            }
        }
    }
    if (m_processes.isEmpty()) {
        m_timer->stop();
        emit(stopping());
    }
}
