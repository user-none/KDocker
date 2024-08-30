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
#include <QElapsedTimer>
#include <QMessageBox>
#include <QProcess>
#include <QStringList>

#include <signal.h>

Scanner::Scanner(TrayItemManager *manager)
{
    m_manager = manager;
    // Check every 1/4 second for a window
    m_timer.setInterval(250);
    connect(&m_timer, &QTimer::timeout, this, &Scanner::check);
}

void Scanner::enqueueSearch(const QRegularExpression &searchPattern, quint32 maxTime, bool checkNormality,
                            const TrayItemOptions &config)
{
    if (maxTime == 0)
        maxTime = 1;

    m_searchTitle.append(ScannerSearchTitle(searchPattern, config, maxTime * 1000, checkNormality));
    m_timer.start();
}

void Scanner::enqueueLaunch(const QString &launchCommand, const QStringList &arguments,
                            const QRegularExpression &searchPattern, quint32 maxTime, bool checkNormality,
                            const TrayItemOptions &config)
{
    if (maxTime == 0)
        maxTime = 1;

    // Launch the requested application.
    qint64 pid;
    if (!QProcess::startDetached(launchCommand, arguments, "", &pid)) {
        QMessageBox box;
        box.setText(tr("'%1' did not start properly.").arg(launchCommand));
        box.setWindowIcon(QPixmap(":/logo/kdocker.png"));
        box.setIcon(QMessageBox::Information);
        box.setStandardButtons(QMessageBox::Ok);
        box.exec();
        return;
    }

    if (!searchPattern.pattern().isEmpty()) {
        m_searchTitle.append(ScannerSearchTitle(searchPattern, config, maxTime * 1000, checkNormality));
    } else {
        m_searchPid.append(
            ScannerSearchPid(launchCommand, static_cast<pid_t>(pid), config, maxTime * 1000, checkNormality));
    }
    m_timer.start();
}

bool Scanner::isRunning()
{
    return !m_searchPid.isEmpty() || !m_searchTitle.isEmpty();
}

void Scanner::checkPid()
{
    // Counting backwards because we can remove items from the list
    for (size_t i = m_searchPid.count(); i-- > 0;) {
        ScannerSearchPid &search = m_searchPid[i];
        windowid_t window = XLibUtil::pidToWid(search.checkNormality, search.pid);
        if (window != 0) {
            emit windowFound(window, search.config);
            m_searchPid.remove(i);
        } else if (search.etimer.hasExpired(search.timeout)) {
            QMessageBox box;
            box.setText(tr("Could not find a window for '%1'").arg(search.launchCommand));
            box.setWindowIcon(QPixmap(":/logo/kdocker.png"));
            box.setIcon(QMessageBox::Information);
            box.setStandardButtons(QMessageBox::Ok);
            box.exec();

            m_searchPid.remove(i);
        }
    }
}

void Scanner::checkTitle()
{
    // Counting backwards because we can remove items from the list
    for (size_t i = m_searchTitle.count(); i-- > 0;) {
        ScannerSearchTitle &search = m_searchTitle[i];
        windowid_t window =
            XLibUtil::findWindow(search.checkNormality, search.searchPattern, m_manager->dockedWindows());
        if (window != 0) {
            emit windowFound(window, search.config);
            m_searchTitle.remove(i);
        } else if (search.etimer.hasExpired(search.timeout)) {
            QMessageBox box;
            box.setText(tr("Could not find a window matching '%1'").arg(search.searchPattern.pattern()));
            box.setWindowIcon(QPixmap(":/logo/kdocker.png"));
            box.setIcon(QMessageBox::Information);
            box.setStandardButtons(QMessageBox::Ok);
            box.exec();

            m_searchTitle.remove(i);
        }
    }
}

void Scanner::check()
{
    checkPid();
    checkTitle();

    if (m_searchPid.isEmpty() && m_searchTitle.isEmpty()) {
        m_timer.stop();
        emit stopping();
    }
}
