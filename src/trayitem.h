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

#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QString>
#include <QSystemTrayIcon>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

class TrayItem : public QSystemTrayIcon {
    Q_OBJECT

public:
    TrayItem(Window window, QObject *parent = 0);
    ~TrayItem();

    Window dockedWindow();

    // Pass on all events through this interface
    bool x11EventFilter(XEvent * event);
    void setCustomIcon(QString path);

public slots:
    void restoreWindow();
    void iconifyWindow();
    void skipTaskbar();
    void close(); // close the docked window

    void setSkipTaskbar(bool value);
    void setIconifyObscure(bool value);
    void setIconifyFocusLost(bool value);
    void setBalloonTimeout(int value);

private slots:
    void toggleWindow(QSystemTrayIcon::ActivationReason reason);
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
    void updateTitle();
    void updateIcon();
    void createContextMenu();
    QIcon createIcon(Window window);

    bool m_withdrawn;
    bool m_customIcon;
    bool m_skipTaskbar;
    bool m_iconifyObscure;
    bool m_iconifyFocusLost;
    int m_balloonTimeout;

    XSizeHints m_sizeHint; // SizeHint of m_window
    Window m_window; // The window that is associated with the tray icon.
    QMenu *m_contextMenu;
};

#endif	/* _TRAYITEM_H */

