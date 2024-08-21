/*
 *  Copyright (C) 2009, 2012, 2015 John Schember <john@nachtimwald.com>
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

#include <QCoreApplication>
#include <QByteArray>
#include <QMessageBox>
#include <QTextStream>

#include "constants.h"
#include "trayitemoptions.h"
#include "trayitemmanager.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <xcb/xproto.h>
#include "xlibutil.h"

#define  ESC_key  9

int ignoreXErrors(Display *, XErrorEvent *) {
    return 0;
}

TrayItemManager::TrayItemManager() {
    m_daemon = false;
    m_scanner = new Scanner(this);
    connect(m_scanner, &Scanner::windowFound, this, &TrayItemManager::dockWindow);
    connect(m_scanner, &Scanner::stopping, this, &TrayItemManager::checkCount);
    m_grabInfo.qtimer = new QTimer;
    m_grabInfo.qloop  = new QEventLoop;
    m_grabInfo.isGrabbing = false;
    connect(m_grabInfo.qtimer, &QTimer::timeout, m_grabInfo.qloop, &QEventLoop::quit);
    connect(this, &TrayItemManager::quitMouseGrab, m_grabInfo.qloop, &QEventLoop::quit);

    // This will prevent x errors from being written to the console.
    // The isValidWindowId function in util.cpp will generate errors if the
    // window is not valid while it is checking.
    XSetErrorHandler(ignoreXErrors);
    qApp-> installNativeEventFilter(this);
}

TrayItemManager::~TrayItemManager() {
    while (!m_trayItems.isEmpty()) {
        TrayItem *t = m_trayItems.takeFirst();
        undock(t);
        delete t;
    }
    delete m_grabInfo.qtimer;
    delete m_grabInfo.qloop;
    delete m_scanner;
    qApp-> removeNativeEventFilter(this);
}

bool TrayItemManager::nativeEventFilter([[maybe_unused]] const QByteArray &eventType, void *message, [[maybe_unused]] qintptr *result) {
    static xcb_window_t dockedWindow = 0;  //     zero: event ignored (default) ...
                                           // non-zero: pass to TrayItem::xcbEventFilter
    switch (static_cast<xcb_generic_event_t *>(message)-> response_type & ~0x80) {
        case XCB_FOCUS_OUT:          // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_focus_out_event_t *>(message)-> event;
            break;

        case XCB_DESTROY_NOTIFY:     // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_destroy_notify_event_t *>(message)-> window;
            break;

        case XCB_UNMAP_NOTIFY:       // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_unmap_notify_event_t *>(message)-> window;
            break;

        case XCB_MAP_NOTIFY:         // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_map_notify_event_t *>(message)-> window;
            break;

        case XCB_VISIBILITY_NOTIFY:  // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_visibility_notify_event_t *>(message)-> window;
            break;

        case XCB_PROPERTY_NOTIFY:    // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_visibility_notify_event_t *>(message)-> window;
            break;

        case XCB_BUTTON_PRESS:
            if (m_grabInfo.isGrabbing) {
                m_grabInfo.isGrabbing = false;   // Cancel immediately

                m_grabInfo.button = static_cast<xcb_button_press_event_t *>(message)-> detail;
                m_grabInfo.window = static_cast<xcb_button_press_event_t *>(message)-> child;

                emit quitMouseGrab();            // Interrupt QTimer waiting for grab
                return true;                     // Event has been handled - don't propagate
            }
            break;

        case XCB_KEY_RELEASE:
            if (m_grabInfo.isGrabbing) {
                if (static_cast<xcb_key_release_event_t *>(message)-> detail == ESC_key)
                {
                    m_grabInfo.isGrabbing = false;

                    emit quitMouseGrab();        // Interrupt QTimer waiting for grab
                    return true;                 // Event has been handled - don't propagate
                }
            }
            break;
    }

    if (dockedWindow) {
        // Pass on the event to the tray item with the associated window.
        QListIterator<TrayItem*> ti(m_trayItems);
        static TrayItem *t;

        while (ti.hasNext()) {
            t = ti.next();
            if (t-> dockedWindow() == static_cast<Window>(dockedWindow)) {
                return t-> xcbEventFilter(static_cast<xcb_generic_event_t *>(message), dockedWindow);
            }
        }
    }

    return false;
}

void TrayItemManager::dockWindowTitle(const QString &searchPattern, uint timeout, bool checkNormality, const TrayItemOptions &options) {
    m_scanner->enqueueSearch(QRegularExpression(searchPattern), timeout, checkNormality, options);
    checkCount();
}

void TrayItemManager::dockLaunchApp(const QString &app, const QStringList &appArguments, const QString &searchPattern, uint timeout, bool checkNormality, const TrayItemOptions &options) {
    m_scanner->enqueueLaunch(app, appArguments, QRegularExpression(searchPattern), timeout, checkNormality, options);
    checkCount();
}

void TrayItemManager::dockWindowId(int wid, const TrayItemOptions &options) {
    Window window = wid;
    if (!XLibUtil::isValidWindowId(XLibUtil::display(), window)) {
        QMessageBox::critical(0, qApp->applicationName(), tr("Invalid window id"));
        checkCount();
        return;
    }
    dockWindow(window, options);
}

void TrayItemManager::dockPid(pid_t pid, bool checkNormality, const TrayItemOptions &options) {
    Window window = XLibUtil::pidToWid(XLibUtil::display(), XLibUtil::appRootWindow(), checkNormality, pid, dockedWindows());
    if (!XLibUtil::isValidWindowId(XLibUtil::display(), window)) {
        QMessageBox::critical(0, qApp->applicationName(), tr("Invalid window id"));
        checkCount();
        return;
    }
    dockWindow(window, options);
}

void TrayItemManager::dockSelectWindow(bool checkNormality, const TrayItemOptions &options) {
    Window window = userSelectWindow(checkNormality);
    if (window) {
        dockWindow(window, options);
    }
    checkCount();
}

void TrayItemManager::dockFocused(const TrayItemOptions &options) {
    Window window = XLibUtil::activeWindow(XLibUtil::display());
    if (!window) {
        QMessageBox::critical(0, qApp->applicationName(), tr("Cannot dock the active window because no window has focus"));
        checkCount();
        return;
    }
    dockWindow(window, options);
}

WindowNameMap TrayItemManager::listWindows() {
    WindowNameMap items;

    QListIterator<TrayItem *> ti(m_trayItems);
    while (ti.hasNext()) {
        TrayItem *trayItem = ti.next();
        items.insert(trayItem->dockedWindow(), trayItem->appName());
    }

    return items;
}

bool TrayItemManager::closeWindow(uint windowId) {
    QListIterator<TrayItem *> ti(m_trayItems);
    while (ti.hasNext()) {
        TrayItem *trayItem = ti.next();
        if (trayItem->dockedWindow() == static_cast<Window>(windowId)) {
            trayItem->closeWindow();
            return true;
        }
    }
    return false;
}

bool TrayItemManager::hideWindow(uint windowId) {
    QListIterator<TrayItem *> ti(m_trayItems);
    while (ti.hasNext()) {
        TrayItem *trayItem = ti.next();
        if (trayItem->dockedWindow() == static_cast<Window>(windowId)) {
            trayItem->iconifyWindow();
            return true;
        }
    }
    return false;
}

bool TrayItemManager::showWindow(uint windowId) {
    QListIterator<TrayItem *> ti(m_trayItems);
    while (ti.hasNext()) {
        TrayItem *trayItem = ti.next();
        if (trayItem->dockedWindow() == static_cast<Window>(windowId)) {
            trayItem->restoreWindow();
            return true;
        }
    }
    return false;
}

bool TrayItemManager::undockWindow(uint windowId) {
    QListIterator<TrayItem *> ti(m_trayItems);
    while (ti.hasNext()) {
        TrayItem *trayItem = ti.next();
        if (trayItem->dockedWindow() == static_cast<Window>(windowId)) {
            undock(trayItem);
            return true;
        }
    }
    return false;
}

void TrayItemManager::dockWindow(Window window, const TrayItemOptions &settings) {
    if (isWindowDocked(window)) {
        QMessageBox::information(0, qApp->applicationName(), tr("This window is already docked.\nClick on system tray icon to toggle docking."));
        checkCount();
        return;
    }

    TrayItem *ti = new TrayItem(window, settings);

    connect(ti, &TrayItem::selectAnother, this, &TrayItemManager::selectAndIconify);
    connect(ti, &TrayItem::dead, this, &TrayItemManager::remove);
    connect(ti, &TrayItem::undock, this, &TrayItemManager::undock);
    connect(ti, &TrayItem::undockAll, this, &TrayItemManager::undockAll);
    connect(ti, &TrayItem::about, this, &TrayItemManager::about);

    ti->showWindow();

    m_trayItems.append(ti);
}

Window TrayItemManager::userSelectWindow(bool checkNormality) {
    QTextStream out(stdout);
    out << tr("Select the application/window to dock with the left mouse button.") << Qt::endl;
    out << tr("Click any other mouse button to abort.") << Qt::endl;

    QString error;
    Window window = XLibUtil::selectWindow(XLibUtil::display(), m_grabInfo, error);
    if (!window) {
        if (error != QString()) {
            QMessageBox::critical(0, qApp->applicationName(), error);
        }
        checkCount();
        return 0;
    }

    if (checkNormality) {
        if (!XLibUtil::isNormalWindow(XLibUtil::display(), window)) {
            if (QMessageBox::warning(0, qApp->applicationName(), tr("The window you are attempting to dock does not seem to be a normal window."), QMessageBox::Abort | QMessageBox::Ignore) == QMessageBox::Abort) {
                checkCount();
                return 0;
            }
        }
    }

    return window;
}

void TrayItemManager::remove(TrayItem *trayItem) {
    m_trayItems.removeAll(trayItem);
    trayItem->deleteLater();

    checkCount();
}

void TrayItemManager::undock(TrayItem *trayItem) {
    trayItem->restoreWindow();
    trayItem->setSkipTaskbar(false);
    trayItem->doSkipTaskbar();
    remove(trayItem);
}

void TrayItemManager::undockAll() {
    Q_FOREACH(TrayItem *ti, m_trayItems) {
        undock(ti);
    }
}

void TrayItemManager::about() {
    QMessageBox aboutBox;
    aboutBox.setWindowTitle(tr("About"));
    aboutBox.setText(QString("" \
        "# %1\n" \
        "### Version %2\n\n" \
        "[Website](%3)")
            .arg(qApp->applicationName())
            .arg(qApp->applicationVersion())
            .arg(Constants::WEBSITE));
    aboutBox.setTextFormat(Qt::MarkdownText);
    aboutBox.setIconPixmap(QPixmap(":/images/kdocker.png"));
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

void TrayItemManager::daemonize() {
    m_daemon = true;
}

void TrayItemManager::selectAndIconify() {
    Window window = userSelectWindow(true);

    if (window) {
        dockWindow(window, m_initArgs);
    }
}

void TrayItemManager::quit() {
    undockAll();
    qApp->quit();
}

void TrayItemManager::checkCount() {
    if (m_daemon)
        return;

    if (m_trayItems.isEmpty() && !m_scanner->isRunning()) {
        qApp->quit();
    }
}

QList<Window> TrayItemManager::dockedWindows() {
    QList<Window> windows;

    QListIterator<TrayItem*> ti(m_trayItems);
    while (ti.hasNext()) {
        windows.append(ti.next()->dockedWindow());
    }

    return windows;
}

bool TrayItemManager::isWindowDocked(Window window) {
    return dockedWindows().contains(window);
}
