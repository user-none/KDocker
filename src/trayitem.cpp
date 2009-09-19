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
#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>
#include <QPixmap>
#include <QStringList>
#include <QX11Info>

#include "constants.h"
#include "trayitem.h"
#include "util.h"

#include <Xatom.h>
#include <X11/xpm.h>

#include "mwmutil.h"

TrayItem::TrayItem(Window window, QObject *parent) : QSystemTrayIcon(parent) {
    m_iconified = false;
    m_customIcon = false;
    m_skipTaskbar = false;
    m_skipPager = false;
    m_sticky = false;
    m_iconifyMinimized = true;
    m_iconifyObscure = false;
    m_iconifyFocusLost = false;
    m_balloonTimeout = 4000;
    m_window = window;
    m_desktop = 999;
    m_dockedAppName = "";

    Display *display = QX11Info::display();
    // Allows events from m_window to be forwarded to the x11EventFilter.
    subscribe(display, m_window, StructureNotifyMask | PropertyChangeMask | VisibilityChangeMask | FocusChangeMask, true);
    // store the desktop on which the window is being shown
    getCardinalProperty(display, m_window, XInternAtom(display, "_NET_WM_DESKTOP", True), &m_desktop);

    readDockedAppName();
    updateTitle();
    updateIcon();

    createContextMenu();
    updateToggleAction();

    connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(toggleWindow(QSystemTrayIcon::ActivationReason)));
}

TrayItem::~TrayItem() {
    // Only the main menu needs to be deleted. The rest of the menus and actions
    // are children of this menu and Qt will delete all children.
    delete m_contextMenu;
}

Window TrayItem::dockedWindow() {
    return m_window;
}

bool TrayItem::x11EventFilter(XEvent *ev) {
    if (isBadWindow()) {
        return false;
    }

    XAnyEvent *event = (XAnyEvent *) ev;
    if (event->window == m_window) {
        if (event->type == DestroyNotify) {
            destroyEvent();
        } else if (event->type == PropertyNotify) {
            propertyChangeEvent(((XPropertyEvent *) event)->atom);
        } else if (event->type == VisibilityNotify) {
            if (((XVisibilityEvent *) event)->state == VisibilityFullyObscured) {
                obscureEvent();
            }
        } else if (event->type == FocusOut) {
            //focusLostEvent();
        } else if (event->type == MapNotify) {
            m_iconified = false;
            updateToggleAction();
        } else if (event->type == UnmapNotify) {
            m_iconified = true;
            updateToggleAction();
        }
        return true; // Dont process this again
    }
    return false;
}

void TrayItem::restoreWindow() {
    if (isBadWindow()) {
        return;
    }

    Display *display = QX11Info::display();
    Window root = QX11Info::appRootWindow();

    if (m_iconified) {
        m_iconified = false;
        /*
         * A simple XMapWindow would not do. Some applications like xmms wont
         * redisplay its other windows (like the playlist, equalizer) since the
         * Withdrawn->Normal state change code does not map them. So we make the
         * window go through Withdrawn->Map->Iconify->Normal state.
         */
        XMapWindow(display, m_window);
        XIconifyWindow(display, m_window, DefaultScreen(display));
        XSync(display, False);
        long l1[1] = {NormalState};
        sendMessage(display, root, m_window, "WM_CHANGE_STATE", 32, SubstructureNotifyMask | SubstructureRedirectMask, l1, sizeof (l1));

        m_sizeHint.flags = USPosition;
        XSetWMNormalHints(display, m_window, &m_sizeHint);
        // make it the active window
        long l2[5] = {None, CurrentTime, None, 0, 0};
        sendMessage(display, root, m_window, "_NET_ACTIVE_WINDOW", 32, SubstructureNotifyMask | SubstructureRedirectMask, l2, sizeof (l2));

        if (m_desktop == -1) {
            /*
             * We track _NET_WM_DESKTOP changes in the x11EventFilter. Its used here.
             * _NET_WM_DESKTOP is set by the WM to the active desktop for newly
             * mapped windows (like this one) at some point in time. We will override
             *  that value to -1 (all desktops) on showOnAllDesktops().
             */
            showOnAllDesktops();
        }

        updateToggleAction();
    } else {
        XRaiseWindow(display, m_window);
    }
}

void TrayItem::iconifyWindow() {
    if (isBadWindow()) {
        return;
    }

    m_iconified = true;

    Display *display = QX11Info::display();
    int screen = DefaultScreen(display);
    long dummy;

    XGetWMNormalHints(display, m_window, &m_sizeHint, &dummy);

    /*
     * A simple call to XWithdrawWindow wont do. Here is what we do:
     * 1. Iconify. This will make the application hide all its other windows. For
     *    example, xmms would take off the playlist and equalizer window.
     * 2. Withdrawn the window so we will be removed from the taskbar.
     */
    XIconifyWindow(display, m_window, screen); // good for effects too
    XSync(display, False);
    XWithdrawWindow(display, m_window, screen);
    updateToggleAction();
}

void TrayItem::skip_NET_WM_STATE(const char *type, bool set) {
    // set, true = add the state to the window. False, remove the state from
    // the window.

    if (isBadWindow() || m_iconified) {
        return;
    }
    Display *display = QX11Info::display();
    Atom atom = XInternAtom(display, type, False);

    long l[5] = {set ? 1 : 0, atom, 0, 0, 0};
    sendMessage(display, QX11Info::appRootWindow(), m_window, "_NET_WM_STATE", 32, SubstructureNotifyMask, l, sizeof (l));
}

void TrayItem::skipTaskbar() {
    skip_NET_WM_STATE("_NET_WM_STATE_SKIP_TASKBAR", m_skipTaskbar);
}

void TrayItem::skipPager() {
    skip_NET_WM_STATE("_NET_WM_STATE_SKIP_PAGER", m_skipPager);
}

void TrayItem::sticky() {
    skip_NET_WM_STATE("_NET_WM_STATE_STICKY", m_sticky);
}

void TrayItem::removeWindowBorder() {
    if (isBadWindow()) {
        return;
    }

    Display *display = QX11Info::display();
    Atom hints_atom = XInternAtom(display, _XA_MOTIF_WM_HINTS, false);

    unsigned char *data;
    Atom type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after;

    XGetWindowProperty(display, m_window, hints_atom, 0, sizeof (MotifWmHints) / sizeof (long), false, AnyPropertyType, &type, &format, &nitems, &bytes_after, &data);

    if (type == None) {
        MotifWmHints hints;
        memset(&hints, 0, sizeof (hints));
        hints.flags = MWM_HINTS_DECORATIONS;
        hints.decorations = 0;
        XChangeProperty(display, m_window, hints_atom, hints_atom, 32, PropModeReplace, (unsigned char *) & hints, sizeof (MotifWmHints) / sizeof (long));
    } else {
        MotifWmHints *hints;
        hints = (MotifWmHints *) data;

        if (!(hints->flags & MWM_HINTS_DECORATIONS)) {
            hints->flags |= MWM_HINTS_DECORATIONS;
        }
        hints->decorations = 0;
        XChangeProperty(display, m_window, hints_atom, hints_atom, 32, PropModeReplace, (unsigned char *) hints, sizeof (MotifWmHints) / sizeof (long));
    }
    XFree(data);
}

void TrayItem::setCustomIcon(QString path) {
    m_customIcon = true;
    QPixmap customIcon;
    if (!customIcon.load(path)) {
        customIcon.load(":/images/question.png");
    }

    setIcon(QIcon(customIcon));
}

void TrayItem::close() {
    if (isBadWindow()) {
        return;
    }
    else {
        Display *display = QX11Info::display();
        long l[5] = {0, 0, 0, 0, 0};
        restoreWindow();
        sendMessage(display, QX11Info::appRootWindow(), m_window, "_NET_CLOSE_WINDOW", 32, SubstructureNotifyMask | SubstructureRedirectMask, l, sizeof (l));
        destroyEvent();
    }
}

void TrayItem::selectCustomIcon(bool value) {
    Q_UNUSED(value);

    QStringList types;
    QString supportedTypes;
    QString path;

    Q_FOREACH(QByteArray type, QImageReader::supportedImageFormats()) {
        types << QString(type);
    }
    if (types.isEmpty()) {
        supportedTypes = "All Files (*.*)";
    } else {
        supportedTypes = QString("Images (*.%1);;All Files (*.*)").arg(types.join(" *."));
    }

    path = QFileDialog::getOpenFileName(0, tr("Select Icon"), QDir::homePath(), supportedTypes);

    if (!path.isEmpty()) {
        setCustomIcon(path);
    }
}

void TrayItem::setSkipTaskbar(bool value) {
    m_skipTaskbar = value;
    m_actionSkipTaskbar->setChecked(value);
    skipTaskbar();
}

void TrayItem::setSkipPager(bool value) {
    m_skipPager = value;
    m_actionSkipPager->setChecked(value);
    skipPager();
}

void TrayItem::setSticky(bool value) {
    m_sticky = value;
    m_actionSticky->setChecked(value);
    sticky();
}

void TrayItem::setIconifyMinimized(bool value) {
    m_iconifyMinimized = value;
    m_actionIconifyMinimized->setChecked(value);
}

void TrayItem::setIconifyObscure(bool value) {
    m_iconifyObscure = value;
    m_actionIconifyObscure->setChecked(value);
}

void TrayItem::setIconifyFocusLost(bool value) {
    m_iconifyFocusLost = value;
    m_actionIconifyFocusLost->setChecked(value);
    focusLostEvent();
}

void TrayItem::setBalloonTimeout(int value) {
    if (value < 0) {
        value = 0;
    }
    m_balloonTimeout = value;
    m_actionBalloonTitleChanges->setChecked(value ? true : false);
}

void TrayItem::setBalloonTimeout(bool value) {
    if (value) {
        setBalloonTimeout(0);
    } else {
        setBalloonTimeout(4000);
    }
}

void TrayItem::toggleWindow(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        if (m_iconified) {
            restoreWindow();
        } else {
            iconifyWindow();
        }
    }
}

/*
 * Sends a message to the WM to show this window on all the desktops
 */
void TrayItem::showOnAllDesktops() {
    if (isBadWindow()) {
        return;
    }

    Display *display = QX11Info::display();
    long l[1] = {-1}; // -1 = all, 0 = Desktop1, 1 = Desktop2 ...
    sendMessage(display, QX11Info::appRootWindow(), m_window, "_NET_WM_DESKTOP", 32, SubstructureNotifyMask | SubstructureRedirectMask, l, sizeof (l));
}

void TrayItem::doAbout() {
    QMessageBox aboutBox;
    aboutBox.setIconPixmap(QPixmap(":/images/kdocker.png"));
    aboutBox.setWindowTitle(tr("About %1 - %2").arg(qApp->applicationName()).arg(qApp->applicationVersion()));
    aboutBox.setText(ABOUT);
    aboutBox.setInformativeText(tr("See %1 for more information.").arg("<a href=\"https://launchpad.net/kdocker\">https://launchpad.net/kdocker</a>"));
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

void TrayItem::doSelectAnother() {
    emit(selectAnother());
}

void TrayItem::doUndock() {
    emit(undock(this));
}

void TrayItem::doUndockAll() {
    emit(undockAll());
}

void TrayItem::minimizeEvent() {
    if (m_iconifyMinimized) {
        iconifyWindow();
    }
}

void TrayItem::destroyEvent() {
    m_window = 0;
    emit(dead(this));
}

void TrayItem::propertyChangeEvent(Atom property) {
    if (isBadWindow()) {
        return;
    }

    Display *display = QX11Info::display();
    static Atom WM_NAME = XInternAtom(display, "WM_NAME", True);
    static Atom WM_ICON = XInternAtom(display, "WM_ICON", True);
    static Atom WM_STATE = XInternAtom(display, "WM_STATE", True);
    static Atom _NET_WM_DESKTOP = XInternAtom(display, "_NET_WM_DESKTOP", True);

    if (property == WM_NAME) {
        updateTitle();
    } else if (property == WM_ICON) {
        updateIcon();
    } else if (property == _NET_WM_DESKTOP) {
        getCardinalProperty(display, m_window, _NET_WM_DESKTOP, &m_desktop);
    } else if (property == WM_STATE) {
        Atom type = None;
        int format;
        unsigned long nitems, after;
        unsigned char *data = 0;
        int r = XGetWindowProperty(display, m_window, WM_STATE, 0, 1, False, AnyPropertyType, &type, &format, &nitems, &after, &data);
        if ((r == Success) && data && (*(long *) data == IconicState)) {
            minimizeEvent();
            XFree(data);
        }
        skipTaskbar();
        skipPager();
    }
}

void TrayItem::obscureEvent() {
    if (m_iconifyObscure) {
        iconifyWindow();
    }
}

void TrayItem::focusLostEvent() {
    if (m_iconifyFocusLost && m_window != activeWindow(QX11Info::display())) {
        iconifyWindow();
    }
}

void TrayItem::readDockedAppName() {
    if (isBadWindow()) {
        return;
    }

    Display *display = QX11Info::display();
    XClassHint ch;
    if (XGetClassHint(display, m_window, &ch)) {
        if (ch.res_class) {
            m_dockedAppName = QString(ch.res_class);
        } else if (ch.res_name) {
            m_dockedAppName = QString(ch.res_name);
        }

        if (ch.res_class) {
            XFree(ch.res_class);
        }
        if (ch.res_name) {
            XFree(ch.res_name);
        }
    }
}

/*
 * Update the title in the tooltip.
 */
void TrayItem::updateTitle() {
    if (isBadWindow()) {
        return;
    }

    Display *display = QX11Info::display();
    char *windowName = 0;
    QString title;

    XFetchName(display, m_window, &windowName);
    title = windowName;
    if (windowName) {
        XFree(windowName);
    }

    setToolTip(QString("%1 [%2]").arg(title).arg(m_dockedAppName));
    if (m_balloonTimeout > 0) {
        showMessage(m_dockedAppName, title, QSystemTrayIcon::Information, m_balloonTimeout);
    }
}

void TrayItem::updateIcon() {
    if (isBadWindow() || m_customIcon) {
        return;
    }

    setIcon(createIcon(m_window));
}

void TrayItem::updateToggleAction() {
    QString text;
    QIcon icon;
    if (m_iconified) {
        text = tr("Show %1").arg(m_dockedAppName);
        icon = QIcon(":/images/restore.png");
    } else {
        text = tr("Hide %1").arg(m_dockedAppName);
        icon = QIcon(":/images/iconify.png");
    }
    m_actionToggle->setIcon(icon);
    m_actionToggle->setText(text);
}

void TrayItem::createContextMenu() {
    m_contextMenu = new QMenu();

    m_contextMenu->addAction(QIcon(":/images/about.png"), tr("About %1").arg(qApp->applicationName()), this, SLOT(doAbout()));
    m_contextMenu->addSeparator();

    // Options menu
    m_optionsMenu = new QMenu(tr("Options"), m_contextMenu);
    m_actionSetIcon = new QAction(tr("Set icon"), m_optionsMenu);
    connect(m_actionSetIcon, SIGNAL(triggered(bool)), this, SLOT(selectCustomIcon(bool)));
    m_optionsMenu->addAction(m_actionSetIcon);
    m_actionSkipTaskbar = new QAction(tr("Skip taskbar"), m_optionsMenu);
    m_actionSkipTaskbar->setCheckable(true);
    m_actionSkipTaskbar->setChecked(m_skipTaskbar);
    connect(m_actionSkipTaskbar, SIGNAL(triggered(bool)), this, SLOT(setSkipTaskbar(bool)));
    m_optionsMenu->addAction(m_actionSkipTaskbar);
    m_actionSkipPager = new QAction(tr("Skip pager"), m_optionsMenu);
    m_actionSkipPager->setCheckable(true);
    m_actionSkipPager->setChecked(m_skipPager);
    connect(m_actionSkipPager, SIGNAL(triggered(bool)), this, SLOT(setSkipPager(bool)));
    m_optionsMenu->addAction(m_actionSkipPager);
    m_actionSticky = new QAction(tr("Sticky"), m_optionsMenu);
    m_actionSticky->setCheckable(true);
    m_actionSticky->setChecked(m_sticky);
    connect(m_actionSticky, SIGNAL(triggered(bool)), this, SLOT(setSticky(bool)));
    m_optionsMenu->addAction(m_actionSticky);
    m_actionIconifyMinimized = new QAction(tr("Iconify when minimized"), m_optionsMenu);
    m_actionIconifyMinimized->setCheckable(true);
    m_actionIconifyMinimized->setChecked(m_iconifyMinimized);
    connect(m_actionIconifyMinimized, SIGNAL(triggered(bool)), this, SLOT(setIconifyMinimized(bool)));
    m_optionsMenu->addAction(m_actionIconifyMinimized);
    m_actionIconifyObscure = new QAction(tr("Iconify when obscured"), m_optionsMenu);
    m_actionIconifyObscure->setCheckable(true);
    m_actionIconifyObscure->setChecked(m_iconifyObscure);
    connect(m_actionIconifyObscure, SIGNAL(triggered(bool)), this, SLOT(setIconifyObscure(bool)));
    m_optionsMenu->addAction(m_actionIconifyObscure);
    m_actionIconifyFocusLost = new QAction(tr("Iconify when focus lost"), m_optionsMenu);
    m_actionIconifyFocusLost->setCheckable(true);
    m_actionIconifyFocusLost->setChecked(m_iconifyFocusLost);
    //connect(m_actionIconifyFocusLost, SIGNAL(toggled(bool)), this, SLOT(setIconifyFocusLost(bool)));
    //m_optionsMenu->addAction(m_actionIconifyFocusLost);
    m_actionBalloonTitleChanges = new QAction(tr("Balloon title changes"), m_optionsMenu);
    m_actionBalloonTitleChanges->setCheckable(true);
    m_actionBalloonTitleChanges->setChecked(m_balloonTimeout ? true : false);
    connect(m_actionBalloonTitleChanges, SIGNAL(triggered(bool)), this, SLOT(setBalloonTimeout(bool)));
    m_optionsMenu->addAction(m_actionBalloonTitleChanges);
    m_contextMenu->addMenu(m_optionsMenu);

    m_contextMenu->addAction(QIcon(":/images/another.png"), tr("Dock Another"), this, SLOT(doSelectAnother()));
    m_contextMenu->addAction(tr("Undock All"), this, SLOT(doUndockAll()));
    m_contextMenu->addSeparator();
    m_actionToggle = new QAction(tr("Toggle"), m_contextMenu);
    connect(m_actionToggle, SIGNAL(triggered()), this, SLOT(toggleWindow()));
    m_contextMenu->addAction(m_actionToggle);
    m_contextMenu->addAction(tr("Undock"), this, SLOT(doUndock()));
    m_contextMenu->addAction(QIcon(":/images/close.png"), tr("Close"), this, SLOT(close()));
    setContextMenu(m_contextMenu);
}

QIcon TrayItem::createIcon(Window window) {
    char **window_icon = 0;

    if (!window) {
        return QIcon();
    }

    QPixmap appIcon;
    Display *display = QX11Info::display();
    XWMHints *wm_hints = XGetWMHints(display, window);
    if (wm_hints != 0) {
        if (!(wm_hints->flags & IconMaskHint))
            wm_hints->icon_mask = None;
        /*
         * We act paranoid here. Progams like KSnake has a bug where
         * IconPixmapHint is set but no pixmap (Actually this happens with
         * quite a few KDE 3.x programs) X-(
         */
        if ((wm_hints->flags & IconPixmapHint) && (wm_hints->icon_pixmap)) {
            XpmCreateDataFromPixmap(display, &window_icon, wm_hints->icon_pixmap, wm_hints->icon_mask, 0);
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

bool TrayItem::isBadWindow() {
    Display *display = QX11Info::display();

    if (!isValidWindowId(display, m_window)) {
        destroyEvent();
        return true;
    }
    return false;
}
