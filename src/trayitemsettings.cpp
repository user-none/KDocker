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

#include "trayitemsettings.h"

static const QString GLOBALSKEY = "_GLOBAL_DEFAULTS";

int TrayItemSettings::nonZeroBalloonTimeout()
{
    int val = getNotifyTime();
    if (val > 0)
        return val;

    val = m_settings.value(QString("%1/BalloonTimeout").arg(m_dockedAppName), 0).toInt();
    if (val > 0)
        return val;

    val = m_settings.value(QString("%1/BalloonTimeout").arg(GLOBALSKEY), 0).toInt();
    if (val > 0)
        return val;

    return defaultNotifyTime();
}

void TrayItemSettings::loadSettings(const QString &dockedAppName, const TrayItemOptions &options)
{
    m_dockedAppName = dockedAppName;

    // Precedence:
    // 1) Command line overrides         (argSetting, if positive)
    // 2) User app-specific defaults     (QSettings: "<m_dockedAppName>/<key>")
    // 3) User global defaults           (QSettings: "_GLOBAL_DEFAULTS/<key>")
    // 4) KDocker defaults               (TrayItemOptions::default*)
    loadSettingsDefault();
    loadSettingsGlobal();
    loadSettingsApp();
    loadSettingsOptions(options);
}

void TrayItemSettings::loadSettingsDefault()
{
    setIconPath(defaultIconPath());
    setAttentionIconPath(defaultAttentionIconPath());
    setIconifyFocusLost(defaultIconifyFocusLost());
    setIconifyMinimized(defaultIconifyMinimized());
    setIconifyObscured(defaultIconifyObscured());
    setNotifyTime(defaultNotifyTime());
    setQuiet(defaultQuiet());
    setSkipPager(defaultSkipPager());
    setSticky(defaultSticky());
    setSkipTaskbar(defaultSkipTaskbar());
    setLockToDesktop(defaultLockToDesktop());
}

void TrayItemSettings::loadSettingsSection()
{
    // Group is set by caller
    QVariant val;

    val = m_settings.value("CustomIcon");
    if (val.isValid())
        setIconPath(val.toString());

    val = m_settings.value("AttentionIcon");
    if (val.isValid())
        setAttentionIconPath(val.toString());

    val = m_settings.value("Quiet");
    if (val.isValid())
        setQuiet(val.toBool());

    val = m_settings.value("BalloonTimeout");
    if (val.isValid())
        setNotifyTime(val.toInt());

    val = m_settings.value("Sticky");
    if (val.isValid())
        setSticky(val.toBool());

    val = m_settings.value("SkipPager");
    if (val.isValid())
        setSkipPager(val.toBool());

    val = m_settings.value("SkipTaskbar");
    if (val.isValid())
        setSkipTaskbar(val.toBool());

    val = m_settings.value("IconifyMinimized");
    if (val.isValid())
        setIconifyMinimized(val.toBool());

    val = m_settings.value("IconifyObscured");
    if (val.isValid())
        setIconifyObscured(val.toBool());

    val = m_settings.value("IconifyFocusLost");
    if (val.isValid())
        setIconifyFocusLost(val.toBool());

    val = m_settings.value("LockToDesktop");
    if (val.isValid())
        setLockToDesktop(val.toBool());
}

void TrayItemSettings::loadSettingsGlobal()
{
    m_settings.beginGroup(GLOBALSKEY);
    loadSettingsSection();
    m_settings.endGroup();
}

void TrayItemSettings::loadSettingsApp()
{
    m_settings.beginGroup(m_dockedAppName);
    loadSettingsSection();
    m_settings.endGroup();
}

void TrayItemSettings::loadSettingsOptions(const TrayItemOptions &options)
{
    TrayItemOptions::TriState tri;

    QString path = options.getIconPath();
    if (!path.isEmpty())
        setIconPath(path);

    path = options.getAttentionIconPath();
    if (!path.isEmpty())
        setAttentionIconPath(path);

    if (options.getNotifyTimeState())
        setNotifyTime(options.getNotifyTime());

    tri = options.getQuietState();
    if (tri != TrayItemOptions::TriState::Unset)
        setQuiet(tri);

    tri = options.getStickyState();
    if (tri != TrayItemOptions::TriState::Unset)
        setSticky(tri);

    tri = options.getSkipPagerState();
    if (tri != TrayItemOptions::TriState::Unset)
        setSkipPager(tri);

    tri = options.getSkipTaskbarState();
    if (tri != TrayItemOptions::TriState::Unset)
        setSkipTaskbar(tri);

    tri = options.getIconifyMinimizedState();
    if (tri != TrayItemOptions::TriState::Unset)
        setIconifyMinimized(tri);

    tri = options.getIconifyObscuredState();
    if (tri != TrayItemOptions::TriState::Unset)
        setIconifyObscured(tri);

    tri = options.getIconifyFocusLostState();
    if (tri != TrayItemOptions::TriState::Unset)
        setIconifyFocusLost(tri);

    tri = options.getLockToDesktopState();
    if (tri != TrayItemOptions::TriState::Unset)
        setLockToDesktop(tri);

    tri = options.getQuietState();
    if (tri != TrayItemOptions::TriState::Unset)
        setQuiet(tri);
}

void TrayItemSettings::saveSettingsSection()
{
    // Group is set by caller
    m_settings.setValue("Quiet", getQuiet());
    m_settings.setValue("BalloonTimeout", getNotifyTime());
    m_settings.setValue("Sticky", getSticky());
    m_settings.setValue("SkipPager", getSkipPager());
    m_settings.setValue("SkipTaskbar", getSkipTaskbar());
    m_settings.setValue("IconifyMinimized", getIconifyMinimized());
    m_settings.setValue("IconifyObscured", getIconifyObscured());
    m_settings.setValue("IconifyFocusLost", getIconifyFocusLost());
    m_settings.setValue("LockToDesktop", getLockToDesktop());
}

void TrayItemSettings::saveSettingsApp()
{
    m_settings.beginGroup(m_dockedAppName);
    if (!getIconPath().isEmpty())
        m_settings.setValue("CustomIcon", getIconPath());
    if (!getAttentionIconPath().isEmpty())
        m_settings.setValue("AttentionIcon", getAttentionIconPath());
    saveSettingsSection();
    m_settings.endGroup();
}

void TrayItemSettings::saveSettingsGlobal()
{
    m_settings.beginGroup(GLOBALSKEY);
    saveSettingsSection();
    m_settings.endGroup();
}
