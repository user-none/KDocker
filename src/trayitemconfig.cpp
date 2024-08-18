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

TrayItemConfig::TrayItemConfig(const TrayItemConfig &obj) {
    m_iconPath = obj.m_iconPath;
    m_attentionIconPath = obj.m_attentionIconPath;
    m_iconifyFocusLost = obj.m_iconifyFocusLost;
    m_iconifyMinimized = obj.m_iconifyMinimized;
    m_iconifyObscured = obj.m_iconifyObscured;
    m_notifyTime = obj.m_notifyTime;
    m_quiet = obj.m_quiet;
    m_skipPager = obj.m_skipPager;
    m_sticky = obj.m_sticky;
    m_skipTaskbar = obj.m_skipTaskbar;
    m_lockToDesktop = obj.m_lockToDesktop;
}

TrayItemConfig& TrayItemConfig::operator=(const TrayItemConfig &obj) {
    if (this == &obj)
        return *this;

    m_iconPath = obj.m_iconPath;
    m_attentionIconPath = obj.m_attentionIconPath;
    m_iconifyFocusLost = obj.m_iconifyFocusLost;
    m_iconifyMinimized = obj.m_iconifyMinimized;
    m_iconifyObscured = obj.m_iconifyObscured;
    m_notifyTime = obj.m_notifyTime;
    m_quiet = obj.m_quiet;
    m_skipPager = obj.m_skipPager;
    m_sticky = obj.m_sticky;
    m_skipTaskbar = obj.m_skipTaskbar;
    m_lockToDesktop = obj.m_lockToDesktop;
    return *this;
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
