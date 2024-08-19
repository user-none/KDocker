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

// DBus string keys for command settings
static const char *DKEY_TYPE  = "type";
static const char *DKEY_PAT   = "pattern";
static const char *DKEY_CASE  = "case-sensitive";
static const char *DKEY_RUN   = "run";
static const char *DKEY_RARGS = "run-argument";
static const char *DKEY_WID   = "wid";
static const char *DKEY_PID   = "pid";
static const char *DKEY_TO    = "timeout";
static const char *DKEY_BLIND = "blind";
// DBus string keys for command type
static const char *DTVAL_NONE = "nocommand";
static const char *DTVAL_TITL = "title";
static const char *DTVAL_WID  = "wndowid";
static const char *DTVAL_PID  = "pid";
static const char *DTVAL_RUN  = "run";
static const char *DTVAL_SEL  = "select";
static const char *DTVAL_FOC  = "focused";

Command::Command() :
    m_type(Command::CommandType::NoCommand),
    m_windowId(0),
    m_pid(0),
    m_timeout(4),
    m_checkNormality(true)
{
    m_searchPattern.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
}

QDBusArgument &operator<<(QDBusArgument &argument, const Command &command) {
    argument.beginMap(QMetaType::fromType<QString>(), QMetaType::fromType<QString>());

    argument.beginMapEntry();
    switch (command.m_type) {
        case Command::CommandType::NoCommand:
            argument << DKEY_TYPE << DTVAL_NONE;
            break;
        case Command::CommandType::Title:
            argument << DKEY_TYPE << DTVAL_TITL;
            break;
        case Command::CommandType::WindowId:
            argument << DKEY_TYPE << DTVAL_WID;
            break;
        case Command::CommandType::Pid:
            argument << DKEY_TYPE << DTVAL_PID;
            break;
        case Command::CommandType::Run:
            argument << DKEY_TYPE << DTVAL_RUN;
            break;
        case Command::CommandType::Select:
            argument << DKEY_TYPE << DTVAL_SEL;
            break;
        case Command::CommandType::Focused:
            argument << DKEY_TYPE << DTVAL_FOC;
            break;

    }
    argument.endMapEntry();

    if (command.m_type == Command::CommandType::Title || command.m_type == Command::CommandType::Run) {
        if (command.m_searchPattern.isValid()) {
            argument.beginMapEntry();
            argument << DKEY_PAT << command.m_searchPattern.pattern();
            argument.endMapEntry();

            argument.beginMapEntry();
            argument << DKEY_CASE << ((command.m_searchPattern.patternOptions() & QRegularExpression::CaseInsensitiveOption) ? "false" : "true");
            argument.endMapEntry();
        }
    }

    if (command.m_type == Command::CommandType::Run) {
        if (!command.m_runApp.isEmpty()) {
            argument.beginMapEntry();
            argument << DKEY_RUN << command.m_runApp;
            argument.endMapEntry();
        }

        if (!command.m_runAppArguments.isEmpty()) {
            argument.beginMapEntry();
            argument << DKEY_RARGS << command.m_runAppArguments.join(' ');
            argument.endMapEntry();
        }
    }

    if (command.m_type == Command::CommandType::WindowId && command.m_windowId != 0) {
        argument.beginMapEntry();
        argument << DKEY_WID << QString::number(command.m_windowId);
        argument.endMapEntry();
    }

    if (command.m_type == Command::CommandType::Pid && command.m_pid != 0) {
        argument.beginMapEntry();
        argument << DKEY_PID << QString::number(command.m_pid);
        argument.endMapEntry();
    }

    argument.beginMapEntry();
    argument << DKEY_TO << QString::number(command.m_timeout);
    argument.endMapEntry();

    argument.endMapEntry();
    argument << DKEY_BLIND << QVariant(!command.m_checkNormality).toString();
    argument.endMapEntry();

    argument.endMap();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, Command &command) {
    bool ok;

    argument.beginMap();

    while (!argument.atEnd()) {
        QString key;
        QString val;

        argument.beginMapEntry();
        argument >> key >> val;
        argument.endMapEntry();

        if (QString::compare(key, DKEY_TYPE, Qt::CaseInsensitive) == 0) {
            if (QString::compare(val, DTVAL_TITL, Qt::CaseInsensitive) == 0) {
                command.m_type = Command::CommandType::Title;
            } else if (QString::compare(val, DTVAL_WID, Qt::CaseInsensitive) == 0) {
                command.m_type = Command::CommandType::WindowId;
            } else if (QString::compare(val, DTVAL_PID, Qt::CaseInsensitive) == 0) {
                command.m_type = Command::CommandType::Pid;
            } else if (QString::compare(val, DTVAL_RUN, Qt::CaseInsensitive) == 0) {
                command.m_type = Command::CommandType::Run;
            } else if (QString::compare(val, DTVAL_SEL, Qt::CaseInsensitive) == 0) {
                command.m_type = Command::CommandType::Select;
            } else if (QString::compare(val, DTVAL_FOC, Qt::CaseInsensitive) == 0) {
                command.m_type = Command::CommandType::Focused;
            } else {
                command.m_type = Command::CommandType::NoCommand;
            }
        } else if (QString::compare(key, DKEY_PAT, Qt::CaseInsensitive) == 0) {
            command.m_searchPattern.setPattern(val);
        } else if (QString::compare(key, DKEY_CASE, Qt::CaseInsensitive) == 0) {
            command.m_searchPattern.setPatternOptions(QRegularExpression::NoPatternOption);
        } else if (QString::compare(key, DKEY_RUN, Qt::CaseInsensitive) == 0) {
            command.m_runApp = val;
        } else if (QString::compare(key, DKEY_RARGS, Qt::CaseInsensitive) == 0) {
            command.m_runAppArguments = val.split(' ');
        } else if (QString::compare(key, DKEY_WID, Qt::CaseInsensitive) == 0) {
            command.m_windowId = val.toInt(&ok);
        } else if (QString::compare(key, DKEY_PID, Qt::CaseInsensitive) == 0) {
            command.m_pid = val.toInt(&ok);
        } else if (QString::compare(key, DKEY_TO, Qt::CaseInsensitive) == 0) {
            command.m_timeout = val.toInt(&ok);
        } else if (QString::compare(key, DKEY_BLIND, Qt::CaseInsensitive) == 0) {
            command.m_checkNormality = !QVariant(val).toBool();
        }
    }

    argument.endMap();
    return argument;
}

Command::CommandType Command::getType() const {
    return m_type;
}

QRegularExpression Command::getSearchPattern() const {
    return m_searchPattern;
}

uint Command::getWindowId() const {
    return m_windowId;
}

uint Command::getPid() const {
    return m_pid;
}

QString Command::getRunApp() const {
    return m_runApp;
}

QStringList Command::getRunAppArguments() const {
    return m_runAppArguments;
}

bool Command::getCheckNormality() const {
    return m_checkNormality;
}

uint Command::getTimeout() const {
    return m_timeout;
}

void Command::setType(Command::CommandType type) {
    m_type = type;
}

void Command::setSearchPattern(const QRegularExpression &pattern) {
    m_searchPattern = pattern;
}

void Command::setWindowId(uint wid) {
    m_windowId = wid;
}

void Command::setPid(uint pid) {
    m_pid = pid;
}

void Command::setRunApp(const QString &app) {
    m_runApp = app;
}

void Command::setRunAppArguments(const QStringList &args) {
    m_runAppArguments = args;
}

void Command::setTimeout(uint v) {
    m_timeout = v;
}

void Command::setCheckNormality(bool v) {
    m_checkNormality = v;
}
