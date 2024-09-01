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

#include "trayitemmanager.h"
#include "constants.h"
#include "trayitemoptions.h"
#include "xlibutil.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTextStream>

#include <xcb/xproto.h>

#define ESC_key 9

TrayItemManager::TrayItemManager() : m_scanner(this)
{
    m_keepRunning = false;
    connect(&m_scanner, &Scanner::windowFound, this, &TrayItemManager::dockWindow);
    connect(&m_scanner, &Scanner::stopping, this, &TrayItemManager::checkCount);
    connect(this, &TrayItemManager::quitMouseGrab, &m_grabInfo, &GrabInfo::stop);

    qApp->installNativeEventFilter(this);
}

TrayItemManager::~TrayItemManager()
{
    while (!m_trayItems.isEmpty()) {
        TrayItem *t = m_trayItems.takeFirst();
        undockRestore(t);
        delete t;
    }
    qApp->removeNativeEventFilter(this);
}

bool TrayItemManager::nativeEventFilter([[maybe_unused]] const QByteArray &eventType, void *message,
                                        [[maybe_unused]] qintptr *result)
{
    static xcb_window_t dockedWindow = 0; //     zero: event ignored (default) ...
                                          // non-zero: pass to TrayItem::xcbEventFilter
    switch (static_cast<xcb_generic_event_t *>(message)->response_type & ~0x80) {
        case XCB_FOCUS_OUT: // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_focus_out_event_t *>(message)->event;
            break;

        case XCB_DESTROY_NOTIFY: // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_destroy_notify_event_t *>(message)->window;
            break;

        case XCB_UNMAP_NOTIFY: // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_unmap_notify_event_t *>(message)->window;
            break;

        case XCB_MAP_NOTIFY: // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_map_notify_event_t *>(message)->window;
            break;

        case XCB_VISIBILITY_NOTIFY: // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_visibility_notify_event_t *>(message)->window;
            break;

        case XCB_PROPERTY_NOTIFY: // -> TrayItem::xcbEventFilter
            dockedWindow = static_cast<xcb_visibility_notify_event_t *>(message)->window;
            break;

        case XCB_BUTTON_PRESS:
            if (m_grabInfo.isGrabbing()) {
                m_grabInfo.stopGrabbing();

                m_grabInfo.setButton(static_cast<xcb_button_press_event_t *>(message)->detail);
                m_grabInfo.setWindow(static_cast<xcb_button_press_event_t *>(message)->child);

                emit quitMouseGrab(); // Interrupt QTimer waiting for grab
                return true;          // Event has been handled - don't propagate
            }
            break;

        case XCB_KEY_RELEASE:
            if (m_grabInfo.isGrabbing()) {
                if (static_cast<xcb_key_release_event_t *>(message)->detail == ESC_key) {
                    m_grabInfo.stopGrabbing();

                    emit quitMouseGrab(); // Interrupt QTimer waiting for grab
                    return true;          // Event has been handled - don't propagate
                }
            }
            break;
    }

    if (dockedWindow) {
        // Pass on the event to the tray item with the associated window.
        for (auto &trayItem : std::as_const(m_trayItems)) {
            if (trayItem->dockedWindow() == static_cast<windowid_t>(dockedWindow)) {
                return trayItem->xcbEventFilter(message);
            }
        }
    }

    return false;
}

void TrayItemManager::dockWindowTitle(const QString &searchPattern, uint timeout, bool checkNormality,
                                      const TrayItemOptions &options)
{
    m_scanner.enqueueSearch(QRegularExpression(searchPattern), timeout, checkNormality, options);
    checkCount();
}

void TrayItemManager::dockLaunchApp(const QString &app, const QStringList &appArguments, const QString &searchPattern,
                                    uint timeout, bool checkNormality, const TrayItemOptions &options)
{
    m_scanner.enqueueLaunch(app, appArguments, QRegularExpression(searchPattern), timeout, checkNormality, options);
    checkCount();
}

bool TrayItemManager::dockWindowId(uint windowId, const TrayItemOptions &options)
{
    if (!XLibUtil::isValidWindowId(windowId)) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Invalid window id"));
        checkCount();
        return false;
    }
    dockWindow(windowId, options);
    return true;
}

bool TrayItemManager::dockPid(int pid, bool checkNormality, const TrayItemOptions &options)
{
    windowid_t window = XLibUtil::pidToWid(checkNormality, pid);
    if (!XLibUtil::isValidWindowId(window)) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Invalid pid"));
        checkCount();
        return false;
    }
    dockWindow(window, options);
    return true;
}

void TrayItemManager::dockSelectWindow(bool checkNormality, const TrayItemOptions &options)
{
    windowid_t window = userSelectWindow(checkNormality);
    if (window)
        dockWindow(window, options);
    checkCount();
}

void TrayItemManager::dockFocused(const TrayItemOptions &options)
{
    windowid_t window = XLibUtil::getActiveWindow();
    if (!window) {
        QMessageBox::critical(nullptr, tr("Error"), tr("Cannot dock the active window because no window has focus"));
        checkCount();
        return;
    }
    dockWindow(window, options);
}

WindowNameMap TrayItemManager::listWindows()
{
    WindowNameMap items;

    for (auto &trayItem : std::as_const(m_trayItems)) {
        items.insert(trayItem->dockedWindow(), trayItem->appName());
    }

    return items;
}

bool TrayItemManager::closeWindow(uint windowId)
{
    for (auto &trayItem : std::as_const(m_trayItems)) {
        if (trayItem->dockedWindow() == static_cast<windowid_t>(windowId)) {
            trayItem->closeWindow();
            return true;
        }
    }
    return false;
}

bool TrayItemManager::hideWindow(uint windowId)
{
    for (auto &trayItem : std::as_const(m_trayItems)) {
        if (trayItem->dockedWindow() == static_cast<windowid_t>(windowId)) {
            trayItem->iconifyWindow();
            return true;
        }
    }
    return false;
}

bool TrayItemManager::showWindow(uint windowId)
{
    for (auto &trayItem : std::as_const(m_trayItems)) {
        if (trayItem->dockedWindow() == static_cast<windowid_t>(windowId)) {
            trayItem->restoreWindow();
            return true;
        }
    }
    return false;
}

bool TrayItemManager::undockWindow(uint windowId)
{
    for (size_t i = m_trayItems.count(); i-- > 0;) {
        auto *trayItem = m_trayItems[i];
        if (trayItem->dockedWindow() == static_cast<windowid_t>(windowId)) {
            undockRestore(trayItem);

            trayItem->deleteLater();
            m_trayItems.remove(i);

            checkCount();
            return true;
        }
    }
    return false;
}

void TrayItemManager::dockWindow(windowid_t window, const TrayItemOptions &settings)
{
    if (isWindowDocked(window)) {
        QMessageBox::information(nullptr, tr("Info"), tr("This window is already docked\nClick on system tray icon to toggle docking."));
        checkCount();
        return;
    }

    TrayItem *ti = new TrayItem(window, settings);

    connect(ti, &TrayItem::selectAnother, this, &TrayItemManager::selectAndIconify);
    connect(ti, &TrayItem::dead, this, &TrayItemManager::remove);
    connect(ti, &TrayItem::undock, this, &TrayItemManager::remove);
    connect(ti, &TrayItem::undockAll, this, &TrayItemManager::undockAll);
    connect(ti, &TrayItem::about, this, &TrayItemManager::about);

    ti->show();

    m_trayItems.append(ti);
}

windowid_t TrayItemManager::userSelectWindow(bool checkNormality)
{
    QTextStream out(stdout);
    out << tr("Select the application/window to dock with the left mouse button.") << Qt::endl;
    out << tr("Click any other mouse button to abort.") << Qt::endl;

    QString error;
    windowid_t window = XLibUtil::selectWindow(m_grabInfo, error);
    if (!window) {
        if (error != QString()) {
            QMessageBox::critical(nullptr, tr("Error"), error);
        }
        checkCount();
        return 0;
    }

    if (checkNormality) {
        if (!XLibUtil::isNormalWindow(window)) {
            auto ret = QMessageBox::warning(nullptr, tr("Warning"), tr("The window you are attempting to dock does not seem to be a normal window"),
                QMessageBox::Abort | QMessageBox::Ignore, QMessageBox::Abort);
            if (ret == QMessageBox::Abort) {
                checkCount();
                return 0;
            }
        }
    }

    return window;
}

void TrayItemManager::remove(TrayItem *trayItem)
{
    m_trayItems.removeAll(trayItem);
    trayItem->deleteLater();

    checkCount();
}

void TrayItemManager::undockRestore(TrayItem *trayItem)
{
    trayItem->restoreWindow();
    trayItem->setSkipTaskbar(false);
    trayItem->doSkipTaskbar();
}

void TrayItemManager::undockAll()
{
    while (!m_trayItems.isEmpty()) {
        TrayItem *t = m_trayItems.takeFirst();
        undockRestore(t);
        t->deleteLater();
    }

    checkCount();
}

void TrayItemManager::about()
{
    QMessageBox::about(nullptr, tr("About"), QString("%1\nVersion: %2").arg(qApp->applicationName()).arg(qApp->applicationVersion()));
}

void TrayItemManager::keepRunning()
{
    m_keepRunning = true;
}

void TrayItemManager::selectAndIconify()
{
    windowid_t window = userSelectWindow(true);
    if (window)
        dockWindow(window, TrayItemOptions());
}

void TrayItemManager::quit()
{
    undockAll();
    qApp->quit();
}

void TrayItemManager::checkCount()
{
    if (m_keepRunning)
        return;

    if (m_trayItems.isEmpty() && !m_scanner.isRunning())
        qApp->quit();
}

QList<windowid_t> TrayItemManager::dockedWindows()
{
    QList<windowid_t> windows;

    for (auto &trayItem : std::as_const(m_trayItems)) {
        windows.append(trayItem->dockedWindow());
    }

    return windows;
}

bool TrayItemManager::isWindowDocked(windowid_t window)
{
    for (auto &trayItem : std::as_const(m_trayItems)) {
        if (trayItem->dockedWindow() == window) {
            return true;
        }
    }
    return false;
}
