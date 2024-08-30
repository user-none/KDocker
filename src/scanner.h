/*
 *  Copyright (C) 2009, 2012, 2015 John Schember <john@nachtimwald.com>
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

#ifndef _SCANNER_H
#define _SCANNER_H

#include "scannersearch.h"
#include "trayitemoptions.h"

#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QTimer>

// Forward declared because trayitemmanager.h incldues scanner.h
class TrayItemManager;

// Launches commands and looks for the window ids they create.
// Looks for windows based on a search pattern.
class Scanner : public QObject
{
    Q_OBJECT

public:
    Scanner(TrayItemManager *manager);
    void enqueueSearch(const QRegularExpression &searchPattern, quint32 maxTime, bool checkNormality,
                       const TrayItemOptions &config);
    void enqueueLaunch(const QString &launchCommand, const QStringList &arguments,
                       const QRegularExpression &searchPattern, quint32 maxTime, bool checkNormality,
                       const TrayItemOptions &config);
    bool isRunning();

private slots:
    void check();
    void checkPid();
    void checkTitle();

signals:
    void windowFound(windowid_t, const TrayItemOptions &);
    void stopping();

private:
    TrayItemManager *m_manager;
    QTimer m_timer;
    QList<ScannerSearchPid> m_searchPid;
    QList<ScannerSearchTitle> m_searchTitle;
};

#endif // _SCANNER_H
