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

#include "scanner.h"
#include "trayitemmanager.h"
#include "xlibtypes.h"
#include "xlibutil.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QProcess>
#include <QStringList>

#include <signal.h>

ProcessId::ProcessId(const QString &command, pid_t pid, const TrayItemOptions &config, uint64_t timeout,
                     bool checkNormality, const QRegularExpression &searchPattern)
    : command(command), pid(pid), config(config), timeout(timeout), checkNormality(checkNormality),
      searchPattern(searchPattern)
{
    etimer.start();
}

ProcessId::ProcessId(const ProcessId &obj)
{
    command = obj.command;
    pid = obj.pid;
    config = obj.config;
    etimer = obj.etimer;
    timeout = obj.timeout;
    checkNormality = obj.checkNormality;
    searchPattern = obj.searchPattern;
}

ProcessId &ProcessId::operator=(const ProcessId &obj)
{
    if (this == &obj)
        return *this;

    command = obj.command;
    pid = obj.pid;
    config = obj.config;
    etimer = obj.etimer;
    timeout = obj.timeout;
    checkNormality = obj.checkNormality;
    searchPattern = obj.searchPattern;
    return *this;
}

Scanner::Scanner(TrayItemManager *manager)
{
    m_manager = manager;
    // Check every 1/4 second if a window has been created.
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &Scanner::check);
}

void Scanner::enqueueSearch(const QRegularExpression &searchPattern, quint32 maxTime, bool checkNormality,
                            const TrayItemOptions &config)
{
    if (maxTime == 0)
        maxTime = 1;

    ProcessId processId(QString(), 0, config, maxTime * 1000, checkNormality, searchPattern);
    m_processesTitle.append(processId);
    m_timer.start();
}

void Scanner::enqueueLaunch(const QString &command, const QStringList &arguments,
                            const QRegularExpression &searchPattern, quint32 maxTime, bool checkNormality,
                            const TrayItemOptions &config)
{
    if (maxTime == 0)
        maxTime = 1;

    // Launch the requested application.
    qint64 pid;
    if (!QProcess::startDetached(command, arguments, "", &pid)) {
        QMessageBox box;
        box.setText(tr("'%1' did not start properly.").arg(command));
        box.setWindowIcon(QPixmap(":/logo/kdocker.png"));
        box.setIcon(QMessageBox::Information);
        box.setStandardButtons(QMessageBox::Ok);
        box.exec();
        return;
    }

    ProcessId processId(command, 0, config, maxTime * 1000, checkNormality, searchPattern);
    if (!searchPattern.pattern().isEmpty()) {
        m_processesTitle.append(processId);
    } else {
        processId.pid = static_cast<pid_t>(pid);
        m_processesPid.append(processId);
    }
    m_timer.start();
}

bool Scanner::isRunning()
{
    return !m_processesPid.isEmpty() || !m_processesTitle.isEmpty();
}

void Scanner::checkPid()
{
    // Counting backwards because we can remove items from the list
    for (size_t i = m_processesPid.count(); i-- > 0;) {
        ProcessId process = m_processesPid[i];
        windowid_t window = XLibUtil::pidToWid(process.checkNormality, process.pid);
        if (window != 0) {
            emit windowFound(window, process.config);
            m_processesPid.remove(i);
        } else if (process.etimer.hasExpired(process.timeout)) {
            QMessageBox box;
            box.setText(tr("Could not find a window for command '%1'").arg(process.command));
            box.setWindowIcon(QPixmap(":/logo/kdocker.png"));
            box.setIcon(QMessageBox::Information);
            box.setStandardButtons(QMessageBox::Ok);
            box.exec();

            m_processesPid.remove(i);
        }
    }
}

void Scanner::checkTitle()
{
    // Counting backwards because we can remove items from the list
    for (size_t i = m_processesTitle.count(); i-- > 0;) {
        ProcessId process = m_processesTitle[i];
        windowid_t window = XLibUtil::findWindow(process.checkNormality, process.searchPattern, m_manager->dockedWindows());
        if (window != 0) {
            emit windowFound(window, process.config);
            m_processesTitle.remove(i);
        } else if (process.etimer.hasExpired(process.timeout)) {
            QMessageBox box;
            box.setText(tr("Could not find a window matching '%1'").arg(process.searchPattern.pattern()));
            box.setWindowIcon(QPixmap(":/logo/kdocker.png"));
            box.setIcon(QMessageBox::Information);
            box.setStandardButtons(QMessageBox::Ok);
            box.exec();

            m_processesTitle.remove(i);
        }
    }
}

void Scanner::check()
{
    checkPid();
    checkTitle();

    if (m_processesPid.isEmpty() && m_processesTitle.isEmpty()) {
        m_timer.stop();
        emit stopping();
    }
}
