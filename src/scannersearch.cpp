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

#include "scannersearch.h"

ScannerSearch::ScannerSearch(const TrayItemOptions &config, uint64_t timeout, bool checkNormality)
    : m_config(config), m_checkNormality(checkNormality)
{
    m_timeout = timeout * 1000;
    m_etimer.start();
}

const TrayItemOptions &ScannerSearch::config()
{
    return m_config;
}

bool ScannerSearch::checkNormality()
{
    return m_checkNormality;
}

bool ScannerSearch::hasExpired()
{
    return m_etimer.hasExpired(m_timeout);
}

ScannerSearchPid::ScannerSearchPid(const QString &launchCommand, pid_t pid, const TrayItemOptions &config,
                                   uint64_t timeout, bool checkNormality)
    : ScannerSearch(config, timeout, checkNormality), m_launchCommand(launchCommand), m_pid(pid)
{}

const QString ScannerSearchPid::launchCommand()
{
    return m_launchCommand;
}

pid_t ScannerSearchPid::pid()
{
    return m_pid;
}

ScannerSearchTitle::ScannerSearchTitle(const QRegularExpression &searchPattern, const TrayItemOptions &config,
                                       uint64_t timeout, bool checkNormality)
    : ScannerSearch(config, timeout, checkNormality), m_searchPattern(searchPattern)
{}

const QRegularExpression &ScannerSearchTitle::searchPattern()
{
    return m_searchPattern;
}
