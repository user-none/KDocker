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

#ifndef _TRAYITEMSETTINGS
#define _TRAYITEMSETTINGS

#include "trayitemoptions.h"

#include <QObject>
#include <QSettings>
#include <QString>

// Settings are saved to "/home/<user>/.config/com.kdocker/KDocker.conf"
class TrayItemSettings : public QObject, public TrayItemOptions
{
    Q_OBJECT

public:
    void loadSettings(const QString &dockedAppName, const TrayItemOptions &options);
    int nonZeroBalloonTimeout();

public slots:
    void saveSettingsApp();
    void saveSettingsGlobal();

private:
    void loadSettingsDefault();
    void loadSettingsApp();
    void loadSettingsGlobal();
    void loadSettingsOptions(const TrayItemOptions &options);
    void loadSettingsSection();
    void saveSettingsSection();

    QString m_dockedAppName;
    QSettings m_settings;
};

#endif //_TRAYITEMSETTINGS
