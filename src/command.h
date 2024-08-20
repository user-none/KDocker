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

#include <QDBusArgument>
#include <QString>
#include <QStringList>

class Command {
    public:
        enum class Type {
            NoCommand = 0,
            Title,
            WindowId,
            Pid,
            Launch,
            Select,
            Focused
        };

        Command();

        friend QDBusArgument &operator<<(QDBusArgument &argument, const Command &command);
        friend const QDBusArgument &operator>>(const QDBusArgument &argument, Command &command);

        Command::Type getType() const;
        QString getSearchPattern() const;
        uint32_t getWindowId() const;
        pid_t getPid() const;
        QString getLaunchApp() const;
        QStringList getLaunchAppArguments() const;
        uint32_t getTimeout() const;
        bool getCheckNormality() const;

        void setType(Command::Type type);
        void setSearchPattern(const QString &pattern);
        void setWindowId(uint32_t wid);
        void setPid(pid_t pid);
        void setLaunchApp(const QString &app);
        void setLaunchAppArguments(const QStringList &args);
        void setTimeout(uint32_t v);
        void setCheckNormality(bool v);

    private:
        Command::Type m_type;
        QString m_searchPattern;
        uint32_t m_windowId;
        uint32_t m_pid;
        QString m_launchApp;
        QStringList m_launchAppArguments;
        uint32_t m_timeout;
        bool m_checkNormality;
};

Q_DECLARE_METATYPE(Command::Type)
Q_DECLARE_METATYPE(Command)
//qDBusRegisterMetaType<Command>();

#endif // _COMMAND_H
