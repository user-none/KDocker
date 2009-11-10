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
#include <QRect>
#include <QStringList>
#include <QTime>
#include <QX11Info>

#include "trayitem.h"
#include "xlibutil.h"

#include <Xatom.h>
#include <X11/xpm.h>

#include "mwmutil.h"

TrayItem::TrayItem(Window window) {
    m_customIcon = false;
    m_skipTaskbar = false;
    m_skipPager = false;
    m_sticky = false;
    m_iconifyMinimized = true;
    m_iconifyObscure = false;
    m_iconifyFocusLost = false;
    m_iconifyOnClose = true;
    m_balloonTimeout = 4000;
    m_dockedAppName = "";
    m_window = window;

    Display *display = QX11Info::display();
    long dummy;
    Window root;
    int x;
    int y;
    unsigned int width;
    unsigned int height;
    unsigned int border;
    unsigned int depth;
    Atom wm_hints_atom = XInternAtom(display, _XA_MOTIF_WM_HINTS, false);
    unsigned char *wm_data;
    Atom wm_type;
    int wm_format;
    unsigned long wm_nitems;
    unsigned long wm_bytes_after;

    // Get the window size info.
    XGetWMNormalHints(display, m_window, &m_sizeHint, &dummy);
    XGetGeometry(display, m_window, &root, &x, &y, &width, &height, &border, &depth);
    // Get the window decoration info.
    XGetWindowProperty(display, m_window, wm_hints_atom, 0, sizeof (MotifWmHints) / sizeof (long), false, AnyPropertyType, &wm_type, &wm_format, &wm_nitems, &wm_bytes_after, &wm_data);

    // There is a conflict between QX11EmbedContainer and QSystemTrayIcon with
    // regard to registering the X11EventFilter. This prevents a segfault.
    QX11EmbedContainer();
    // Create the container window and place the selected window into it.
    m_container = new QX11EmbedContainer();
    m_container->embedClient(m_window);
    m_container->show();

    // Allows events from m_container and m_window to be forwarded to the
    // x11EventFilter. Only event types that are subscribed to will be sent on.
    XLibUtil::subscribe(display, m_container->winId(), StructureNotifyMask | PropertyChangeMask | VisibilityChangeMask | FocusChangeMask, true);
    XLibUtil::subscribe(display, m_window, PropertyChangeMask, true);

    // store the desktop on which the window is being shown
    XLibUtil::getCardinalProperty(display, m_container->winId(), XInternAtom(display, "_NET_WM_DESKTOP", true), &m_desktop);

    // Do not close the window when the X button is pressed. This only works
    // Because we have embedded into our own window.
    Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", false);
    XSetWMProtocols(display, m_container->winId(), &wm_delete, 1);

    // Set the containers size hints to that of window.
    m_container->setMinimumSize(m_sizeHint.min_width, m_sizeHint.min_height);
    // Set the containers geometry to that of window.
    m_container->setGeometry(x, y, width, height);

    // Set the containers decorations to those of window.
    if (wm_type == None) {
        MotifWmHints hints;
        memset(&hints, 0, sizeof (hints));
        hints.flags = MWM_HINTS_DECORATIONS;
        hints.decorations = MWM_DECOR_ALL;
        XChangeProperty(display, m_container->winId(), wm_hints_atom, wm_hints_atom, 32, PropModeReplace, reinterpret_cast<unsigned char *> (& hints), sizeof (MotifWmHints) / sizeof (long));
    } else {
        MotifWmHints *hints;
        hints = reinterpret_cast<MotifWmHints *> (wm_data);
        if (!(hints->flags & MWM_HINTS_DECORATIONS)) {
            hints->flags |= MWM_HINTS_DECORATIONS;
            hints->decorations = 0;
        }
        XChangeProperty(display, m_container->winId(), wm_hints_atom, wm_hints_atom, 32, PropModeReplace, reinterpret_cast<unsigned char *> (hints), sizeof (MotifWmHints) / sizeof (long));
    }
    XFree(wm_data);

    readDockedAppName();
    updateTitle();
    updateIcon();

    createContextMenu();
    updateToggleAction();

    connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
}

TrayItem::~TrayItem() {
    // Only the main menu needs to be deleted. The rest of the menus and actions
    // are children of this menu and Qt will delete all children.
    delete m_contextMenu;

    // Move and resize the embedded window if it is still open to the size and
    // position of the container.
    QRect geo = m_container->geometry();
    m_container->discardClient();
    delete m_container;
    if (m_window) {
        XMoveResizeWindow(QX11Info::display(), m_window, geo.x(), geo.y(), static_cast<unsigned int> (geo.width()), static_cast<unsigned int> (geo.height()));
    }
}

Window TrayItem::containerWindow() {
    return m_container->winId();
}

Window TrayItem::embedWindow() {
    return m_window;
}

bool TrayItem::x11EventFilter(XEvent *ev) {
    XAnyEvent *event = reinterpret_cast<XAnyEvent *> (ev);

    if (event->window == m_container->winId()) {
        if (ev->type == ClientMessage) {
            if (static_cast<unsigned long> (ev->xclient.data.l[0]) == XInternAtom(QX11Info::display(), "WM_DELETE_WINDOW", false)) {
                if (m_iconifyOnClose) {
                    iconifyWindow();
                } else {
                    closeWindow();
                }
                return true;
            }
        }

        if (event->type == DestroyNotify) {
            destroyEvent();
            return true;
        } else if (event->type == PropertyNotify) {
            return propertyChangeEvent(reinterpret_cast<XPropertyEvent *> (event)->atom);
        } else if (event->type == VisibilityNotify) {
            if (reinterpret_cast<XVisibilityEvent *> (event)->state == VisibilityFullyObscured) {
                obscureEvent();
                return true;
            }
        } else if (event->type == FocusOut) {
            focusLostEvent();
            return true;
        }
    } else if (event->window == m_window) {
        if (event->type == PropertyNotify) {
            return updateEmbedProperty(reinterpret_cast<XPropertyEvent *> (event)->atom);
        }
    }
    return false;
}

void TrayItem::restoreWindow() {
    m_container->show();

    Display *display = QX11Info::display();
    Window root = QX11Info::appRootWindow();
    XRaiseWindow(display, m_container->winId());

    // Change to the desktop that the window was last on.
    long l_currDesk[2] = {m_desktop, CurrentTime};
    XLibUtil::sendMessage(display, root, root, "_NET_CURRENT_DESKTOP", 32, SubstructureNotifyMask | SubstructureRedirectMask, l_currDesk, sizeof (l_currDesk));
    // Set the desktop the window wants to be on.
    long l_wmDesk[2] = {m_desktop, 1}; // 1 == request sent from application. 2 == from pager
    XLibUtil::sendMessage(display, root, m_container->winId(), "_NET_WM_DESKTOP", 32, SubstructureNotifyMask | SubstructureRedirectMask, l_wmDesk, sizeof (l_wmDesk));

    m_container->activateWindow();
    updateToggleAction();
    doSkipTaskbar();
}

void TrayItem::iconifyWindow() {
    m_container->hide();
    updateToggleAction();
}

void TrayItem::closeWindow() {
    Display *display = QX11Info::display();
    long l[2] = {XInternAtom(QX11Info::display(), "WM_DELETE_WINDOW", false), CurrentTime};
    restoreWindow();
    XLibUtil::sendMessage(display, m_window, m_window, "WM_PROTOCOLS", 32, NoEventMask, l, sizeof (l));
}

void TrayItem::doSkipTaskbar() {
    set_NET_WM_STATE("_NET_WM_STATE_SKIP_TASKBAR", m_skipTaskbar);
}

void TrayItem::doSkipPager() {
    set_NET_WM_STATE("_NET_WM_STATE_SKIP_PAGER", m_skipPager);
}

void TrayItem::doSticky() {
    set_NET_WM_STATE("_NET_WM_STATE_STICKY", m_sticky);
}

void TrayItem::setCustomIcon(QString path) {
    m_customIcon = true;
    QPixmap customIcon;
    if (!customIcon.load(path)) {
        customIcon.load(":/images/question.png");
    }

    setIcon(QIcon(customIcon));
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
    doSkipTaskbar();
}

void TrayItem::setSkipPager(bool value) {
    m_skipPager = value;
    m_actionSkipPager->setChecked(value);
    doSkipPager();
}

void TrayItem::setSticky(bool value) {
    m_sticky = value;
    m_actionSticky->setChecked(value);
    doSticky();
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

void TrayItem::setIconifyOnClose(bool value) {
    m_iconifyOnClose = value;
    m_actionIconifyOnClose->setChecked(value);
}

void TrayItem::setBalloonTimeout(int value) {
    if (value < 0) {
        value = 0;
    }
    m_balloonTimeout = value;
    m_actionBalloonTitleChanges->setChecked(value ? true : false);
}

void TrayItem::setBalloonTimeout(bool value) {
    if (!value) {
        setBalloonTimeout(-1);
    } else {
        setBalloonTimeout(4000);
    }
}

void TrayItem::toggleWindow() {
    if (m_container->isVisible()) {
        iconifyWindow();
    } else {
        restoreWindow();
    }
}

void TrayItem::trayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        if (m_container->winId() == XLibUtil::activeWindow(QX11Info::display())) {
            iconifyWindow();
        } else {
            restoreWindow();
        }
    }
}

void TrayItem::doAbout() {
    emit(about());
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

bool TrayItem::propertyChangeEvent(Atom property) {
    Display *display = QX11Info::display();
    static Atom WM_STATE = XInternAtom(display, "WM_STATE", True);
    static Atom _NET_WM_DESKTOP = XInternAtom(display, "_NET_WM_DESKTOP", True);

    if (property == _NET_WM_DESKTOP) {
        XLibUtil::getCardinalProperty(display, m_container->winId(), _NET_WM_DESKTOP, &m_desktop);
        return true;
    } else if (property == WM_STATE) {
        Atom type = None;
        int format;
        unsigned long nitems, after;
        unsigned char *data = 0;
        int r = XGetWindowProperty(display, m_container->winId(), WM_STATE, 0, 1, False, AnyPropertyType, &type, &format, &nitems, &after, &data);
        if ((r == Success) && data && (*reinterpret_cast<long *> (data) == IconicState)) {
            minimizeEvent();
            XFree(data);
            return true;
        }
    }
    return false;
}

bool TrayItem::updateEmbedProperty(Atom property) {
    Display *display = QX11Info::display();
    static Atom WM_NAME = XInternAtom(display, "WM_NAME", True);
    static Atom WM_ICON = XInternAtom(display, "WM_ICON", True);

    if (property == WM_NAME) {
        updateTitle();
        return true;
    } else if (property == WM_ICON) {
        updateIcon();
        return true;
    }
    return false;
}

void TrayItem::obscureEvent() {
    if (m_iconifyObscure) {
        iconifyWindow();
    }
}

void TrayItem::focusLostEvent() {
    // Wait half a second before checking to ensure the window is properly
    // focused when being restored.
    QTime t;
    t.start();
    while (t.elapsed() < 500) {
        qApp->processEvents();
    }

    if (m_iconifyFocusLost && m_container->winId() != XLibUtil::activeWindow(QX11Info::display())) {
        iconifyWindow();
    }
}

void TrayItem::set_NET_WM_STATE(const char *type, bool set) {
    // set, true = add the state to the window. False, remove the state from
    // the window.
    Display *display = QX11Info::display();
    Atom atom = XInternAtom(display, type, False);

    long l[2] = {set ? 1 : 0, atom};
    XLibUtil::sendMessage(display, QX11Info::appRootWindow(), m_container->winId(), "_NET_WM_STATE", 32, SubstructureNotifyMask, l, sizeof (l));
}

void TrayItem::readDockedAppName() {
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
    Display *display = QX11Info::display();
    char *windowName = 0;
    QString title;

    XFetchName(display, m_window, &windowName);
    title = windowName;
    if (windowName) {
        XFree(windowName);
    }

    m_container->setWindowTitle(title);

    setToolTip(QString("%1 [%2]").arg(title).arg(m_dockedAppName));
    if (m_balloonTimeout > 0) {
        showMessage(m_dockedAppName, title, QSystemTrayIcon::Information, m_balloonTimeout);
    }
}

void TrayItem::updateIcon() {
    if (m_customIcon) {
        return;
    }

    QIcon icon = createIcon(m_window);

    m_container->setWindowIcon(icon);
    setIcon(icon);
}

void TrayItem::updateToggleAction() {
    QString text;
    QIcon icon;

    if (m_container->isVisible()) {
        text = tr("Hide %1").arg(m_dockedAppName);
        icon = QIcon(":/images/iconify.png");
    } else {
        text = tr("Show %1").arg(m_dockedAppName);
        icon = QIcon(":/images/restore.png");
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
    connect(m_actionIconifyFocusLost, SIGNAL(toggled(bool)), this, SLOT(setIconifyFocusLost(bool)));
    m_optionsMenu->addAction(m_actionIconifyFocusLost);

    m_actionIconifyOnClose = new QAction(tr("Iconify on close"), m_optionsMenu);
    m_actionIconifyOnClose->setCheckable(true);
    m_actionIconifyOnClose->setChecked(m_iconifyOnClose);
    connect(m_actionIconifyOnClose, SIGNAL(toggled(bool)), this, SLOT(setIconifyOnClose(bool)));
    m_optionsMenu->addAction(m_actionIconifyOnClose);

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
    m_contextMenu->addAction(QIcon(":/images/close.png"), tr("Close"), this, SLOT(closeWindow()));

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
