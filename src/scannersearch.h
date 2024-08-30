/*
 *  Copyright (C) 2024 John Schember <john@nachtimwald.com>
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

#ifndef _SCANNERSEARCH_H
#define _SCANNERSEARCH_H

#include "trayitemoptions.h"
#include "xlibtypes.h"

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QString>

class ScannerSearch
{
public:
    ScannerSearch(const TrayItemOptions &config, uint64_t timeout, bool checkNormality);

    const TrayItemOptions &config();
    bool checkNormality();
    bool hasExpired();

private:
    TrayItemOptions m_config;
    bool m_checkNormality;

    QElapsedTimer m_etimer;
    uint64_t m_timeout;
};

class ScannerSearchPid : public ScannerSearch
{
public:
    ScannerSearchPid(const QString &launchCommand, pid_t pid, const TrayItemOptions &config, uint64_t timeout,
                     bool checkNormality);

    const QString launchCommand();
    pid_t pid();

private:
    QString m_launchCommand;
    pid_t m_pid;
};

class ScannerSearchTitle : public ScannerSearch
{
public:
    ScannerSearchTitle(const QRegularExpression &searchPattern, const TrayItemOptions &config, uint64_t timeout,
                       bool checkNormality);

    const QRegularExpression &searchPattern();

private:
    QRegularExpression m_searchPattern;
};

#endif // _SCANNERSEARCH_H
