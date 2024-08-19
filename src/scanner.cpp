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

#include <signal.h>

#include "scanner.h"
#include "xlibutil.h"

#include <X11/Xlib.h>

ProcessId::ProcessId(const QString &command, pid_t pid, const TrayItemConfig &config, uint timeout, bool checkNormality, const QRegularExpression &windowName) :
    command(command),
    pid(pid),
    config(config),
    timeout(timeout),
    checkNormality(checkNormality),
    windowName(windowName)
{
    etimer.start();
}

ProcessId::ProcessId(const ProcessId &obj) {
    command = obj.command;
    pid = obj.pid;
    config = obj.config;
    etimer = obj.etimer;
    timeout = obj.timeout;
    checkNormality = obj.checkNormality;
    windowName = obj.windowName;
}

ProcessId& ProcessId::operator=(const ProcessId &obj) {
    if (this == &obj)
        return *this;

    command = obj.command;
    pid = obj.pid;
    config = obj.config;
    etimer = obj.etimer;
    timeout = obj.timeout;
    checkNormality = obj.checkNormality;
    windowName = obj.windowName;
    return *this;
}

Scanner::Scanner(TrayItemManager *manager) {
    m_manager = manager;
    m_timer = new QTimer();
    // Check every 1/4 second if a window has been created.
    m_timer->setInterval(250);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(check()));
}

Scanner::~Scanner() {
    delete m_timer;
}

void Scanner::enqueueSearch(const QRegularExpression &windowName, uint maxTime, bool checkNormality, const TrayItemConfig &config) {
    enqueue(QString(), QStringList(), windowName, config, maxTime, checkNormality);
}

void Scanner::enqueueLaunch(const QString &command, const QStringList &arguments, const QRegularExpression &windowName, uint maxTime, bool checkNormality, const TrayItemConfig &config) {
    enqueue(command, arguments, windowName, config, maxTime, checkNormality);
}

void Scanner::enqueue(const QString &command, const QStringList &arguments, const QRegularExpression &windowName, const TrayItemConfig &config, uint maxTime, bool checkNormality) {
    qint64 pid = 0;
    bool started = true;

    if (maxTime == 0) {
        maxTime = 1;
    }
    maxTime *= 1000;

    ProcessId processId(command, 0, config, maxTime, checkNormality, windowName);
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
        pi.setValue(id);

        Window w = None;
        if (id.windowName.pattern().isEmpty()) {
            if (kill(id.pid, 0) == -1) {
                // PID does not exist; fall back to name matching.
                id.windowName.setPattern(QRegularExpression::escape(id.command.split("/").last()));
                pi.setValue(id);
            } else {
                // Check based on PID.
                w = XLibUtil::pidToWid(XLibUtil::display(), XLibUtil::appRootWindow(), id.checkNormality, id.pid);
            }
        }
        // Use an if instead of else because the windowName could be set previously if the PID does not exist.
        if (!id.windowName.pattern().isEmpty()) {
            // Check based on window name if the user specified a window name to match with.
            w = XLibUtil::findWindow(XLibUtil::display(), XLibUtil::appRootWindow(), id.checkNormality, id.windowName, m_manager->dockedWindows());
        }

        if (w != None) {
            emit windowFound(w, id.config);
            pi.remove();
        } else {
            if (id.etimer.hasExpired(id.timeout)) {
                QMessageBox::information(0, tr("KDocker"), tr("Could not find a matching window for %1 in the specified time: %2 seconds.").arg(id.command).arg(QString::number(id.timeout / 1000)));
                pi.remove();
            }
        }
    }
    if (m_processes.isEmpty()) {
        m_timer->stop();
        emit stopping();
    }
}
