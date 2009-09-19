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

void Scanner::enqueue(const QString &command, const QStringList &arguments, TrayItemSettings settings, int maxTime, bool checkNormality, bool windowNameMatch, const QString &windowName) {
    qint64 pid;
    QString name;

    if (maxTime <= 0) {
        maxTime = 1;
    }
    if (windowName.isEmpty()) {
        name = command.split("/").last();
    }
    else {
        name = windowName;
    }
    
    if (QProcess::startDetached(command, arguments, "", &pid)) {
        ProcessId processId = {command, (pid_t) pid, settings, 0, maxTime, checkNormality, windowNameMatch, name};
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
        id.count++;
        pi.setValue(id);
        
        Window w = None;
        if (id.windowNameMatch || kill(id.pid, 0) == -1) {
            // Check based on window name if force matching by window name is set or the PID is not valid.
            w = findWindow(QX11Info::display(), QX11Info::appRootWindow(), id.checkNormality, id.windowName);
        } else {
            // Check based on PID if it is still valid.
            w = pidToWid(QX11Info::display(), QX11Info::appRootWindow(), id.checkNormality, id.pid);
        }
        
        if (w != None) {
            emit(windowFound(w, id.settings));
            pi.remove();
        } else {
            if (id.count >= id.maxCount) {
                QMessageBox::information(0, tr("KDocker"), tr("Could not find a matching window for %1 in the specified time: %2 seconds.").arg(id.command).arg(QString::number(id.maxCount)));
                pi.remove();
            }
        }
    }
    if (m_processes.isEmpty()) {
        m_timer->stop();
        emit(stopping());
    }
}
