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
#define	_TRAYITEM_H

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QString>
#include <QSystemTrayIcon>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

struct TrayItemSettings {
    QString customIcon;
    int balloonTimeout;
    bool borderless;
    bool iconify;
    bool skipTaskbar;
    bool skipPager;
    bool sticky;
    bool iconifyObscure;
    bool iconifyFocusLost;
};

class TrayItem : public QSystemTrayIcon {
    Q_OBJECT

public:
    TrayItem(Window window, QObject *parent = 0);
    ~TrayItem();

    Window dockedWindow();

    // Pass on all events through this interface
    bool x11EventFilter(XEvent * event);

public slots:
    void restoreWindow();
    void iconifyWindow();
    void skip_NET_WM_STATE(const char *type, bool set);
    void skipTaskbar();
    void skipPager();
    void sticky();
    void removeWindowBorder();
    void setCustomIcon(QString path);
    void close(); // close the docked window

    void setSkipTaskbar(bool value);
    void setSkipPager(bool value);
    void setSticky(bool value);
    void setIconifyMinimized(bool value);
    void setIconifyObscure(bool value);
    void setIconifyFocusLost(bool value);
    void setBalloonTimeout(int value);
    void setBalloonTimeout(bool value);

private slots:
    void toggleWindow(QSystemTrayIcon::ActivationReason reason = QSystemTrayIcon::Trigger);
    void showOnAllDesktops();
    void doAbout();
    void doSelectAnother();
    void doUndock();
    void doUndockAll();

signals:
    void selectAnother();
    void undockAll();
    void undock(TrayItem*);

private:
    void minimizeEvent();
    void destroyEvent();
    void propertyChangeEvent(Atom property);
    void obscureEvent();
    void focusLostEvent();
    void readDockedAppName();
    void updateTitle();
    void updateIcon();
    void updateToggleAction();
    void createContextMenu();
    QIcon createIcon(Window window);

    bool m_iconified;

    bool m_customIcon;
    bool m_skipTaskbar;
    bool m_skipPager;
    bool m_sticky;
    bool m_iconifyMinimized;
    bool m_iconifyObscure;
    bool m_iconifyFocusLost;
    int m_balloonTimeout;

    XSizeHints m_sizeHint; // SizeHint of m_window
    Window m_window; // The window that is associated with the tray icon.
    long m_desktop;
    QString m_dockedAppName;

    QMenu *m_contextMenu;
    QMenu *m_optionsMenu;
    QAction *m_actionSkipTaskbar;
    QAction *m_actionSkipPager;
    QAction *m_actionSticky;
    QAction *m_actionIconifyMinimized;
    QAction *m_actionIconifyObscure;
    QAction *m_actionIconifyFocusLost;
    QAction *m_actionBalloonTitleChanges;
    QAction *m_actionToggle;

};

#endif	/* _TRAYITEM_H */

