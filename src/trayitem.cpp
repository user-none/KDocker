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

#include <QApplication>
#include <QPixmap>
#include <QX11Info>
#include <QDebug>

#include "trayitem.h"
#include "util.h"

#include <X11/xpm.h>

const long SYSTEM_TRAY_REQUEST_DOCK = 0;

TrayItem::TrayItem(Window window, QObject *parent) : QSystemTrayIcon(parent) {
    m_withdrawn = false;
    m_window = window;

    // Allows events from m_window to be forwarded to the x11EventFilter.
    subscribe(QX11Info::display(), m_window, StructureNotifyMask | PropertyChangeMask, true);

    createContextMenu();

    updateTitle();
    updateIcon();

    connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(toggleWindow(QSystemTrayIcon::ActivationReason)));
}

TrayItem::~TrayItem() {
    delete m_contextMenu;
}

Window TrayItem::dockedWindow() {
    return m_window;
}

bool TrayItem::x11EventFilter(XEvent *ev) {
    if (!m_window) {
        return false;
    }

    XAnyEvent *event = (XAnyEvent *) ev;
    if (event->window == m_window) {
        if (event->type == DestroyNotify) {
            destroyEvent();
        } else if (event->type == PropertyNotify) {
            propertyChangeEvent(((XPropertyEvent *) event)->atom);
        } else if (event->type == MapNotify) {
            m_withdrawn = false;
        } else if (event->type == UnmapNotify) {
            m_withdrawn = true;
        }
        return true; // Dont process this again
    }
    return false;
}

void TrayItem::restoreWindow() {
    m_withdrawn = false;
    if (!m_window) {
        return;
    }

    Display *display = QX11Info::display();

    /*
     * A simple XMapWindow would not do. Some applications like xmms wont
     * redisplay its other windows (like the playlist, equalizer) since the
     * Withdrawn->Normal state change code does not map them. So we make the
     * window go through Withdrawn->Iconify->Normal state.
     */
    XWMHints *wm_hint = XGetWMHints(display, m_window);
    if (wm_hint) {
        wm_hint->initial_state = IconicState;
        XSetWMHints(display, m_window, wm_hint);
        XFree(wm_hint);
    }

    XMapWindow(display, m_window);
    m_sizeHint.flags = USPosition; // Obsolete ?
    XSetWMNormalHints(display, m_window, &m_sizeHint);
    // make it the active window
    long l[5] = {None, CurrentTime, None, 0, 0};
    sendMessage(display, QX11Info::appRootWindow(), m_window, "_NET_ACTIVE_WINDOW", 32, SubstructureNotifyMask | SubstructureRedirectMask, l, sizeof (l));
}

void TrayItem::iconifyWindow() {
    if (!m_window) {
        return;
    }

    m_withdrawn = true;

    Display *display = QX11Info::display();
    int screen = DefaultScreen(display);
    long dummy;

    XGetWMNormalHints(display, m_window, &m_sizeHint, &dummy);

    /*
     * A simple call to XWithdrawWindow wont do. Here is what we do:
     * 1. Iconify. This will make the application hide all its other windows. For
     *    example, xmms would take off the playlist and equalizer window.
     * 2. Next tell the WM, that we would like to go to withdrawn state. Withdrawn
     *    state will remove us from the taskbar.
     *    Reference: ICCCM 4.1.4 Changing Window State
     */
    XIconifyWindow(display, m_window, screen); // good for effects too
    XUnmapWindow(display, m_window);
    XUnmapEvent ev;
    memset(&ev, 0, sizeof (ev));
    ev.type = UnmapNotify;
    ev.display = display;
    ev.event = QX11Info::appRootWindow();
    ev.window = m_window;
    ev.from_configure = false;
    XSendEvent(display, QX11Info::appRootWindow(), False, SubstructureRedirectMask | SubstructureNotifyMask, (XEvent *) & ev);
    XSync(display, False);
}

void TrayItem::close() {
    if (m_window) {
        Display *display = QX11Info::display();
        long l[5] = {0, 0, 0, 0, 0};
        restoreWindow();
        sendMessage(display, QX11Info::appRootWindow(), m_window, "_NET_CLOSE_WINDOW", 32, SubstructureNotifyMask | SubstructureRedirectMask, l, sizeof (l));
    }
    destroyEvent();
}

void TrayItem::toggleWindow(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        if (m_withdrawn) {
            restoreWindow();
        } else {
            iconifyWindow();
        }
    }
}

void TrayItem::another() {
    emit(selectAnother());
}

void TrayItem::minimizeEvent() {
    iconifyWindow();
}

void TrayItem::destroyEvent() {
    m_window = 0;
    emit(itemClose(this));
}

void TrayItem::propertyChangeEvent(Atom property) {
    if (!m_window) {
        return;
    }

    Display *display = QX11Info::display();
    static Atom WM_NAME = XInternAtom(display, "WM_NAME", True);
    static Atom WM_ICON = XInternAtom(display, "WM_ICON", True);
    static Atom WM_STATE = XInternAtom(display, "WM_STATE", True);

    if (property == WM_NAME) {
        updateTitle();
    } else if (property == WM_ICON) {
        updateIcon();
    } else if (property == WM_STATE) {
        Atom type = None;
        int format;
        unsigned long nitems, after;
        unsigned char *data = NULL;
        int r = XGetWindowProperty(display, m_window, WM_STATE, 0, 1, False, AnyPropertyType, &type, &format, &nitems, &after, &data);
        if ((r == Success) && data && (*(long *) data == IconicState)) {
            minimizeEvent();
            XFree(data);
        }
    }
}

/*
 * Update the title in the tooltip.
 */
void TrayItem::updateTitle() {
    if (!m_window) {
        return;
    }

    Display *display = QX11Info::display();
    char *windowName = NULL;
    QString title;
    QString className;

    XFetchName(display, m_window, &windowName);
    title = windowName;
    if (windowName) {
        XFree(windowName);
    }

    XClassHint ch;
    if (XGetClassHint(display, m_window, &ch)) {
        if (ch.res_class) {
            className = QString(ch.res_class);
        } else if (ch.res_name) {
            className = QString(ch.res_name);
        }

        if (ch.res_class) {
            XFree(ch.res_class);
        }
        if (ch.res_name) {
            XFree(ch.res_name);
        }
    }

    setToolTip(QString("%1 [%2]").arg(title).arg(className));
    showMessage(className, title);
}

void TrayItem::updateIcon() {
    if (!m_window) {
        return;
    }

    setIcon(createIcon(m_window));
}

void TrayItem::createContextMenu() {
    m_contextMenu = new QMenu();

    m_contextMenu->addAction(tr("Dock Another"), this, SLOT(another()));
    m_contextMenu->addAction(tr("Close"), this, SLOT(close()));
    setContextMenu(m_contextMenu);
}

QIcon TrayItem::createIcon(Window window) {
    char **window_icon = NULL;

    if (!window) {
        return QIcon();
    }

    QPixmap appIcon;
    Display *display = QX11Info::display();
    XWMHints *wm_hints = XGetWMHints(display, window);
    if (wm_hints != NULL) {
        if (!(wm_hints->flags & IconMaskHint))
            wm_hints->icon_mask = None;
        /*
         * We act paranoid here. Progams like KSnake has a bug where
         * IconPixmapHint is set but no pixmap (Actually this happens with
         * quite a few KDE programs) X-(
         */
        if ((wm_hints->flags & IconPixmapHint) && (wm_hints->icon_pixmap)) {
            XpmCreateDataFromPixmap(display, &window_icon, wm_hints->icon_pixmap, wm_hints->icon_mask, NULL);
        }
        XFree(wm_hints);
    }

    if (!window_icon) {
        appIcon.load(":/images/question.png");
    } else {
        appIcon = QPixmap((const char **) window_icon);
    }
    if (window_icon) {
        XpmFree(window_icon);
    }
    return QIcon(appIcon);
}
