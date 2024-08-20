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


#include "command.h"

Command::Command() :
    m_type(Command::Type::NoCommand),
    m_windowId(0),
    m_pid(0),
    m_timeout(4),
    m_checkNormality(true)
{
}

Command::Type Command::getType() const {
    return m_type;
}

QString Command::getSearchPattern() const {
    return m_searchPattern;
}

uint32_t Command::getWindowId() const {
    return m_windowId;
}

pid_t Command::getPid() const {
    return m_pid;
}

QString Command::getLaunchApp() const {
    return m_launchApp;
}

QStringList Command::getLaunchAppArguments() const {
    return m_launchAppArguments;
}

bool Command::getCheckNormality() const {
    return m_checkNormality;
}

uint32_t Command::getTimeout() const {
    return m_timeout;
}

void Command::setType(Command::Type type) {
    m_type = type;
}

void Command::setSearchPattern(const QString &pattern) {
    m_searchPattern = pattern;
}

void Command::setWindowId(uint32_t wid) {
    m_windowId = wid;
}

void Command::setPid(pid_t pid) {
    m_pid = pid;
}

void Command::setLaunchApp(const QString &app) {
    m_launchApp = app;
}

void Command::setLaunchAppArguments(const QStringList &args) {
    m_launchAppArguments = args;
}

void Command::setTimeout(uint32_t v) {
    m_timeout = v;
}

void Command::setCheckNormality(bool v) {
    m_checkNormality = v;
}
