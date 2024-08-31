/*
 *  Copyright (C) 2009, 2015 John Schember <john@nachtimwald.com>
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

#include "trayitem.h"
#include "xlibutil.h"

#include <QElapsedTimer>
#include <QFileDialog>
#include <QIcon>
#include <QImageReader>
#include <QPixmap>
#include <QWheelEvent>

#include <xcb/xproto.h>

static const QString GLOBALSKEY = "_GLOBAL_DEFAULTS";

TrayItem::TrayItem(windowid_t window, const TrayItemOptions &args)
{
    m_wantsAttention = false;
    m_iconified = false;
    m_customIcon = false;

    m_dockedAppName = "";
    m_window = window;

    m_sizeHint = XLibUtil::newSizeHints();

    // Allows events from m_window to be forwarded to the x11EventFilter.
    XLibUtil::subscribe(m_window);

    // Store the desktop on which the window is being shown.
    m_desktop = XLibUtil::getWindowDesktop(m_window);

    readDockedAppName();
    m_settings.loadSettings(m_dockedAppName, args);
    updateTitle();
    updateIcon();

    createContextMenu();
    updateToggleAction();

    doSkipTaskbar();
    doSkipPager();
    doSticky();
    focusLostEvent();

    connect(this, &TrayItem::activated, this, &TrayItem::trayActivated);
}

TrayItem::~TrayItem()
{
    // No further interest in events from undocked window.
    XLibUtil::unSubscribe(m_window);
    XLibUtil::deleteSizeHints(m_sizeHint);
}

bool TrayItem::xcbEventFilter(void *message)
{
    if (isBadWindow())
        return false;

    xcb_generic_event_t *event = static_cast<xcb_generic_event_t *>(message);
    switch (event->response_type & ~0x80) {
        case XCB_FOCUS_OUT:
            focusLostEvent();
            break;

        case XCB_DESTROY_NOTIFY:
            destroyEvent();
            // return true;
            break;

        case XCB_UNMAP_NOTIFY:
            // In KDE 5.14 they started issuing an unmap event when the user
            // changes virtual desktops so we need to check that the window
            // is on the current desktop before saying that it has been iconized
            if (isOnCurrentDesktop()) {
                m_iconified = true;
                updateToggleAction();
            }
            break;

        case XCB_MAP_NOTIFY:
            m_iconified = false;
            updateToggleAction();
            break;

        case XCB_VISIBILITY_NOTIFY:
            if (reinterpret_cast<xcb_visibility_notify_event_t *>(event)->state == XCB_VISIBILITY_FULLY_OBSCURED) {
                obscureEvent();
            }
            break;

        case XCB_PROPERTY_NOTIFY: {
            if (isBadWindow())
                break;

            static atom_t WM_NAME = XLibUtil::getAtom("WM_NAME");
            static atom_t WM_ICON = XLibUtil::getAtom("WM_ICON");
            static atom_t WM_STATE = XLibUtil::getAtom("WM_STATE");
            static atom_t _NET_WM_DESKTOP = XLibUtil::getAtom("_NET_WM_DESKTOP");

            atom_t property = static_cast<atom_t>(reinterpret_cast<xcb_property_notify_event_t *>(event)->atom);
            if (property == WM_NAME) {
                updateTitle();
            } else if (property == WM_ICON) {
                updateIcon();
            } else if (property == _NET_WM_DESKTOP) {
                m_desktop = XLibUtil::getWindowDesktop(m_window);
            } else if (property == WM_STATE) {
                // KDE 5.14 started issuing this event when the user changes virtual desktops so
                // a minimizeEvent() should not be executed unless the window is on the currently
                // visible desktop
                if (XLibUtil::isWindowIconic(m_window) && isOnCurrentDesktop()) {
                    minimizeEvent();
                }
            }
            break;
        }

        default:
            // Not an event we care about let someone else deal with it
            return false;
    }

    // We handled the end so it doesn't need to be probigated
    return true;
}

windowid_t TrayItem::dockedWindow()
{
    return m_window;
}

void TrayItem::show()
{
    doSkipTaskbar();
    iconifyWindow();
    QSystemTrayIcon::show();
}

void TrayItem::restoreWindow()
{
    if (isBadWindow())
        return;

    if (m_iconified) {
        m_iconified = false;
        XLibUtil::setWMSizeHints(m_window, m_sizeHint);
        updateToggleAction();

        if (m_wantsAttention) {
            m_wantsAttention = false;
            setIcon(m_defaultIcon);
        }
    }
    XLibUtil::raiseWindow(m_window);

    // Change to the desktop that the window was last on.
    XLibUtil::setCurrentDesktop(m_desktop);

    if (m_settings.getLockToDesktop()) {
        // Set the desktop the window wants to be on.
        XLibUtil::setWindowDesktop(m_desktop, m_window);
    }

    // Make it the active window
    XLibUtil::setActiveWindow(m_window);

    updateToggleAction();
    doSkipTaskbar();
}

void TrayItem::iconifyWindow()
{
    if (isBadWindow())
        return;

    m_iconified = true;

    // Get screen number
    XLibUtil::getWMSizeHints(m_window, m_sizeHint);
    XLibUtil::iconifyWindow(m_window);
    updateToggleAction();
}

void TrayItem::closeWindow()
{
    if (isBadWindow())
        return;

    restoreWindow();
    XLibUtil::closeWindow(m_window);
}

void TrayItem::doSkipTaskbar()
{
    if (isBadWindow())
        return;

    XLibUtil::setWindowSkipTaskbar(m_window, m_settings.getSkipTaskbar());
}

void TrayItem::doSkipPager()
{
    if (isBadWindow())
        return;

    XLibUtil::setWindowSkipPager(m_window, m_settings.getSkipPager());
}

void TrayItem::doSticky()
{
    if (isBadWindow())
        return;

    XLibUtil::setWindowSticky(m_window, m_settings.getSticky());
}

QString TrayItem::appName()
{
    return m_dockedAppName;
}

void TrayItem::setCustomIcon(QString path)
{
    m_customIcon = true;

    QPixmap customIcon;
    if (customIcon.load(path)) {
        m_settings.setIconPath(path);
    } else {
        customIcon.load(":/menu/question.png");
    }

    m_defaultIcon = QIcon(customIcon);

    if (!m_wantsAttention)
        setIcon(m_defaultIcon);
}

void TrayItem::setAttentionIcon(QString path)
{
    QPixmap icon;
    if (icon.load(path)) {
        m_settings.setAttentionIconPath(path);
    } else {
        icon.load(":/menu/question.png");
    }

    m_attentionIcon = QIcon(icon);

    if (m_wantsAttention)
        setIcon(m_attentionIcon);
}

QString TrayItem::selectIcon(QString title)
{
    QStringList types;
    QString supportedTypes;

    for (QByteArray type : QImageReader::supportedImageFormats()) {
        types << QString(type);
    }
    if (types.isEmpty()) {
        supportedTypes = "All Files (*.*)";
    } else {
        supportedTypes = QString("Images (*.%1);;All Files (*.*)").arg(types.join(" *."));
    }

    return QFileDialog::getOpenFileName(0, title, QDir::homePath(), supportedTypes);
}

void TrayItem::selectCustomIcon([[maybe_unused]] bool value)
{
    QString path = selectIcon(tr("Select Icon"));
    if (!path.isEmpty())
        setCustomIcon(path);
}

void TrayItem::selectAttentionIcon([[maybe_unused]] bool value)
{
    QString path = selectIcon(tr("Select Attention Icon"));
    if (!path.isEmpty())
        setAttentionIcon(path);
}

void TrayItem::setSkipTaskbar(bool value)
{
    m_settings.setSkipTaskbar(value);
    doSkipTaskbar();
}

void TrayItem::setSkipPager(bool value)
{
    m_settings.setSkipPager(value);
    doSkipPager();
}

void TrayItem::setSticky(bool value)
{
    m_settings.setSticky(value);
    doSticky();
}

void TrayItem::setIconifyMinimized(bool value)
{
    m_settings.setIconifyMinimized(value);
}

void TrayItem::setIconifyObscured(bool value)
{
    m_settings.setIconifyObscured(value);
}

void TrayItem::setIconifyFocusLost(bool value)
{
    m_settings.setIconifyFocusLost(value);
    focusLostEvent();
}

void TrayItem::setLockToDesktop(bool value)
{
    m_settings.setLockToDesktop(value);
}

void TrayItem::setBalloonTimeout(bool value)
{
    m_settings.setQuiet(!value);
}

void TrayItem::toggleWindow()
{
    if (m_iconified || m_window != XLibUtil::getActiveWindow()) {
        // Iconify on original desktop in case restoring to another
        if (!m_iconified && !isOnCurrentDesktop()) {
            iconifyWindow();
        }
        restoreWindow();
    } else {
        iconifyWindow();
    }
}

void TrayItem::trayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
        toggleWindow();
}

bool TrayItem::event(QEvent *e)
{
    if (e->type() == QEvent::Wheel) {
        QWheelEvent *we = static_cast<QWheelEvent *>(e);
        QPoint delta = we->angleDelta();
        if (!delta.isNull() && delta.y() != 0) {
            if (delta.y() > 0) {
                restoreWindow();
            } else {
                iconifyWindow();
            }
            return true;
        }
    }
    return QSystemTrayIcon::event(e);
}

void TrayItem::doUndock()
{
    restoreWindow();
    setSkipTaskbar(false);
    doSkipTaskbar();

    emit undock(this);
}

void TrayItem::minimizeEvent()
{
    if (m_settings.getIconifyMinimized())
        iconifyWindow();
}

void TrayItem::destroyEvent()
{
    m_window = 0;
    emit dead(this);
}

void TrayItem::obscureEvent()
{
    if (m_settings.getIconifyObscured())
        iconifyWindow();
}

void TrayItem::focusLostEvent()
{
    if (m_settings.getIconifyFocusLost())
        iconifyWindow();
}

void TrayItem::readDockedAppName()
{
    if (isBadWindow())
        return;

    m_dockedAppName = XLibUtil::getAppName(m_window);
}

// Update the title in the tooltip.
void TrayItem::updateTitle()
{
    if (isBadWindow())
        return;

    QString title = XLibUtil::getWindowTitle(m_window);

    setToolTip(QString("%1 [%2]").arg(title).arg(m_dockedAppName));
    if (!m_settings.getQuiet() && m_settings.getNotifyTime() > 0)
        showMessage(m_dockedAppName, title, QSystemTrayIcon::Information, m_settings.getNotifyTime());

    if (m_iconified && !m_attentionIcon.isNull() && !m_wantsAttention) {
        m_wantsAttention = true;
        setIcon(m_attentionIcon);
    }
}

void TrayItem::updateIcon()
{
    if (isBadWindow() || m_customIcon)
        return;

    QPixmap pm = XLibUtil::getWindowIcon(m_window);
    if (pm.isNull())
        pm.load(":/menu/question.png");
    m_defaultIcon = QIcon(pm);

    if (!m_wantsAttention)
        setIcon(m_defaultIcon);
}

void TrayItem::updateToggleAction()
{
    QString text;
    QIcon icon;

    if (m_iconified) {
        text = tr("Show %1").arg(m_dockedAppName);
        icon = QIcon(":/menu/restore.png");
    } else {
        text = tr("Hide %1").arg(m_dockedAppName);
        icon = QIcon(":/menu/iconify.png");
    }

    m_actionToggle->setIcon(icon);
    m_actionToggle->setText(text);
}

void TrayItem::createContextMenu()
{
    m_contextMenu.addAction(QIcon(":/menu/about.png"), tr("About %1").arg(qApp->applicationName()), this, &TrayItem::about);
    m_contextMenu.addSeparator();

    // Options menu
    QMenu *optionsMenu = m_contextMenu.addMenu(tr("Options"));
    optionsMenu->setIcon(QIcon(":/menu/options.png"));

    QAction *action = optionsMenu->addAction(QIcon(":/menu/seticon.png"), tr("Set icon..."), this, &TrayItem::selectCustomIcon);
    action = optionsMenu->addAction(QIcon(":/menu/setaicon.png"), tr("Set attention icon..."), this, &TrayItem::selectAttentionIcon);
    optionsMenu->addSeparator();

    action = optionsMenu->addAction(tr("Skip taskbar"), this, &TrayItem::setSkipTaskbar);
    action->setCheckable(true);
    action->setChecked(m_settings.getSkipTaskbar());

    action = optionsMenu->addAction(tr("Skip pager"), this, &TrayItem::setSkipPager);
    action->setCheckable(true);
    action->setChecked(m_settings.getSkipPager());

    action = optionsMenu->addAction(tr("Sticky"), this, &TrayItem::setSticky);
    action->setCheckable(true);
    action->setChecked(m_settings.getSticky());

    action = optionsMenu->addAction(tr("Iconify when minimized"), this, &TrayItem::setIconifyMinimized);
    action->setCheckable(true);
    action->setChecked(m_settings.getIconifyMinimized());

    action = optionsMenu->addAction(tr("Iconify when obscured"), this, &TrayItem::setIconifyObscured);
    action->setCheckable(true);
    action->setChecked(m_settings.getIconifyObscured());

    action = optionsMenu->addAction(tr("Iconify when focus lost"), this, &TrayItem::setIconifyFocusLost);
    action->setCheckable(true);
    action->setChecked(m_settings.getIconifyFocusLost());

    action = optionsMenu->addAction(tr("Lock to desktop"), this, &TrayItem::setLockToDesktop);
    action->setCheckable(true);
    action->setChecked(m_settings.getLockToDesktop());

    action = optionsMenu->addAction(tr("Balloon title changes"), this, &TrayItem::setBalloonTimeout);
    action->setCheckable(true);
    action->setChecked(!m_settings.getQuiet());

    // Save settings menu
    optionsMenu->addSeparator();
    QMenu *menu = optionsMenu->addMenu(QIcon(":/menu/savesettings.png"), tr("Save settings"));
    menu->addAction(tr("%1 only").arg(m_dockedAppName), &m_settings, &TrayItemSettings::saveSettingsApp);
    menu->addAction(tr("Global (all new)"), &m_settings, &TrayItemSettings::saveSettingsGlobal);

    // ---

    m_contextMenu.addAction(QIcon(":/menu/another.png"), tr("Dock Another"), this, &TrayItem::selectAnother);
    m_contextMenu.addAction(QIcon(":/menu/undockall.png"), tr("Undock All"), this, &TrayItem::undockAll);

    m_contextMenu.addSeparator();

    m_actionToggle = m_contextMenu.addAction(tr("Toggle"), this, &TrayItem::toggleWindow);
    m_contextMenu.addAction(QIcon(":/menu/undock.png"), tr("Undock"), this, &TrayItem::doUndock);
    m_contextMenu.addAction(QIcon(":/menu/close.png"), tr("Close"), this, &TrayItem::closeWindow);

    setContextMenu(&m_contextMenu);
}

bool TrayItem::isBadWindow()
{
    if (!XLibUtil::isValidWindowId(m_window)) {
        destroyEvent();
        return true;
    }
    return false;
}

// Checks to see if the virtual desktop the window is on is currently
// displayed. Returns true if it is, otherwise false
bool TrayItem::isOnCurrentDesktop()
{
    long currentDesktop = XLibUtil::getCurrentDesktop();
    if (currentDesktop == -1)
        return true;
    return (currentDesktop == m_desktop);
}
