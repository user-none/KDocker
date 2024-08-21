/*
 *  Copyright (C) 2009 John Schember <john@nachtimwald.com>
 *  Copyright (C) 2004 Girish Ramakrishnan All Rights Reserved.
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

#ifndef _TRAYITEM_H
#define _TRAYITEM_H

#include "trayitemoptions.h"
#include "xlibutil.h"

#include <QAction>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QSettings>
#include <QString>
#include <QSystemTrayIcon>

#include <xcb/xproto.h>

class TrayItem : public QSystemTrayIcon
{
    Q_OBJECT

public:
    TrayItem(Window window, const TrayItemOptions &config);
    ~TrayItem();

    Window dockedWindow();

    // Pass on all events through this interface
    bool xcbEventFilter(xcb_generic_event_t *event, xcb_window_t dockedWindow);

    void showWindow();
    void restoreWindow();
    void iconifyWindow();

    void doSkipTaskbar();
    void doSkipPager();
    void doSticky();

    QString appName();

public slots:
    void closeWindow();
    void setCustomIcon(QString path);
    void setAttentionIcon(QString path);
    void selectCustomIcon(bool value);
    void selectAttentionIcon(bool value);
    void setSkipTaskbar(bool value);
    void setSkipPager(bool value);
    void setSticky(bool value);
    void setIconifyMinimized(bool value);
    void setIconifyObscured(bool value);
    void setIconifyFocusLost(bool value);
    void setLockToDesktop(bool value);
    void setBalloonTimeout(int value);
    void setBalloonTimeout(bool value);

private slots:
    void toggleWindow();
    void trayActivated(QSystemTrayIcon::ActivationReason reason = QSystemTrayIcon::Trigger);
    void doUndock();
    void saveSettingsGlobal();
    void saveSettingsApp();

signals:
    void selectAnother();
    void dead(TrayItem *);
    void undockAll();
    void undock(TrayItem *);
    void about();

protected:
    bool event(QEvent *e);

private:
    //   readSetting overloaded function
    bool readSetting(TrayItemOptions::TriState argSetting, QString key, bool kdockerDefault);
    int readSetting(int argSetting, QString key, int kdockerDefault);
    QString readSetting(const QString &argSetting, QString key, const QString &kdockerDefault);
    int nonZeroBalloonTimeout();
    TrayItemOptions readConfigGlobals();
    void saveSettings();

    void minimizeEvent();
    void destroyEvent();
    void propertyChangeEvent(Atom property);
    void obscureEvent();
    void focusLostEvent();

    void set_NET_WM_STATE(const char *type, bool set);

    void readDockedAppName();
    void updateTitle();
    void updateIcon();
    void updateToggleAction();

    void createContextMenu();
    QString selectIcon(QString title);

    bool isBadWindow();
    bool isOnCurrentDesktop();

    bool m_wantsAttention;
    bool m_iconified;
    bool m_is_restoring;
    bool m_customIcon;

    QIcon m_defaultIcon;
    QIcon m_attentionIcon;

    QSettings m_config;
    TrayItemOptions m_settings;

    // SizeHint of m_window
    XLibUtilSizeHints *m_sizeHint;
    // The window that is associated with the tray icon.
    Window m_window;
    long m_desktop;
    QString m_dockedAppName;

    QMenu *m_contextMenu;
    QMenu *m_optionsMenu;
    QAction *m_actionSetIcon;
    QAction *m_actionSetAttentionIcon;
    QAction *m_actionSkipTaskbar;
    QAction *m_actionSkipPager;
    QAction *m_actionSticky;
    QAction *m_actionIconifyMinimized;
    QAction *m_actionIconifyObscured;
    QAction *m_actionIconifyFocusLost;
    QAction *m_actionLockToDesktop;
    QAction *m_actionBalloonTitleChanges;
    QAction *m_actionToggle;
    QMenu *m_defaultsMenu;
    QAction *m_actionSaveSettingsApp;
    QAction *m_actionSaveSettingsGlobal;
};

#endif // _TRAYITEM_H
