/*
 *  Copyright (C) 2009, 2012 John Schember <john@nachtimwald.com>
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
#include <QMessageBox>
#include <QMutableListIterator>
#include <QProcess>
#include <QStringList>
#include <QX11Info>

#include "scanner.h"
#include "xlibutil.h"

#include <signal.h>

Scanner::Scanner(TrayItemManager *manager) {
    m_manager = manager;
    m_timer = new QTimer();
    // Check every second if a window has been created.
    m_timer->setInterval(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(check()));
}

Scanner::~Scanner() {
    delete m_timer;
}

void Scanner::enqueue(const QString &command, const QStringList &arguments, TrayItemSettings settings, int maxTime, bool checkNormality, const QRegExp &windowName) {
    qint64 pid = 0;
    bool started = true;

    if (maxTime <= 0) {
        maxTime = 1;
    }

    ProcessId processId = {command, 0, settings, 0, maxTime, checkNormality, windowName};
    if (!command.isEmpty()) {
        // Launch the requested application.
        started = QProcess::startDetached(command, arguments, "", &pid);
    }

    if (started) {
        // Either the application started properly or we are matching by name.
        processId.pid = static_cast<pid_t>(pid);
        m_processes.append(processId);
        m_timer->start();
    } else {
        QMessageBox::information(0, qApp->applicationName(), tr("%1 did not start properly.").arg(command));
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
        if (id.windowName.isEmpty()) {
            if (kill(id.pid, 0) == -1) {
                // PID does not exist; fall back to name matching.
                id.windowName.setPattern(id.command.split("/").last());
                id.windowName.setPatternSyntax(QRegExp::FixedString);
                pi.setValue(id);
            } else {
                // Check based on PID.
                w = XLibUtil::pidToWid(QX11Info::display(), QX11Info::appRootWindow(), id.checkNormality, id.pid);
            }
        }
        // Use an if instead of else because the windowName could be set previously if the PID does not exist.
        if (!id.windowName.isEmpty()) {
            // Check based on window name if the user specified a window name to match with.
            w = XLibUtil::findWindow(QX11Info::display(), QX11Info::appRootWindow(), id.checkNormality, id.windowName, m_manager->dockedWindows());
        }

        if (w != None) {
            emit windowFound(w, id.settings);
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
        emit stopping();
    }
}
