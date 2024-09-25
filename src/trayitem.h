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
#include "trayitemsettings.h"
#include "xlibtypes.h"

#include <QAction>
#include <QEvent>
#include <QIcon>
#include <QMenu>
#include <QSettings>
#include <QString>
#include <QSystemTrayIcon>

class TrayItem : public QSystemTrayIcon
{
    Q_OBJECT

public:
    TrayItem(windowid_t window, const TrayItemOptions &config);
    ~TrayItem();

    windowid_t dockedWindow();

    // Pass on all events through this interface
    bool xcbEventFilter(void *message);

    void show();
    void restoreWindow();
    void iconifyWindow();

    void doSkipTaskbar();

    QString appName();

public slots:
    void closeWindow();
    void setSkipTaskbar(bool value);

private slots:
    void setCustomIcon(QString path);
    void setAttentionIcon(QString path);
    void selectCustomIcon(bool value);
    void clearCustomIcon(bool value);
    void selectAttentionIcon(bool value);
    void clearAttentionIcon(bool value);
    void setSkipPager(bool value);
    void setSticky(bool value);
    void setIconifyMinimized(bool value);
    void setIconifyObscured(bool value);
    void setIconifyFocusLost(bool value);
    void setLockToDesktop(bool value);
    void setBalloonTimeout(bool value);

    void toggleWindow();
    void trayActivated(QSystemTrayIcon::ActivationReason reason = QSystemTrayIcon::Trigger);

    void doUndock();
    void doSkipPager();
    void doSticky();

signals:
    void selectAnother();
    void dead(TrayItem *);
    void undockAll();
    void undock(TrayItem *);
    void about();

protected:
    bool event(QEvent *e);

private:
    void minimizeEvent();
    void destroyEvent();
    void obscureEvent();
    void focusLostEvent();

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
    bool m_customIcon;

    QIcon m_defaultIcon;
    QIcon m_attentionIcon;

    TrayItemSettings m_settings;

    // SizeHint of m_window
    XLibUtilSizeHints *m_sizeHint;
    // The window that is associated with the tray icon.
    windowid_t m_window;
    long m_desktop;
    QString m_dockedAppName;

    QMenu m_contextMenu;
    // Owned and managed by m_contextMenu
    QAction *m_actionToggle;
};

#endif // _TRAYITEM_H
