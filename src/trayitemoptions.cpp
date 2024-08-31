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

#include "trayitemoptions.h"

#include <QMetaType>

static const QString DKEY_ICONP = "icon";
static const QString DKEY_AICOP = "attention-icon";
static const QString DKEY_ICONFFOC = "iconify-focus-lost";
static const QString DKEY_ICONFMIN = "iconify-minimized";
static const QString DKEY_ICONFOBS = "iconify-obscured";
static const QString DKEY_NOTIFYT = "notify-time";
static const QString DKEY_QUIET = "quiet";
static const QString DKEY_SKPAG = "skip-pager";
static const QString DKEY_STICKY = "sticky";
static const QString DKEY_SKTASK = "skip-taskbar";

TrayItemOptions::TrayItemOptions()
    : m_iconifyFocusLost(TrayItemOptions::TriState::Unset), m_iconifyMinimized(TrayItemOptions::TriState::Unset),
      m_iconifyObscured(TrayItemOptions::TriState::Unset), m_notifyTime(-1), m_quiet(TrayItemOptions::TriState::Unset),
      m_skipPager(TrayItemOptions::TriState::Unset), m_sticky(TrayItemOptions::TriState::Unset),
      m_skipTaskbar(TrayItemOptions::TriState::Unset), m_lockToDesktop(TrayItemOptions::TriState::Unset)
{}

TrayItemOptions::TrayItemOptions(const TrayItemOptions &other)
{
    m_iconPath = other.m_iconPath;
    m_attentionIconPath = other.m_attentionIconPath;
    m_iconifyFocusLost = other.m_iconifyFocusLost;
    m_iconifyMinimized = other.m_iconifyMinimized;
    m_iconifyObscured = other.m_iconifyObscured;
    m_notifyTime = other.m_notifyTime;
    m_quiet = other.m_quiet;
    m_skipPager = other.m_skipPager;
    m_sticky = other.m_sticky;
    m_skipTaskbar = other.m_skipTaskbar;
    m_lockToDesktop = other.m_lockToDesktop;
}

TrayItemOptions &TrayItemOptions::operator=(const TrayItemOptions &other)
{
    if (this == &other)
        return *this;

    m_iconPath = other.m_iconPath;
    m_attentionIconPath = other.m_attentionIconPath;
    m_iconifyFocusLost = other.m_iconifyFocusLost;
    m_iconifyMinimized = other.m_iconifyMinimized;
    m_iconifyObscured = other.m_iconifyObscured;
    m_notifyTime = other.m_notifyTime;
    m_quiet = other.m_quiet;
    m_skipPager = other.m_skipPager;
    m_sticky = other.m_sticky;
    m_skipTaskbar = other.m_skipTaskbar;
    m_lockToDesktop = other.m_lockToDesktop;
    return *this;
}

QDBusArgument &operator<<(QDBusArgument &argument, const TrayItemOptions &options)
{
    argument.beginMap(QMetaType::fromType<QString>(), QMetaType::fromType<QString>());

    if (!options.m_iconPath.isEmpty()) {
        argument.beginMapEntry();
        argument << DKEY_ICONP << options.m_iconPath;
        argument.endMapEntry();
    }

    if (!options.m_attentionIconPath.isEmpty()) {
        argument.beginMapEntry();
        argument << DKEY_AICOP << options.m_attentionIconPath;
        argument.endMapEntry();
    }

    if (options.m_iconifyFocusLost != TrayItemOptions::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_ICONFFOC
                 << QVariant(options.m_iconifyFocusLost == TrayItemOptions::TriState::SetTrue).toString();
        argument.endMapEntry();
    }

    if (options.m_iconifyMinimized != TrayItemOptions::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_ICONFMIN
                 << QVariant(options.m_iconifyMinimized == TrayItemOptions::TriState::SetTrue).toString();
        argument.endMapEntry();
    }

    if (options.m_iconifyObscured != TrayItemOptions::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_ICONFOBS
                 << QVariant(options.m_iconifyObscured == TrayItemOptions::TriState::SetTrue).toString();
        argument.endMapEntry();
    }

    if (options.m_notifyTime > -1) {
        argument.beginMapEntry();
        argument << DKEY_NOTIFYT << QString::number(options.m_notifyTime / 1000);
        argument.endMapEntry();
    }

    if (options.m_quiet != TrayItemOptions::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_QUIET << QVariant(options.m_quiet == TrayItemOptions::TriState::SetTrue).toString();
        argument.endMapEntry();
    }

    if (options.m_skipPager != TrayItemOptions::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_SKPAG << QVariant(options.m_skipPager == TrayItemOptions::TriState::SetTrue).toString();
        argument.endMapEntry();
    }

    if (options.m_skipTaskbar != TrayItemOptions::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_SKTASK << QVariant(options.m_skipTaskbar == TrayItemOptions::TriState::SetTrue).toString();
        argument.endMapEntry();
    }

    if (options.m_sticky != TrayItemOptions::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_STICKY << QVariant(options.m_sticky == TrayItemOptions::TriState::SetTrue).toString();
        argument.endMapEntry();
    }

    argument.endMap();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, TrayItemOptions &options)
{
    argument.beginMap();

    while (!argument.atEnd()) {
        QString key;
        QString val;

        argument.beginMapEntry();
        argument >> key >> val;
        argument.endMapEntry();

        if (QString::compare(key, DKEY_ICONP, Qt::CaseInsensitive) == 0) {
            options.m_iconPath = val;
        } else if (QString::compare(key, DKEY_AICOP, Qt::CaseInsensitive) == 0) {
            options.m_attentionIconPath = val;
        } else if (QString::compare(key, DKEY_ICONFFOC, Qt::CaseInsensitive) == 0) {
            options.m_iconifyFocusLost =
                QVariant(val).toBool() ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_ICONFMIN, Qt::CaseInsensitive) == 0) {
            options.m_iconifyMinimized =
                QVariant(val).toBool() ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_ICONFOBS, Qt::CaseInsensitive) == 0) {
            options.m_iconifyObscured =
                QVariant(val).toBool() ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_NOTIFYT, Qt::CaseInsensitive) == 0) {
            bool ok;
            options.m_notifyTime = val.toInt(&ok) * 1000;
        } else if (QString::compare(key, DKEY_QUIET, Qt::CaseInsensitive) == 0) {
            options.m_quiet =
                QVariant(val).toBool() ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_SKPAG, Qt::CaseInsensitive) == 0) {
            options.m_skipPager =
                QVariant(val).toBool() ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_STICKY, Qt::CaseInsensitive) == 0) {
            options.m_sticky =
                QVariant(val).toBool() ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_SKTASK, Qt::CaseInsensitive) == 0) {
            options.m_skipTaskbar =
                QVariant(val).toBool() ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
        }
    }

    argument.endMap();
    return argument;
}

QString TrayItemOptions::getIconPath() const
{
    return m_iconPath;
}

QString TrayItemOptions::getAttentionIconPath() const
{
    return m_attentionIconPath;
}

TrayItemOptions::TriState TrayItemOptions::getIconifyFocusLostState() const
{
    return m_iconifyFocusLost;
}

TrayItemOptions::TriState TrayItemOptions::getIconifyMinimizedState() const
{
    return m_iconifyMinimized;
}

TrayItemOptions::TriState TrayItemOptions::getIconifyObscuredState() const
{
    return m_iconifyObscured;
}

bool TrayItemOptions::getNotifyTimeState() const
{
    return m_notifyTime == -1 ? false : true;
}

TrayItemOptions::TriState TrayItemOptions::getQuietState() const
{
    return m_quiet;
}

TrayItemOptions::TriState TrayItemOptions::getSkipPagerState() const
{
    return m_skipPager;
}

TrayItemOptions::TriState TrayItemOptions::getStickyState() const
{
    return m_sticky;
}

TrayItemOptions::TriState TrayItemOptions::getSkipTaskbarState() const
{
    return m_skipTaskbar;
}

TrayItemOptions::TriState TrayItemOptions::getLockToDesktopState() const
{
    return m_lockToDesktop;
}

bool TrayItemOptions::getIconifyFocusLost() const
{
    switch (m_iconifyFocusLost) {
        case TrayItemOptions::TriState::Unset:
            return defaultIconifyFocusLost();
        case TrayItemOptions::TriState::SetTrue:
            return true;
        case TrayItemOptions::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemOptions::getIconifyMinimized() const
{
    switch (m_iconifyMinimized) {
        case TrayItemOptions::TriState::Unset:
            return defaultIconifyMinimized();
        case TrayItemOptions::TriState::SetTrue:
            return true;
        case TrayItemOptions::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemOptions::getIconifyObscured() const
{
    switch (m_iconifyObscured) {
        case TrayItemOptions::TriState::Unset:
            return defaultIconifyObscured();
        case TrayItemOptions::TriState::SetTrue:
            return true;
        case TrayItemOptions::TriState::SetFalse:
            return false;
    }
    return false;
}

int TrayItemOptions::getNotifyTime() const
{
    if (m_quiet == TrayItemOptions::TriState::SetTrue)
        return 0;
    return m_notifyTime;
}

bool TrayItemOptions::getQuiet() const
{
    switch (m_quiet) {
        case TrayItemOptions::TriState::Unset:
            return defaultQuiet();
        case TrayItemOptions::TriState::SetTrue:
            return true;
        case TrayItemOptions::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemOptions::getSkipPager() const
{
    switch (m_skipPager) {
        case TrayItemOptions::TriState::Unset:
            return defaultSkipPager();
        case TrayItemOptions::TriState::SetTrue:
            return true;
        case TrayItemOptions::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemOptions::getSticky() const
{
    switch (m_sticky) {
        case TrayItemOptions::TriState::Unset:
            return defaultSticky();
        case TrayItemOptions::TriState::SetTrue:
            return true;
        case TrayItemOptions::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemOptions::getSkipTaskbar() const
{
    switch (m_skipTaskbar) {
        case TrayItemOptions::TriState::Unset:
            return defaultSkipTaskbar();
        case TrayItemOptions::TriState::SetTrue:
            return true;
        case TrayItemOptions::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemOptions::getLockToDesktop() const
{
    switch (m_iconifyMinimized) {
        case TrayItemOptions::TriState::Unset:
            return defaultIconifyMinimized();
        case TrayItemOptions::TriState::SetTrue:
            return true;
        case TrayItemOptions::TriState::SetFalse:
            return false;
    }
    return false;
}

void TrayItemOptions::setIconPath(const QString &v)
{
    m_iconPath = v;
}

void TrayItemOptions::setAttentionIconPath(const QString &v)
{
    m_attentionIconPath = v;
}

void TrayItemOptions::setIconifyFocusLost(TrayItemOptions::TriState v)
{
    m_iconifyFocusLost = v;
}

void TrayItemOptions::setIconifyMinimized(TrayItemOptions::TriState v)
{
    m_iconifyMinimized = v;
}

void TrayItemOptions::setIconifyObscured(TrayItemOptions::TriState v)
{
    m_iconifyObscured = v;
}

void TrayItemOptions::setNotifyTime(int v)
{
    m_notifyTime = v;
}

void TrayItemOptions::setQuiet(TrayItemOptions::TriState v)
{
    m_quiet = v;
}

void TrayItemOptions::setSkipPager(TrayItemOptions::TriState v)
{
    m_skipPager = v;
}

void TrayItemOptions::setSticky(TrayItemOptions::TriState v)
{
    m_sticky = v;
}

void TrayItemOptions::setSkipTaskbar(TrayItemOptions::TriState v)
{
    m_skipTaskbar = v;
}

void TrayItemOptions::setLockToDesktop(TrayItemOptions::TriState v)
{
    m_lockToDesktop = v;
}

void TrayItemOptions::setIconifyFocusLost(bool v)
{
    m_iconifyFocusLost = v ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
}

void TrayItemOptions::setIconifyMinimized(bool v)
{
    m_iconifyMinimized = v ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
}

void TrayItemOptions::setIconifyObscured(bool v)
{
    m_iconifyObscured = v ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
}

void TrayItemOptions::setQuiet(bool v)
{
    m_quiet = v ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
}

void TrayItemOptions::setSkipPager(bool v)
{
    m_skipPager = v ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
}

void TrayItemOptions::setSticky(bool v)
{
    m_sticky = v ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
}

void TrayItemOptions::setSkipTaskbar(bool v)
{
    m_skipTaskbar = v ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
}

void TrayItemOptions::setLockToDesktop(bool v)
{
    m_lockToDesktop = v ? TrayItemOptions::TriState::SetTrue : TrayItemOptions::TriState::SetFalse;
}

QString TrayItemOptions::defaultIconPath()
{
    return QString();
}

QString TrayItemOptions::defaultAttentionIconPath()
{
    return QString();
}

bool TrayItemOptions::defaultIconifyFocusLost()
{
    return false;
}

bool TrayItemOptions::defaultIconifyMinimized()
{
    return true;
}

bool TrayItemOptions::defaultIconifyObscured()
{
    return false;
}

int TrayItemOptions::defaultNotifyTime()
{
    return 4000; // 4 seconds
}

bool TrayItemOptions::defaultQuiet()
{
    return false;
}

bool TrayItemOptions::defaultSkipPager()
{
    return false;
}

bool TrayItemOptions::defaultSticky()
{
    return false;
}

bool TrayItemOptions::defaultSkipTaskbar()
{
    return false;
}

bool TrayItemOptions::defaultLockToDesktop()
{
    return true;
}
