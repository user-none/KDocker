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
#include "trayitemconfig.h"
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
    connect(m_scanner, SIGNAL(windowFound(Window, TrayItemConfig)), this, SLOT(dockWindow(Window, TrayItemConfig)));
    connect(m_scanner, SIGNAL(stopping()), this, SLOT(checkCount()));
    m_grabInfo.qtimer = new QTimer;
    m_grabInfo.qloop  = new QEventLoop;
    m_grabInfo.isGrabbing = false;
    connect(m_grabInfo.qtimer, SIGNAL(timeout()), m_grabInfo.qloop, SLOT(quit()));
    connect(this, SIGNAL(quitMouseGrab()),        m_grabInfo.qloop, SLOT(quit()));

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

void TrayItemManager::processCommand(const Command &command, const TrayItemConfig &config) {
    Window window;

    switch (command.getType()) {
        case Command::CommandType::NoCommand:
            checkCount();
            return;
        case Command::CommandType::Title:
            m_scanner->enqueueSearch(command.getSearchPattern(), config, command.getTimeout(), command.getCheckNormality());
            checkCount();
            break;
        case Command::CommandType::WindowId:
            window = command.getWindowId();
            if (!XLibUtil::isValidWindowId(XLibUtil::display(), window)) {
                QMessageBox::critical(0, qApp->applicationName(), tr("Invalid window id"));
                checkCount();
                return;
            }
            dockWindow(window, config);
            break;
        case Command::CommandType::Pid:
            window = XLibUtil::pidToWid(XLibUtil::display(), XLibUtil::appRootWindow(), command.getCheckNormality(), command.getPid(), dockedWindows());
            if (!XLibUtil::isValidWindowId(XLibUtil::display(), window)) {
                QMessageBox::critical(0, qApp->applicationName(), tr("Invalid window id"));
                checkCount();
                return;
            }
            dockWindow(window, config);
            break;
        case Command::CommandType::Run:
            m_scanner->enqueueRun(command.getRunApp(), command.getRunAppArguments(), config, command.getTimeout(), command.getCheckNormality(), command.getSearchPattern());
            checkCount();
            return;
        case Command::CommandType::Select:
            window = userSelectWindow(command.getCheckNormality());
            if (window) {
                dockWindow(window, config);
            }
            checkCount();
            break;
        case Command::CommandType::Focused:
            window = XLibUtil::activeWindow(XLibUtil::display());
            if (!window) {
                QMessageBox::critical(0, qApp->applicationName(), tr("Cannot dock the active window because no window has focus"));
                checkCount();
                return;
            }
            dockWindow(window, config);
            break;
    }
}

// XXX
void TrayItemManager::processCommandSearch() {}
void TrayItemManager::processCommandRun() {}

void TrayItemManager::processCommandWindowId(int wid, const TrayItemConfig &config) {
    Window window = wid;
    if (!XLibUtil::isValidWindowId(XLibUtil::display(), window)) {
        QMessageBox::critical(0, qApp->applicationName(), tr("Invalid window id"));
        checkCount();
        return;
    }
    dockWindow(window, config);
}

void TrayItemManager::processCommandPid(int pid, bool checkNormality, const TrayItemConfig &config) {
    Window window = XLibUtil::pidToWid(XLibUtil::display(), XLibUtil::appRootWindow(), checkNormality, pid, dockedWindows());
    if (!XLibUtil::isValidWindowId(XLibUtil::display(), window)) {
        QMessageBox::critical(0, qApp->applicationName(), tr("Invalid window id"));
        checkCount();
        return;
    }
    dockWindow(window, config);
}

void TrayItemManager::selectWindow(bool checkNormality, const TrayItemConfig &config) {
    Window window = userSelectWindow(checkNormality);
    if (window) {
        dockWindow(window, config);
    }
    checkCount();
}

void TrayItemManager::processCommandFocused(const TrayItemConfig &config) {
    Window window = XLibUtil::activeWindow(XLibUtil::display());
    if (!window) {
        QMessageBox::critical(0, qApp->applicationName(), tr("Cannot dock the active window because no window has focus"));
        checkCount();
        return;
    }
    dockWindow(window, config);
}

void TrayItemManager::dockWindow(Window window, const TrayItemConfig &settings) {
    if (isWindowDocked(window)) {
        QMessageBox::information(0, qApp->applicationName(), tr("This window is already docked.\nClick on system tray icon to toggle docking."));
        checkCount();
        return;
    }

    TrayItem *ti = new TrayItem(window, settings);

    connect(ti, SIGNAL(selectAnother()), this, SLOT(selectAndIconify()));
    connect(ti, SIGNAL(dead(TrayItem*)), this, SLOT(remove(TrayItem*)));
    connect(ti, SIGNAL(undock(TrayItem*)), this, SLOT(undock(TrayItem*)));
    connect(ti, SIGNAL(undockAll()), this, SLOT(undockAll()));
    connect(ti, SIGNAL(about()), this, SLOT(about()));

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
#if 0
    QMessageBox aboutBox;
    aboutBox.setIconPixmap(QPixmap(":/images/kdocker.png"));
    aboutBox.setWindowTitle(tr("About %1 - %2").arg(qApp->applicationName()).arg(qApp->applicationVersion()));
    aboutBox.setText(Constants::ABOUT_MESSAGE);
    aboutBox.setInformativeText(tr("See %1 for more information.").arg("<a href=\"https://github.com/user-none/KDocker\">https://github.com/user-none/KDocker</a>"));
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
#endif
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
