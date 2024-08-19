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


#ifndef _COMMAND_H
#define	_COMMAND_H

//#include <QDBusMetaType>
#include <QDBusArgument>
#include <QMetaType>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

class Command {
    public:
        enum class CommandType {
            NoCommand,
            Title,
            WindowId,
            Pid,
            Run,
            Select,
            Focused
        };

        Command();

        friend QDBusArgument &operator<<(QDBusArgument &argument, const Command &command);
        friend const QDBusArgument &operator>>(const QDBusArgument &argument, Command &command);

        Command::CommandType getType() const;
        QRegularExpression getSearchPattern() const;
        uint getWindowId() const;
        uint getPid() const;
        QString getRunApp() const;
        QStringList getRunAppArguments() const;
        uint getTimeout() const;
        bool getCheckNormality() const;

        void setType(Command::CommandType type);
        void setSearchPattern(const QRegularExpression &pattern);
        void setWindowId(uint wid);
        void setPid(uint pid);
        void setRunApp(const QString &app);
        void setRunAppArguments(const QStringList &args);
        void setTimeout(uint v);
        void setCheckNormality(bool v);

    private:
        Command::CommandType m_type;
        QRegularExpression m_searchPattern;
        uint m_windowId;
        uint m_pid;
        QString m_runApp;
        QStringList m_runAppArguments;
        uint m_timeout;
        bool m_checkNormality;
};

Q_DECLARE_METATYPE(Command::CommandType)
Q_DECLARE_METATYPE(Command)
//qDBusRegisterMetaType<Command>();

#endif // _COMMAND_H
