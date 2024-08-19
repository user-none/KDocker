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


#ifndef _TRAYITEMCONFIG
#define	_TRAYITEMCONFIG

#include <QDBusArgument>
#include <QMetaType>
#include <QString>

class TrayItemConfig {
    public:
        enum class TriState {
            Unset = -1,
            SetTrue = true,
            SetFalse = false
        };

        TrayItemConfig();
        ~TrayItemConfig() { };
        TrayItemConfig(const TrayItemConfig &other);
        TrayItemConfig& operator=(const TrayItemConfig &other);

        friend QDBusArgument &operator<<(QDBusArgument &argument, const TrayItemConfig &config);
        friend const QDBusArgument &operator>>(const QDBusArgument &argument, TrayItemConfig &config);

        QString getIconPath() const;
        QString getAttentionIconPath() const;
        TrayItemConfig::TriState getIconifyFocusLostState() const;
        TrayItemConfig::TriState getIconifyMinimizedState() const;
        TrayItemConfig::TriState getIconifyObscuredState() const;
        int getNotifyTimeState() const;
        TrayItemConfig::TriState getQuietState() const;
        TrayItemConfig::TriState getSkipPagerState() const;
        TrayItemConfig::TriState getStickyState() const;
        TrayItemConfig::TriState getSkipTaskbarState() const;
        TrayItemConfig::TriState getLockToDesktopState() const;

        bool getIconifyFocusLost() const;
        bool getIconifyMinimized() const;
        bool getIconifyObscured() const;
        int getNotifyTime() const;
        bool getQuiet() const;
        bool getSkipPager() const;
        bool getSticky() const;
        bool getSkipTaskbar() const;
        bool getLockToDesktop() const;

        void setIconPath(const QString &v);
        void setAttentionIconPath(const QString &v);
        void setIconifyFocusLost(TrayItemConfig::TriState v);
        void setIconifyMinimized(TrayItemConfig::TriState v);
        void setIconifyObscured(TrayItemConfig::TriState v);
        void setQuiet(TrayItemConfig::TriState v);
        void setSkipPager(TrayItemConfig::TriState v);
        void setSticky(TrayItemConfig::TriState v);
        void setSkipTaskbar(TrayItemConfig::TriState v);
        void setLockToDesktop(TrayItemConfig::TriState v);

        void setIconifyFocusLost(bool v);
        void setIconifyMinimized(bool v);
        void setIconifyObscured(bool v);
        void setNotifyTime(int v);
        void setQuiet(bool v);
        void setSkipPager(bool v);
        void setSticky(bool v);
        void setSkipTaskbar(bool v);
        void setLockToDesktop(bool v);

        static QString defaultIconPath();
        static QString defaultAttentionIconPath();
        static bool defaultIconifyFocusLost();
        static bool defaultIconifyMinimized();
        static bool defaultIconifyObscured();
        static int defaultNotifyTime();
        static bool defaultQuiet();
        static bool defaultSkipPager();
        static bool defaultSticky();
        static bool defaultSkipTaskbar();
        static bool defaultLockToDesktop();

    private:
        QString m_iconPath;
        QString m_attentionIconPath;
        TrayItemConfig::TriState m_iconifyFocusLost;
        TrayItemConfig::TriState m_iconifyMinimized;
        TrayItemConfig::TriState m_iconifyObscured;
        int m_notifyTime; // In milliseconds
        TrayItemConfig::TriState m_quiet;
        TrayItemConfig::TriState m_skipPager;
        TrayItemConfig::TriState m_sticky;
        TrayItemConfig::TriState m_skipTaskbar;
        TrayItemConfig::TriState m_lockToDesktop;
};

Q_DECLARE_METATYPE(TrayItemConfig)

#endif // _TRAYITEMCONFIG
