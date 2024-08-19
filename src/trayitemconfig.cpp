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


#include "trayitemconfig.h"

static const char *DKEY_ICONP    = "icon";
static const char *DKEY_AICOP    = "attention-icon";
static const char *DKEY_ICONFFOC = "iconify-focus-lost";
static const char *DKEY_ICONFMIN = "iconify-minimized";
static const char *DKEY_ICONFOBS = "iconify-obscured";
static const char *DKEY_NOTIFYT  = "notify-time";
static const char *DKEY_QUIET    = "quiet";
static const char *DKEY_SKPAG    = "skip-pager";
static const char *DKEY_STICKY   = "sticky";
static const char *DKEY_SKTASK   = "skip-taskbar";

TrayItemConfig::TrayItemConfig() :
        m_iconifyFocusLost(TrayItemConfig::TriState::Unset),
        m_iconifyMinimized(TrayItemConfig::TriState::Unset),
        m_iconifyObscured(TrayItemConfig::TriState::Unset),
        m_notifyTime(-1),
        m_quiet(TrayItemConfig::TriState::Unset),
        m_skipPager(TrayItemConfig::TriState::Unset),
        m_sticky(TrayItemConfig::TriState::Unset),
        m_skipTaskbar(TrayItemConfig::TriState::Unset),
        m_lockToDesktop(TrayItemConfig::TriState::Unset)
{
}

TrayItemConfig::TrayItemConfig(const TrayItemConfig &other) {
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

TrayItemConfig& TrayItemConfig::operator=(const TrayItemConfig &other) {
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

QDBusArgument &operator<<(QDBusArgument &argument, const TrayItemConfig &config) {
    argument.beginMap(QMetaType::fromType<QString>(), QMetaType::fromType<QString>());

    if (!config.m_iconPath.isEmpty()) {
        argument.beginMapEntry();
        argument << DKEY_ICONP << config.m_iconPath;
        argument.endMapEntry();
    }

    if (!config.m_attentionIconPath.isEmpty()) {
        argument.beginMapEntry();
        argument << DKEY_AICOP << config.m_attentionIconPath;
        argument.endMapEntry();
    }

    if (config.m_iconifyFocusLost != TrayItemConfig::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_ICONFFOC << ((config.m_iconifyFocusLost == TrayItemConfig::TriState::SetTrue) ? "true" : "false");
        argument.endMapEntry();
    }

    if (config.m_iconifyMinimized != TrayItemConfig::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_ICONFMIN << ((config.m_iconifyMinimized == TrayItemConfig::TriState::SetTrue) ? "true" : "false");
        argument.endMapEntry();
    }

    if (config.m_iconifyObscured != TrayItemConfig::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_ICONFOBS << ((config.m_iconifyObscured == TrayItemConfig::TriState::SetTrue) ? "true" : "false");
        argument.endMapEntry();
    }

    if (config.m_notifyTime > -1) {
        argument.beginMapEntry();
        argument << DKEY_NOTIFYT << QString::number(config.m_notifyTime / 1000);
        argument.endMapEntry();
    }

    if (config.m_quiet != TrayItemConfig::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_QUIET << ((config.m_quiet == TrayItemConfig::TriState::SetTrue) ? "true" : "false");
        argument.endMapEntry();
    }

    if (config.m_skipPager != TrayItemConfig::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_SKPAG << ((config.m_skipPager == TrayItemConfig::TriState::SetTrue) ? "true" : "false");
        argument.endMapEntry();
    }

    if (config.m_skipTaskbar != TrayItemConfig::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_SKTASK << ((config.m_skipTaskbar == TrayItemConfig::TriState::SetTrue) ? "true" : "false");
        argument.endMapEntry();
    }

    if (config.m_sticky != TrayItemConfig::TriState::Unset) {
        argument.beginMapEntry();
        argument << DKEY_STICKY << ((config.m_sticky == TrayItemConfig::TriState::SetTrue) ? "true" : "false");
        argument.endMapEntry();
    }

    argument.endMap();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, TrayItemConfig &config) {
    argument.beginMap();

    while (!argument.atEnd()) {
        QString key;
        QString val;

        argument.beginMapEntry();
        argument >> key >> val;
        argument.endMapEntry();

        if (QString::compare(key, DKEY_ICONP, Qt::CaseInsensitive) == 0) {
            config.m_iconPath = val;
        } else if (QString::compare(key, DKEY_AICOP, Qt::CaseInsensitive) == 0) {
            config.m_attentionIconPath = val;
        } else if (QString::compare(key, DKEY_ICONFFOC, Qt::CaseInsensitive) == 0) {
            config.m_iconifyFocusLost = QVariant(val).toBool() ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_ICONFMIN, Qt::CaseInsensitive) == 0) {
            config.m_iconifyMinimized = QVariant(val).toBool() ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_ICONFOBS, Qt::CaseInsensitive) == 0) {
            config.m_iconifyObscured = QVariant(val).toBool() ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_NOTIFYT, Qt::CaseInsensitive) == 0) {
            bool ok;
            config.m_notifyTime = val.toInt(&ok) * 1000;
        } else if (QString::compare(key, DKEY_QUIET, Qt::CaseInsensitive) == 0) {
            config.m_quiet = QVariant(val).toBool() ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_SKPAG, Qt::CaseInsensitive) == 0) {
            config.m_skipPager = QVariant(val).toBool() ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_STICKY, Qt::CaseInsensitive) == 0) {
            config.m_sticky = QVariant(val).toBool() ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;
        } else if (QString::compare(key, DKEY_SKTASK, Qt::CaseInsensitive) == 0) {
            config.m_skipTaskbar = QVariant(val).toBool() ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;
        }
    }

    argument.endMap();
    return argument;
}

QString TrayItemConfig::getIconPath() const {
    return m_iconPath;
}

QString TrayItemConfig::getAttentionIconPath() const {
    return m_attentionIconPath;
}

TrayItemConfig::TriState TrayItemConfig::getIconifyFocusLostState() const {
    return m_iconifyFocusLost;
}

TrayItemConfig::TriState TrayItemConfig::getIconifyMinimizedState() const {
    return m_iconifyMinimized;
}

TrayItemConfig::TriState TrayItemConfig::getIconifyObscuredState() const {
    return m_iconifyObscured;
}

TrayItemConfig::TriState TrayItemConfig::getQuietState() const {
    return m_quiet;
}

TrayItemConfig::TriState TrayItemConfig::getSkipPagerState() const {
    return m_skipPager;
}

TrayItemConfig::TriState TrayItemConfig::getStickyState() const {
    return m_sticky;
}

TrayItemConfig::TriState TrayItemConfig::getSkipTaskbarState() const {
    return m_skipTaskbar;
}

TrayItemConfig::TriState TrayItemConfig::getLockToDesktopState() const {
    return m_lockToDesktop;
}

bool TrayItemConfig::getIconifyFocusLost() const {
    switch (m_iconifyFocusLost) {
        case TrayItemConfig::TriState::Unset:
            return defaultIconifyFocusLost();
        case TrayItemConfig::TriState::SetTrue:
            return true;
        case TrayItemConfig::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemConfig::getIconifyMinimized() const {
    switch (m_iconifyMinimized) {
        case TrayItemConfig::TriState::Unset:
            return defaultIconifyMinimized();
        case TrayItemConfig::TriState::SetTrue:
            return true;
        case TrayItemConfig::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemConfig::getIconifyObscured() const {
    switch (m_iconifyObscured) {
        case TrayItemConfig::TriState::Unset:
            return defaultIconifyObscured();
        case TrayItemConfig::TriState::SetTrue:
            return true;
        case TrayItemConfig::TriState::SetFalse:
            return false;
    }
    return false;
}

int TrayItemConfig::getNotifyTime() const {
    return m_notifyTime;
}

bool TrayItemConfig::getQuiet() const {
    switch (m_quiet) {
        case TrayItemConfig::TriState::Unset:
            return defaultQuiet();
        case TrayItemConfig::TriState::SetTrue:
            return true;
        case TrayItemConfig::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemConfig::getSkipPager() const {
    switch (m_skipPager) {
        case TrayItemConfig::TriState::Unset:
            return defaultSkipPager();
        case TrayItemConfig::TriState::SetTrue:
            return true;
        case TrayItemConfig::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemConfig::getSticky() const {
    switch (m_sticky) {
        case TrayItemConfig::TriState::Unset:
            return defaultSticky();
        case TrayItemConfig::TriState::SetTrue:
            return true;
        case TrayItemConfig::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemConfig::getSkipTaskbar() const {
    switch (m_skipTaskbar) {
        case TrayItemConfig::TriState::Unset:
            return defaultSkipTaskbar();
        case TrayItemConfig::TriState::SetTrue:
            return true;
        case TrayItemConfig::TriState::SetFalse:
            return false;
    }
    return false;
}

bool TrayItemConfig::getLockToDesktop() const {
    switch (m_iconifyMinimized) {
        case TrayItemConfig::TriState::Unset:
            return defaultIconifyMinimized();
        case TrayItemConfig::TriState::SetTrue:
            return true;
        case TrayItemConfig::TriState::SetFalse:
            return false;
    }
    return false;
}

void TrayItemConfig::setIconPath(const QString &v) {
    m_iconPath = v;
}

void TrayItemConfig::setAttentionIconPath(const QString &v) {
    m_attentionIconPath = v;
}

void TrayItemConfig::setIconifyFocusLost(TrayItemConfig::TriState v) {
    m_iconifyFocusLost = v;
}

void TrayItemConfig::setIconifyMinimized(TrayItemConfig::TriState v) {
    m_iconifyMinimized = v;
}

void TrayItemConfig::setIconifyObscured(TrayItemConfig::TriState v) {
    m_iconifyObscured = v;
}

void TrayItemConfig::setNotifyTime(int v) {
    m_notifyTime = v;
}

void TrayItemConfig::setQuiet(TrayItemConfig::TriState v) {
    m_quiet = v;
}

void TrayItemConfig::setSkipPager(TrayItemConfig::TriState v) {
    m_skipPager = v;
}

void TrayItemConfig::setSticky(TrayItemConfig::TriState v) {
    m_sticky = v;
}

void TrayItemConfig::setSkipTaskbar(TrayItemConfig::TriState v) {
    m_skipTaskbar = v;
}

void TrayItemConfig::setLockToDesktop(TrayItemConfig::TriState v) {
    m_lockToDesktop = v;
}


void TrayItemConfig::setIconifyFocusLost(bool v) {
    m_iconifyFocusLost = v ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;
}

void TrayItemConfig::setIconifyMinimized(bool v) {
    m_iconifyMinimized = v ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;;
}

void TrayItemConfig::setIconifyObscured(bool v) {
    m_iconifyObscured = v ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;;
}

void TrayItemConfig::setQuiet(bool v) {
    m_quiet = v ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;;
}

void TrayItemConfig::setSkipPager(bool v) {
    m_skipPager = v ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;;
}

void TrayItemConfig::setSticky(bool v) {
    m_sticky = v ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;;
}

void TrayItemConfig::setSkipTaskbar(bool v) {
    m_skipTaskbar = v ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;;
}

void TrayItemConfig::setLockToDesktop(bool v) {
    m_lockToDesktop = v ? TrayItemConfig::TriState::SetTrue : TrayItemConfig::TriState::SetFalse;;
}

QString TrayItemConfig::defaultIconPath() {
    return QString();
}

QString TrayItemConfig::defaultAttentionIconPath() {
    return QString();
}

bool TrayItemConfig::defaultIconifyFocusLost() {
    return false;
}

bool TrayItemConfig::defaultIconifyMinimized() {
    return true;
}

bool TrayItemConfig::defaultIconifyObscured() {
    return false;
}

int TrayItemConfig::defaultNotifyTime() {
    return 4000; // 4 seconds
}

bool TrayItemConfig::defaultQuiet() {
    return false;
}

bool TrayItemConfig::defaultSkipPager() {
    return false;
}

bool TrayItemConfig::defaultSticky() {
    return false;
}

bool TrayItemConfig::defaultSkipTaskbar() {
    return false;
}

bool TrayItemConfig::defaultLockToDesktop() {
    return true;
}
