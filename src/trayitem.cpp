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

#include <QApplication>
#include <QFileDialog>
#include <QImageReader>
#include <QMessageBox>
#include <QPixmap>
#include <QElapsedTimer>
#include <QWheelEvent>
#include <QImage>
#include <QIcon>

#include <X11/Xatom.h>

#include "trayitem.h"
#include "xlibutil.h"

TrayItem::TrayItem(Window window, const TrayItemOptions &args) {
    m_wantsAttention = false;
    m_iconified = false;
    m_is_restoring = false;
    m_customIcon = false;

    m_dockedAppName = "";
    m_window = window;

    Display *display = XLibUtil::display();

    // Allows events from m_window to be forwarded to the x11EventFilter.
    XLibUtil::subscribe(display, m_window, StructureNotifyMask | PropertyChangeMask | VisibilityChangeMask | FocusChangeMask);

    // Store the desktop on which the window is being shown.
    XLibUtil::getCardinalProperty(display, m_window, XInternAtom(display, "_NET_WM_DESKTOP", True), &m_desktop);

    readDockedAppName();

    m_settings.setIconPath(          readSetting(args.getIconPath(),               "CustomIcon",       TrayItemOptions::defaultIconPath()));
    m_settings.setAttentionIconPath( readSetting(args.getAttentionIconPath(),      "AttentionIcon",    TrayItemOptions::defaultAttentionIconPath()));
    m_settings.setNotifyTime(        readSetting(args.getNotifyTime(),             "BalloonTimeout",   TrayItemOptions::defaultNotifyTime()));
    m_settings.setSticky(            readSetting(args.getStickyState(),            "Sticky",           TrayItemOptions::defaultSticky()));
    m_settings.setSkipPager(         readSetting(args.getSkipPagerState(),         "SkipPager",        TrayItemOptions::defaultSkipPager()));
    m_settings.setSkipTaskbar(       readSetting(args.getSkipTaskbarState(),       "SkipTaskbar",      TrayItemOptions::defaultSkipTaskbar()));
    m_settings.setIconifyMinimized(  readSetting(args.getIconifyMinimizedState(),  "IconifyMinimized", TrayItemOptions::defaultIconifyMinimized()));
    m_settings.setIconifyObscured(   readSetting(args.getIconifyObscuredState(),   "IconifyObscured",  TrayItemOptions::defaultIconifyObscured()));
    m_settings.setIconifyFocusLost(  readSetting(args.getIconifyFocusLostState(),  "IconifyFocusLost", TrayItemOptions::defaultIconifyFocusLost()));
    m_settings.setLockToDesktop(     readSetting(args.getLockToDesktopState(),     "LockToDesktop",    TrayItemOptions::defaultLockToDesktop()));

    updateTitle();
    updateIcon();

    createContextMenu();
    updateToggleAction();

    if (!m_settings.getIconPath().isEmpty()) {
        setCustomIcon(m_settings.getIconPath());
    }
    if (!m_settings.getAttentionIconPath().isEmpty()) {
        setAttentionIcon(m_settings.getAttentionIconPath());
    }
    setBalloonTimeout(m_settings.getNotifyTime());
    setSticky(m_settings.getSticky());
    setSkipPager(m_settings.getSkipPager());
    setSkipTaskbar(m_settings.getSkipTaskbar());
    setIconifyMinimized(m_settings.getIconifyMinimized());
    setIconifyObscured(m_settings.getIconifyObscured());
    setIconifyFocusLost(m_settings.getIconifyFocusLost());
    setLockToDesktop(m_settings.getLockToDesktop());

    connect(this, &TrayItem::activated, this, &TrayItem::trayActivated);
}

TrayItem::~TrayItem() {
    // No further interest in events from undocked window.
    XLibUtil::unSubscribe(XLibUtil::display(), m_window);
    // Only the main menu needs to be deleted. The rest of the menus and actions
    // are children of this menu and Qt will delete all children.
    delete m_contextMenu;
}

bool TrayItem::readSetting(TrayItemOptions::TriState argSetting, QString key, bool kdockerDefault) {
    /* Precedence:
     * 1) Command line overrides         (argSetting, if positive)
     * 2) User app-specific defaults     (QSettings: "<m_dockedAppName>/<key>")
     * 3) User global defaults           (QSettings: "_GLOBAL_DEFAULTS/<key>")
     * 4) KDocker defaults               (#define DEFAULT_keyname)
     */
    if (argSetting != TrayItemOptions::TriState::Unset) {
        return (argSetting == TrayItemOptions::TriState::SetTrue ? true : false);
    }
    key.prepend("%1/");   // Add formatting to local QString copy
    return m_config.value(key.arg(m_dockedAppName),
            m_config.value(key.arg("_GLOBAL_DEFAULTS"), kdockerDefault)).toBool();
}

int TrayItem::readSetting(int argSetting, QString key, int kdockerDefault) {
    if (argSetting >= 0) {
        return argSetting;
    }
    key.prepend("%1/");
    return m_config.value(key.arg(m_dockedAppName),
            m_config.value(key.arg("_GLOBAL_DEFAULTS"), kdockerDefault)).toInt();
}

QString TrayItem::readSetting(const QString &argSetting, QString key, const QString &kdockerDefault) {
    if (!argSetting.isEmpty()) {
        return argSetting;
    }
    key.prepend("%1/");
    return m_config.value(key.arg(m_dockedAppName),
            m_config.value(key.arg("_GLOBAL_DEFAULTS"), kdockerDefault)).toString();
}

int TrayItem::nonZeroBalloonTimeout() {
    QString fmt = "%1/BalloonTimeout";
    int bto = m_config.value(fmt.arg(m_dockedAppName), 0).toInt();
    if (!bto) {
        bto = m_config.value(fmt.arg("_GLOBAL_DEFAULTS"), 0).toInt();
    }
    return bto ? bto : TrayItemOptions::defaultNotifyTime();
}

TrayItemOptions TrayItem::readConfigGlobals() {
    TrayItemOptions config;

    m_config.beginGroup("_GLOBAL_DEFAULTS");
      config.setIconPath(          m_config.value("CustomIcon",       config.getIconPath()).toString());
      config.setAttentionIconPath( m_config.value("AttentionIcon",    config.getAttentionIconPath()).toString());
      config.setNotifyTime(        m_config.value("BalloonTimeout",   config.getNotifyTime()).toInt());
      config.setSticky(            m_config.value("Sticky",           config.getSticky()).toBool());
      config.setSkipPager(         m_config.value("SkipPager",        config.getSkipPager()).toBool());
      config.setSkipTaskbar(       m_config.value("SkipTaskbar",      config.getSkipTaskbar()).toBool());
      config.setIconifyMinimized(  m_config.value("IconifyMinimized", config.getIconifyMinimized()).toBool());
      config.setIconifyObscured(   m_config.value("IconifyObscured",  config.getIconifyObscured()).toBool());
      config.setIconifyFocusLost(  m_config.value("IconifyFocusLost", config.getIconifyFocusLost()).toBool());
      config.setLockToDesktop(     m_config.value("LockToDesktop",    config.getLockToDesktop()).toBool());
    m_config.endGroup();

    return config;
}

void TrayItem::saveSettingsGlobal()
{
    m_config.beginGroup("_GLOBAL_DEFAULTS");
      saveSettings();
    m_config.endGroup();
}

void TrayItem::saveSettingsApp()
{
    TrayItemOptions globals = readConfigGlobals();

    m_config.beginGroup(m_dockedAppName);
      QVariant keyval;

      if (!m_settings.getIconPath().isEmpty()) {
          m_config.setValue("CustomIcon", m_settings.getIconPath());
      }
      if (!m_settings.getAttentionIconPath().isEmpty()) {
          m_config.setValue("AttentionIcon", m_settings.getAttentionIconPath());
      }
      saveSettings();

      // Remove app-specific settings if they match their default values

      keyval = m_config.value("BalloonTimeout");
      if (keyval.isValid() && (keyval.toInt()  == globals.getNotifyTime())) {
          m_config.remove("BalloonTimeout");
      }
      keyval = m_config.value("Sticky");
      if (keyval.isValid() && keyval.toBool() == globals.getSticky()) {
          m_config.remove("Sticky");
      }
      keyval = m_config.value("SkipPager");
      if (keyval.isValid() && keyval.toBool() == globals.getSkipPager()) {
          m_config.remove("SkipPager");
      }
      keyval = m_config.value("SkipTaskbar");
      if (keyval.isValid() && keyval.toBool() == globals.getSkipTaskbar()) {
          m_config.remove("SkipTaskbar");
      }
      keyval = m_config.value("IconifyMinimized");
      if (keyval.isValid() && keyval.toBool() == globals.getIconifyMinimized()) {
          m_config.remove("IconifyMinimized");
      }
      keyval = m_config.value("IconifyObscured");
      if (keyval.isValid() && keyval.toBool() == globals.getIconifyObscured()) {
          m_config.remove("IconifyObscured");
      }
      keyval = m_config.value("IconifyFocusLost");
      if (keyval.isValid() && keyval.toBool() == globals.getIconifyFocusLost()) {
          m_config.remove("IconifyFocusLost");
      }
      keyval = m_config.value("LockToDesktop");
      if (keyval.isValid() && keyval.toBool() == globals.getLockToDesktop()) {
          m_config.remove("LockToDesktop");
      }
    m_config.endGroup();
}

void TrayItem::saveSettings() {    /*  "/home/<user>/.config/com.kdocker/KDocker.conf"    //  <==  m_config.fileName();  */
    // Group is set by caller
    m_config.setValue("BalloonTimeout",   m_settings.getNotifyTime());
    m_config.setValue("Sticky",           m_settings.getSticky());
    m_config.setValue("SkipPager",        m_settings.getSkipPager());
    m_config.setValue("SkipTaskbar",      m_settings.getSkipTaskbar());
    m_config.setValue("IconifyMinimized", m_settings.getIconifyMinimized());
    m_config.setValue("IconifyObscured",  m_settings.getIconifyObscured());
    m_config.setValue("IconifyFocusLost", m_settings.getIconifyFocusLost());
    m_config.setValue("LockToDesktop",    m_settings.getLockToDesktop());
}

bool TrayItem::xcbEventFilter(xcb_generic_event_t *event, xcb_window_t dockedWindow) {
    if (!isBadWindow() && static_cast<Window>(dockedWindow) == m_window) {
        switch (event-> response_type & ~0x80) {
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
                if (reinterpret_cast<xcb_visibility_notify_event_t *>(event)-> state == XCB_VISIBILITY_FULLY_OBSCURED) {
                    obscureEvent();
                }
                break;

            case XCB_PROPERTY_NOTIFY:
                propertyChangeEvent(static_cast<Atom>(reinterpret_cast<xcb_property_notify_event_t *>(event)-> atom));
                break;
        }
    }
    return false;
}


Window TrayItem::dockedWindow() {
    return m_window;
}

void TrayItem::showWindow() {

    show();

    if (m_settings.getIconifyMinimized()) {
        iconifyWindow();
    } else {
        if (m_settings.getSkipTaskbar()) {
            doSkipTaskbar();
        }
    }
}

void TrayItem::restoreWindow() {
    if (isBadWindow()) {
        return;
    }

    m_is_restoring = true;

    Display *display = XLibUtil::display();
    Window root = XLibUtil::appRootWindow();

    if (m_iconified) {
        m_iconified = false;
        XMapWindow(display, m_window);
        m_sizeHint.flags = USPosition;
        XSetWMNormalHints(display, m_window, &m_sizeHint);

        updateToggleAction();

        if (m_wantsAttention) {
            m_wantsAttention = false;
            setIcon(m_defaultIcon);
        }
    }
    XMapRaised(display, m_window);
    XFlush(display);

    // Change to the desktop that the window was last on.
    long l_currDesk[2] = {m_desktop, CurrentTime};
    XLibUtil::sendMessage(display, root, root, "_NET_CURRENT_DESKTOP", 32, SubstructureNotifyMask | SubstructureRedirectMask, l_currDesk, sizeof (l_currDesk));

    if (m_settings.getLockToDesktop()) {
        // Set the desktop the window wants to be on.
        long l_wmDesk[2] = {m_desktop, 1}; // 1 == request sent from application. 2 == from pager
        XLibUtil::sendMessage(display, root, m_window, "_NET_WM_DESKTOP", 32, SubstructureNotifyMask | SubstructureRedirectMask, l_wmDesk, sizeof (l_wmDesk));
    }

    // Make it the active window
    // 1 == request sent from application. 2 == from pager.
    // We use 2 because KWin doesn't always give the window focus with 1.
    long l_active[2] = {2, CurrentTime};
    XLibUtil::sendMessage(display, root, m_window, "_NET_ACTIVE_WINDOW", 32, SubstructureNotifyMask | SubstructureRedirectMask, l_active, sizeof (l_active));
    XSetInputFocus(display, m_window, RevertToParent, CurrentTime);

    updateToggleAction();
    doSkipTaskbar();

    /*
     * Wait half a second to ensure the window is fully restored.
     * This and m_is_restoring are a work around for KWin.
     * KWin is the only WM that will send a PropertyNotify
     * event with the Iconic state set because of the above
     * XIconifyWindow call.
     */
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < 500) {
        qApp->processEvents();
    }

    m_is_restoring = false;
}

void TrayItem::iconifyWindow() {
    if (isBadWindow() || m_is_restoring) {
        return;
    }

    m_iconified = true;

    /* Get screen number */
    Display *display = XLibUtil::display();
    int screen = DefaultScreen(display);
    long dummy;

    XGetWMNormalHints(display, m_window, &m_sizeHint, &dummy);

    /*
     * A simple call to XWithdrawWindow wont do. Here is what we do:
     * 1. Iconify. This will make the application hide all its other windows. For
     *    example, xmms would take off the playlist and equalizer window.
     * 2. Withdraw the window to remove it from the taskbar.
     */
    XIconifyWindow(display, m_window, screen); // good for effects too
    XSync(display, False);
    XWithdrawWindow(display, m_window, screen);

    updateToggleAction();
}

void TrayItem::closeWindow() {    
    if (isBadWindow()) {
        return;
    }

    Display *display = XLibUtil::display();
    long l[5] = {0, 0, 0, 0, 0};
    restoreWindow();
    XLibUtil::sendMessage(display, XLibUtil::appRootWindow(), m_window, "_NET_CLOSE_WINDOW", 32, SubstructureNotifyMask | SubstructureRedirectMask, l, sizeof (l));
}

void TrayItem::doSkipTaskbar() {
    set_NET_WM_STATE("_NET_WM_STATE_SKIP_TASKBAR", m_settings.getSkipTaskbar());
}

void TrayItem::doSkipPager() {
    set_NET_WM_STATE("_NET_WM_STATE_SKIP_PAGER", m_settings.getSkipPager());
}

void TrayItem::doSticky() {
    set_NET_WM_STATE("_NET_WM_STATE_STICKY", m_settings.getSticky());
}

QString TrayItem::appName() {
    return m_dockedAppName;
}

void TrayItem::setCustomIcon(QString path) {
    m_customIcon = true;

    QPixmap customIcon;
    if (customIcon.load(path)) {
        m_settings.setIconPath(path);
    } else {
        customIcon.load(":/images/question.png");
    }

    m_defaultIcon = QIcon(customIcon);

    if (!m_wantsAttention) {
        setIcon(m_defaultIcon);
    }
}

void TrayItem::setAttentionIcon(QString path) {
    QPixmap icon;
    if (icon.load(path)) {
        m_settings.setAttentionIconPath(path);
    } else {
        icon.load(":/images/question.png");
    }

    m_attentionIcon = QIcon(icon);

    if (m_wantsAttention) {
        setIcon(m_attentionIcon);
    }
}

QString TrayItem::selectIcon(QString title) {
    QStringList types;
    QString supportedTypes;

    Q_FOREACH(QByteArray type, QImageReader::supportedImageFormats()) {
        types << QString(type);
    }
    if (types.isEmpty()) {
        supportedTypes = "All Files (*.*)";
    } else {
        supportedTypes = QString("Images (*.%1);;All Files (*.*)").arg(types.join(" *."));
    }

    return QFileDialog::getOpenFileName(0, title, QDir::homePath(), supportedTypes);
}

void TrayItem::selectCustomIcon([[maybe_unused]] bool value) {
    QString path = selectIcon(tr("Select Icon"));
    if (!path.isEmpty()) {
        setCustomIcon(path);
    }
}

void TrayItem::selectAttentionIcon([[maybe_unused]] bool value) {
    QString path = selectIcon(tr("Select Attention Icon"));
    if (!path.isEmpty()) {
        setAttentionIcon(path);
    }
}

void TrayItem::setSkipTaskbar(bool value) {
    m_settings.setSkipTaskbar(value);
    m_actionSkipTaskbar->setChecked(value);
    doSkipTaskbar();
}

void TrayItem::setSkipPager(bool value) {
    m_settings.setSkipPager(value);
    m_actionSkipPager->setChecked(value);
    doSkipPager();
}

void TrayItem::setSticky(bool value) {
    m_settings.setSticky(value);
    m_actionSticky->setChecked(value);
    doSticky();
}

void TrayItem::setIconifyMinimized(bool value) {
    m_settings.setIconifyMinimized(value);
    m_actionIconifyMinimized->setChecked(value);
}

void TrayItem::setIconifyObscured(bool value) {
    m_settings.setIconifyObscured(value);
    m_actionIconifyObscured->setChecked(value);
}

void TrayItem::setIconifyFocusLost(bool value) {
    m_settings.setIconifyFocusLost(value);
    m_actionIconifyFocusLost->setChecked(value);
    focusLostEvent();
}

void TrayItem::setLockToDesktop(bool value) {
    m_settings.setLockToDesktop(value);
    m_actionLockToDesktop->setChecked(value);
}

void TrayItem::setBalloonTimeout(int value) {
    if (value < 0) {
        value = 0;
    }
    m_settings.setNotifyTime(value);
    m_actionBalloonTitleChanges->setChecked(value ? true : false);
}

void TrayItem::setBalloonTimeout(bool value) {
    if (!value) {
        setBalloonTimeout(-1);
    } else {
        setBalloonTimeout(nonZeroBalloonTimeout());
    }
}

void TrayItem::toggleWindow() {
    if (m_iconified || m_window != XLibUtil::activeWindow(XLibUtil::display())) {
        if (!m_iconified) {
            // Iconify on original desktop in case restoring to another
            iconifyWindow();
        }
        restoreWindow();
    } else {
        iconifyWindow();
    }
}

void TrayItem::trayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        toggleWindow();
    }
}

bool TrayItem::event(QEvent *e) {
    if (e->type() == QEvent::Wheel) {
        QWheelEvent *we = static_cast<QWheelEvent*>(e);
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

void TrayItem::doUndock() {
    emit undock(this);
}

void TrayItem::minimizeEvent() {
    if (m_settings.getIconifyMinimized()) {
        iconifyWindow();
    }
}

void TrayItem::destroyEvent() {
    m_window = 0;
    emit dead(this);
}

void TrayItem::propertyChangeEvent(Atom property) {
    if (isBadWindow()) {
        return;
    }

    Display *display = XLibUtil::display();
    static Atom WM_NAME         = XInternAtom(display, "WM_NAME", True);
    static Atom WM_ICON         = XInternAtom(display, "WM_ICON", True);
    static Atom WM_STATE        = XInternAtom(display, "WM_STATE", True);
    static Atom _NET_WM_DESKTOP = XInternAtom(display, "_NET_WM_DESKTOP", True);

    if (property == WM_NAME) {
        updateTitle();
    } else if (property == WM_ICON) {
        updateIcon();
    } else if (property == _NET_WM_DESKTOP) {
        XLibUtil::getCardinalProperty(display, m_window, _NET_WM_DESKTOP, &m_desktop);
    } else if (property == WM_STATE) {
        Atom type = None;
        int format;
        unsigned long nitems, after;
        unsigned char *data = 0;
        int r = XGetWindowProperty(display, m_window, WM_STATE, 0, 1, False, AnyPropertyType, &type, &format, &nitems, &after, &data);
        if ((r == Success) && data && (*reinterpret_cast<long *> (data) == IconicState)) {
            // KDE 5.14 started issuing this event when the user changes virtual desktops so
            // a minimizeEvent() should not be executed unless the window is on the currently
            // visible desktop
            if (isOnCurrentDesktop()) {
                minimizeEvent();
            }
        }
        XFree(data);
    }
}

void TrayItem::obscureEvent() {
    if (m_settings.getIconifyObscured()) {
        iconifyWindow();
    }
}

void TrayItem::focusLostEvent() {
    // Wait half a second before checking to ensure the window is properly
    // focused when being restored.
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < 500) {
        qApp->processEvents();
    }

    if (m_settings.getIconifyFocusLost() && m_window != XLibUtil::activeWindow(XLibUtil::display())) {
        iconifyWindow();
    }
}

void TrayItem::set_NET_WM_STATE(const char *type, bool set) {    
    if (isBadWindow()) {
        return;
    }

    // set, true = add the state to the window. False, remove the state from
    // the window.
    Display *display = XLibUtil::display();
    Atom atom = XInternAtom(display, type, False);

    qint64 l[2] = {set ? 1 : 0, static_cast<qint64>(atom)};
    XLibUtil::sendMessage(display, XLibUtil::appRootWindow(), m_window, "_NET_WM_STATE", 32, SubstructureNotifyMask, l, sizeof (l));
}

void TrayItem::readDockedAppName() {
    if (isBadWindow()) {
        return;
    }

    Display *display = XLibUtil::display();
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

    Display *display = XLibUtil::display();
    char *windowName = 0;
    QString title;

    XFetchName(display, m_window, &windowName);
    title = windowName;
    if (windowName) {
        XFree(windowName);
    }

    setToolTip(QString("%1 [%2]").arg(title).arg(m_dockedAppName));
    if (m_settings.getNotifyTime() > 0) {
        showMessage(m_dockedAppName, title, QSystemTrayIcon::Information, m_settings.getNotifyTime());
    }

    if (m_iconified && !m_attentionIcon.isNull() && !m_wantsAttention) {
        m_wantsAttention = true;
        setIcon(m_attentionIcon);
    }
}

void TrayItem::updateIcon() {
    if (isBadWindow() || m_customIcon) {
        return;
    }

    m_defaultIcon = createIcon(m_window);
    if (!m_wantsAttention) {
        setIcon(m_defaultIcon);
    }
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

    m_contextMenu->addAction(QIcon(":/images/about.png"), tr("About %1").arg(qApp->applicationName()), this, &TrayItem::about);
    m_contextMenu->addSeparator();

    // Options menu
    m_optionsMenu = new QMenu(tr("Options"), m_contextMenu);
    m_optionsMenu-> setIcon(QIcon(":/images/options.png"));

    m_actionSetIcon = new QAction(QIcon(":/images/seticon.png"), tr("Set icon..."), m_optionsMenu);
    connect(m_actionSetIcon, &QAction::triggered, this, &TrayItem::selectCustomIcon);
    m_optionsMenu->addAction(m_actionSetIcon);

    m_actionSetAttentionIcon = new QAction(QIcon(":/images/setaicon.png"), tr("Set attention icon..."), m_optionsMenu);
    connect(m_actionSetAttentionIcon, &QAction::triggered, this, &TrayItem::selectAttentionIcon);
    m_optionsMenu->addAction(m_actionSetAttentionIcon);

    m_actionSkipTaskbar = new QAction(tr("Skip taskbar"), m_optionsMenu);
    m_actionSkipTaskbar->setCheckable(true);
    m_actionSkipTaskbar->setChecked(m_settings.getSkipTaskbar());
    connect(m_actionSkipTaskbar, &QAction::triggered, this, &TrayItem::setSkipTaskbar);
    m_optionsMenu->addAction(m_actionSkipTaskbar);

    m_actionSkipPager = new QAction(tr("Skip pager"), m_optionsMenu);
    m_actionSkipPager->setCheckable(true);
    m_actionSkipPager->setChecked(m_settings.getSkipPager());
    connect(m_actionSkipPager, &QAction::triggered, this, &TrayItem::setSkipPager);
    m_optionsMenu->addAction(m_actionSkipPager);

    m_actionSticky = new QAction(tr("Sticky"), m_optionsMenu);
    m_actionSticky->setCheckable(true);
    m_actionSticky->setChecked(m_settings.getSticky());
    connect(m_actionSticky, &QAction::triggered, this, &TrayItem::setSticky);
    m_optionsMenu->addAction(m_actionSticky);

    m_actionIconifyMinimized = new QAction(tr("Iconify when minimized"), m_optionsMenu);
    m_actionIconifyMinimized->setCheckable(true);
    m_actionIconifyMinimized->setChecked(m_settings.getIconifyMinimized());
    connect(m_actionIconifyMinimized, &QAction::triggered, this, &TrayItem::setIconifyMinimized);
    m_optionsMenu->addAction(m_actionIconifyMinimized);

    m_actionIconifyObscured = new QAction(tr("Iconify when obscured"), m_optionsMenu);
    m_actionIconifyObscured->setCheckable(true);
    m_actionIconifyObscured->setChecked(m_settings.getIconifyObscured());
    connect(m_actionIconifyObscured, &QAction::triggered, this, &TrayItem::setIconifyObscured);
    m_optionsMenu->addAction(m_actionIconifyObscured);

    m_actionIconifyFocusLost = new QAction(tr("Iconify when focus lost"), m_optionsMenu);
    m_actionIconifyFocusLost->setCheckable(true);
    m_actionIconifyFocusLost->setChecked(m_settings.getIconifyFocusLost());
    connect(m_actionIconifyFocusLost, &QAction::toggled, this, &TrayItem::setIconifyFocusLost);
    m_optionsMenu->addAction(m_actionIconifyFocusLost);

    m_actionLockToDesktop = new QAction(tr("Lock to desktop"), m_optionsMenu);
    m_actionLockToDesktop->setCheckable(true);
    m_actionLockToDesktop->setChecked(m_settings.getLockToDesktop());
    connect(m_actionLockToDesktop, &QAction::toggled, this, &TrayItem::setLockToDesktop);
    m_optionsMenu->addAction(m_actionLockToDesktop);

    m_actionBalloonTitleChanges = new QAction(tr("Balloon title changes"), m_optionsMenu);
    m_actionBalloonTitleChanges->setCheckable(true);
    m_actionBalloonTitleChanges->setChecked(m_settings.getNotifyTime() ? true : false);
    connect(m_actionBalloonTitleChanges, &QAction::triggered, this, qOverload<bool>(&TrayItem::setBalloonTimeout));
    m_optionsMenu->addAction(m_actionBalloonTitleChanges);

    m_contextMenu->addMenu(m_optionsMenu);

    // Save settings menu
    m_optionsMenu->addSeparator();
    m_defaultsMenu = new QMenu(tr("Save settings"), m_optionsMenu);
    m_defaultsMenu-> setIcon(QIcon(":/images/savesettings.png"));

    m_actionSaveSettingsApp = new QAction(tr("%1 only").arg(m_dockedAppName), m_defaultsMenu);
    connect(m_actionSaveSettingsApp, &QAction::triggered, this, &TrayItem::saveSettingsApp);
    m_defaultsMenu->addAction(m_actionSaveSettingsApp);

    m_actionSaveSettingsGlobal = new QAction(tr("Global (all new)"), m_defaultsMenu);
    connect(m_actionSaveSettingsGlobal, &QAction::triggered, this, &TrayItem::saveSettingsGlobal);
    m_defaultsMenu->addAction(m_actionSaveSettingsGlobal);

    m_optionsMenu->addMenu(m_defaultsMenu);
    // ---

    m_contextMenu->addAction(QIcon(":/images/another.png"), tr("Dock Another"), this, &TrayItem::selectAnother);
    m_contextMenu->addAction(QIcon(":/images/undockall.png"), tr("Undock All"), this, &TrayItem::undockAll);
    m_contextMenu->addSeparator();
    m_actionToggle = new QAction(tr("Toggle"), m_contextMenu);
    connect(m_actionToggle, &QAction::triggered, this, &TrayItem::toggleWindow);
    m_contextMenu->addAction(m_actionToggle);
    m_contextMenu->addAction(QIcon(":/images/undock.png"), tr("Undock"), this, &TrayItem::doUndock);
    m_contextMenu->addAction(QIcon(":/images/close.png"), tr("Close"), this, &TrayItem::closeWindow);

    setContextMenu(m_contextMenu);
}

QRgb convertToQColor(unsigned long pixel) {
    return qRgba((pixel & 0x00FF0000) >> 16, // Red
                 (pixel & 0x0000FF00) >> 8,  // Green
                 (pixel & 0x000000FF),       // Blue
                 (pixel & 0xFF000000) >> 24); // Alpha
}

QImage imageFromX11IconData(unsigned long* iconData, unsigned long dataLength) {
    if (!iconData || dataLength < 2) {
        return QImage();
    }

    unsigned long width = iconData[0];
    unsigned long height = iconData[1];

    if (width == 0 || height == 0 || dataLength < width * height + 2) {
        return QImage();
    }

    QVector<QRgb> pixels(width * height);
    unsigned long* src = iconData + 2;
    for (unsigned long i = 0; i < width * height; ++i) {
        pixels[i] = convertToQColor(src[i]);
    }

    QImage iconImage((uchar*)pixels.data(), width, height, QImage::Format_ARGB32);
    return iconImage.copy();
}

QImage imageFromX11Pixmap(Display* display, Pixmap pixmap, int width, int height) {
    XImage* ximage = XGetImage(display, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);
    if (!ximage) {
        return QImage();
    }

    QImage image((uchar*)ximage->data, width, height, QImage::Format_ARGB32);
    QImage result = image.copy(); // Make a copy of the image data
    XDestroyImage(ximage);

    return result;
}

QIcon TrayItem::createIcon(Window window) {
    if (!window) {
        return QIcon();
    }

    Display* display = XLibUtil::display();
    QPixmap appIcon;

    // First try to get the icon from WM_HINTS
    XWMHints* wm_hints = XGetWMHints(display, window);
    if (wm_hints != nullptr) {
        if (!(wm_hints->flags & IconMaskHint)) {
            wm_hints->icon_mask = None;
        }

        if ((wm_hints->flags & IconPixmapHint) && (wm_hints->icon_pixmap)) {
            Window root;
            int x, y;
            unsigned int width, height, border_width, depth;
            XGetGeometry(display, wm_hints->icon_pixmap, &root, &x, &y, &width, &height, &border_width, &depth);

            QImage image = imageFromX11Pixmap(display, wm_hints->icon_pixmap, width, height);
            appIcon = QPixmap::fromImage(image);
        }

        XFree(wm_hints);
    }

    // Fallback to _NET_WM_ICON if WM_HINTS icon is not available
    if (appIcon.isNull()) {
        Atom netWmIcon = XInternAtom(display, "_NET_WM_ICON", False);
        Atom actualType;
        int actualFormat;
        unsigned long nItems, bytesAfter;
        unsigned char* data = nullptr;

        if (XGetWindowProperty(display, window, netWmIcon, 0, LONG_MAX, False, XA_CARDINAL,
                               &actualType, &actualFormat, &nItems, &bytesAfter, &data) == Success && data) {
            unsigned long* iconData = reinterpret_cast<unsigned long*>(data);
            unsigned long dataLength = nItems;

            // Extract the largest icon available
            QImage largestImage;
            unsigned long maxIconSize = 0;

            for (unsigned long i = 0; i < dataLength; ) {
                unsigned long width = iconData[i];
                unsigned long height = iconData[i + 1];
                unsigned long iconSize = width * height;

                if (iconSize > maxIconSize) {
                    largestImage = imageFromX11IconData(&iconData[i], dataLength - i);
                    maxIconSize = iconSize;
                }

                i += (2 + iconSize);
            }

            if (!largestImage.isNull()) {
                appIcon = QPixmap::fromImage(largestImage);
            }

            XFree(data);
        }
    }

    if (appIcon.isNull()) {
        appIcon.load(":/images/question.png");
    }

    return QIcon(appIcon);
}

bool TrayItem::isBadWindow() {
    Display *display = XLibUtil::display();

    if (!XLibUtil::isValidWindowId(display, m_window)) {
        destroyEvent();
        return true;
    }
    return false;
}

// Checks to see if the virtual desktop the window is on is currently 
// displayed. Returns true if it is, otherwise false
bool TrayItem::isOnCurrentDesktop() {
    Display *display = XLibUtil::display();
    Atom type = None;
    int format;
    unsigned long nitems, after;
    unsigned char *data = 0;

    static Atom _NET_CURRENT_DESKTOP = XInternAtom(display, "_NET_CURRENT_DESKTOP", True);

    long currentDesktop;
    int r = XGetWindowProperty(display, DefaultRootWindow(display), _NET_CURRENT_DESKTOP, 0, 4, False,
                           AnyPropertyType, &type, &format,     
                           &nitems, &after, &data);
    if (r == Success && data) 
        currentDesktop = *reinterpret_cast<long *> (data);
    else
        currentDesktop = m_desktop;

    XFree(data);

    return (currentDesktop == m_desktop);
}
