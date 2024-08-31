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

#ifndef _TRAYITEMOPTIONS
#define _TRAYITEMOPTIONS

#include <QDBusArgument>
#include <QString>

class TrayItemOptions
{
public:
    enum class TriState
    {
        Unset = -1,
        SetTrue = true,
        SetFalse = false
    };

    TrayItemOptions();
    ~TrayItemOptions() {};
    TrayItemOptions(const TrayItemOptions &other);
    TrayItemOptions &operator=(const TrayItemOptions &other);

    friend QDBusArgument &operator<<(QDBusArgument &argument, const TrayItemOptions &options);
    friend const QDBusArgument &operator>>(const QDBusArgument &argument, TrayItemOptions &options);

    QString getIconPath() const;
    QString getAttentionIconPath() const;
    bool getNotifyTimeState() const;

    TrayItemOptions::TriState getIconifyFocusLostState() const;
    TrayItemOptions::TriState getIconifyMinimizedState() const;
    TrayItemOptions::TriState getIconifyObscuredState() const;
    TrayItemOptions::TriState getQuietState() const;
    TrayItemOptions::TriState getSkipPagerState() const;
    TrayItemOptions::TriState getStickyState() const;
    TrayItemOptions::TriState getSkipTaskbarState() const;
    TrayItemOptions::TriState getLockToDesktopState() const;

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
    void setIconifyFocusLost(TrayItemOptions::TriState v);
    void setIconifyMinimized(TrayItemOptions::TriState v);
    void setIconifyObscured(TrayItemOptions::TriState v);
    void setQuiet(TrayItemOptions::TriState v);
    void setSkipPager(TrayItemOptions::TriState v);
    void setSticky(TrayItemOptions::TriState v);
    void setSkipTaskbar(TrayItemOptions::TriState v);
    void setLockToDesktop(TrayItemOptions::TriState v);

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
    TrayItemOptions::TriState m_iconifyFocusLost;
    TrayItemOptions::TriState m_iconifyMinimized;
    TrayItemOptions::TriState m_iconifyObscured;
    int m_notifyTime; // In milliseconds
    TrayItemOptions::TriState m_quiet;
    TrayItemOptions::TriState m_skipPager;
    TrayItemOptions::TriState m_sticky;
    TrayItemOptions::TriState m_skipTaskbar;
    TrayItemOptions::TriState m_lockToDesktop;
};

Q_DECLARE_METATYPE(TrayItemOptions)

#endif // _TRAYITEMOPTIONS
