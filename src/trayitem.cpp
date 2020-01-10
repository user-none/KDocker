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
#include <QTime>
#include <QX11Info>
#include <QWheelEvent>

#include <Xatom.h>
#include <X11/xpm.h>

#include "trayitem.h"
#include "xlibutil.h"


TrayItem::TrayItem(Window window, const TrayItemArgs args) {
    m_iconified = false;
    m_is_restoring = false;
    m_customIcon = false;

    m_dockedAppName = "";
    m_window = window;

    Display *display = QX11Info::display();

    // Allows events from m_window to be forwarded to the x11EventFilter.
    XLibUtil::subscribe(display, m_window, StructureNotifyMask | PropertyChangeMask | VisibilityChangeMask | FocusChangeMask);

    // Store the desktop on which the window is being shown.
    XLibUtil::getCardinalProperty(display, m_window, XInternAtom(display, "_NET_WM_DESKTOP", True), &m_desktop);

    readDockedAppName();

    m_settings.sCustomIcon           = readSetting(args.sCustomIcon,           "CustomIcon",       DEFAULT_CustomIcon);
    m_settings.iBalloonTimeout       = readSetting(args.iBalloonTimeout,       "BalloonTimeout",   DEFAULT_BalloonTimeout);
    m_settings.opt[Sticky]           = readSetting(args.opt[Sticky],           "Sticky",           DEFAULT_Sticky);
    m_settings.opt[SkipPager]        = readSetting(args.opt[SkipPager],        "SkipPager",        DEFAULT_SkipPager);
    m_settings.opt[SkipTaskbar]      = readSetting(args.opt[SkipTaskbar],      "SkipTaskbar",      DEFAULT_SkipTaskbar);
    m_settings.opt[IconifyMinimized] = readSetting(args.opt[IconifyMinimized], "IconifyMinimized", DEFAULT_IconifyMinimized);
    m_settings.opt[IconifyObscured]  = readSetting(args.opt[IconifyObscured],  "IconifyObscured",  DEFAULT_IconifyObscured);
    m_settings.opt[IconifyFocusLost] = readSetting(args.opt[IconifyFocusLost], "IconifyFocusLost", DEFAULT_IconifyFocusLost);
    m_settings.opt[LockToDesktop]    = readSetting(args.opt[LockToDesktop],    "LockToDesktop",    DEFAULT_LockToDesktop);

    updateTitle();
    updateIcon();

    createContextMenu();
    updateToggleAction();

    if (!m_settings.sCustomIcon.isEmpty()) {
        setCustomIcon(m_settings.sCustomIcon);
    }
    setBalloonTimeout(m_settings.iBalloonTimeout);
    setSticky(m_settings.opt[Sticky]);
    setSkipPager(m_settings.opt[SkipPager]);
    setSkipTaskbar(m_settings.opt[SkipTaskbar]);
    setIconifyMinimized(m_settings.opt[IconifyMinimized]);
    setIconifyObscured(m_settings.opt[IconifyObscured]);
    setIconifyFocusLost(m_settings.opt[IconifyFocusLost]);
    setLockToDesktop(m_settings.opt[LockToDesktop]);

    connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayActivated(QSystemTrayIcon::ActivationReason)));
}

TrayItem::~TrayItem() {
    // No further interest in events from undocked window.
    XLibUtil::unSubscribe(QX11Info::display(), m_window);
    // Only the main menu needs to be deleted. The rest of the menus and actions
    // are children of this menu and Qt will delete all children.
    delete m_contextMenu;
}

bool TrayItem::readSetting(int8_t argSetting, QString key, bool kdockerDefault) {
    /* Precedence:
     * 1) Command line overrides         (argSetting, if positive)
     * 2) User app-specific defaults     (QSettings: "<m_dockedAppName>/<key>")
     * 3) User global defaults           (QSettings: "_GLOBAL_DEFAULTS/<key>")
     * 4) KDocker defaults               (#define DEFAULT_keyname)
     */
    if (argSetting != NOARG) {
        return (argSetting != 0);
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

QString TrayItem::readSetting(QString argSetting, QString key, QString kdockerDefault) {
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
    return bto ? bto : DEFAULT_BalloonTimeout;
}

TrayItemConfig TrayItem::readConfigGlobals() {
    TrayItemConfig config;

    m_config.beginGroup("_GLOBAL_DEFAULTS");
      config.sCustomIcon           = m_config.value("CustomIcon",       DEFAULT_CustomIcon).toString();
      config.iBalloonTimeout       = m_config.value("BalloonTimeout",   DEFAULT_BalloonTimeout).toInt();
      config.opt[Sticky]           = m_config.value("Sticky",           DEFAULT_Sticky).toBool();
      config.opt[SkipPager]        = m_config.value("SkipPager",        DEFAULT_SkipPager).toBool();
      config.opt[SkipTaskbar]      = m_config.value("SkipTaskbar",      DEFAULT_SkipTaskbar).toBool();
      config.opt[IconifyMinimized] = m_config.value("IconifyMinimized", DEFAULT_IconifyMinimized).toBool();
      config.opt[IconifyObscured]  = m_config.value("IconifyObscured",  DEFAULT_IconifyObscured).toBool();
      config.opt[IconifyFocusLost] = m_config.value("IconifyFocusLost", DEFAULT_IconifyFocusLost).toBool();
      config.opt[LockToDesktop]    = m_config.value("LockToDesktop",    DEFAULT_LockToDesktop).toBool();
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
    TrayItemConfig globals = readConfigGlobals();

    m_config.beginGroup(m_dockedAppName);
      QVariant keyval;

      if (!m_settings.sCustomIcon.isEmpty()) {
          m_config.setValue("CustomIcon", m_settings.sCustomIcon);
      }
      saveSettings();

      // Remove app-specific settings if they match their default values

      keyval = m_config.value("BalloonTimeout");
      if (keyval.isValid() && (keyval.toInt()  == globals.iBalloonTimeout)) {
          m_config.remove("BalloonTimeout");
      }
      keyval = m_config.value("Sticky");
      if (keyval.isValid() && keyval.toBool() == globals.opt[Sticky]) {
          m_config.remove("Sticky");
      }
      keyval = m_config.value("SkipPager");
      if (keyval.isValid() && keyval.toBool() == globals.opt[SkipPager]) {
          m_config.remove("SkipPager");
      }
      keyval = m_config.value("SkipTaskbar");
      if (keyval.isValid() && keyval.toBool() == globals.opt[SkipTaskbar]) {
          m_config.remove("SkipTaskbar");
      }
      keyval = m_config.value("IconifyMinimized");
      if (keyval.isValid() && keyval.toBool() == globals.opt[IconifyMinimized]) {
          m_config.remove("IconifyMinimized");
      }
      keyval = m_config.value("IconifyObscured");
      if (keyval.isValid() && keyval.toBool() == globals.opt[IconifyObscured]) {
          m_config.remove("IconifyObscured");
      }
      keyval = m_config.value("IconifyFocusLost");
      if (keyval.isValid() && keyval.toBool() == globals.opt[IconifyFocusLost]) {
          m_config.remove("IconifyFocusLost");
      }
      keyval = m_config.value("LockToDesktop");
      if (keyval.isValid() && keyval.toBool() == globals.opt[LockToDesktop]) {
          m_config.remove("LockToDesktop");
      }
    m_config.endGroup();
}

void TrayItem::saveSettings() {    /*  "/home/<user>/.config/com.kdocker/KDocker.conf"    //  <==  m_config.fileName();  */
    // Group is set by caller
    m_config.setValue("BalloonTimeout",   m_settings.iBalloonTimeout);
    m_config.setValue("Sticky",           m_settings.opt[Sticky]);
    m_config.setValue("SkipPager",        m_settings.opt[SkipPager]);
    m_config.setValue("SkipTaskbar",      m_settings.opt[SkipTaskbar]);
    m_config.setValue("IconifyMinimized", m_settings.opt[IconifyMinimized]);
    m_config.setValue("IconifyObscured",  m_settings.opt[IconifyObscured]);
    m_config.setValue("IconifyFocusLost", m_settings.opt[IconifyFocusLost]);
    m_config.setValue("LockToDesktop",    m_settings.opt[LockToDesktop]);
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

    if (m_settings.opt[IconifyMinimized]) {
        iconifyWindow();
    } else {
        if (m_settings.opt[SkipTaskbar]) {
            doSkipTaskbar();
        }
    }
}

void TrayItem::restoreWindow() {
    if (isBadWindow()) {
        return;
    }

    m_is_restoring = true;

    Display *display = QX11Info::display();
    Window root = QX11Info::appRootWindow();

    if (m_iconified) {
        m_iconified = false;
        XMapWindow(display, m_window);
        m_sizeHint.flags = USPosition;
        XSetWMNormalHints(display, m_window, &m_sizeHint);

        updateToggleAction();
    }
    XMapRaised(display, m_window);
    XFlush(display);

    // Change to the desktop that the window was last on.
    long l_currDesk[2] = {m_desktop, CurrentTime};
    XLibUtil::sendMessage(display, root, root, "_NET_CURRENT_DESKTOP", 32, SubstructureNotifyMask | SubstructureRedirectMask, l_currDesk, sizeof (l_currDesk));

    if (m_settings.opt[LockToDesktop]) {
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
    QTime t;
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
    Display *display = QX11Info::display();
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

    Display *display = QX11Info::display();
    long l[5] = {0, 0, 0, 0, 0};
    restoreWindow();
    XLibUtil::sendMessage(display, QX11Info::appRootWindow(), m_window, "_NET_CLOSE_WINDOW", 32, SubstructureNotifyMask | SubstructureRedirectMask, l, sizeof (l));
}

void TrayItem::doSkipTaskbar() {
    set_NET_WM_STATE("_NET_WM_STATE_SKIP_TASKBAR", m_settings.opt[SkipTaskbar]);
}

void TrayItem::doSkipPager() {
    set_NET_WM_STATE("_NET_WM_STATE_SKIP_PAGER", m_settings.opt[SkipPager]);
}

void TrayItem::doSticky() {
    set_NET_WM_STATE("_NET_WM_STATE_STICKY", m_settings.opt[Sticky]);
}

void TrayItem::setCustomIcon(QString path) {
    m_customIcon = true;
    QPixmap customIcon;
    if (customIcon.load(path)) {
        m_settings.sCustomIcon = path;
    } else {
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
    m_settings.opt[SkipTaskbar] = value;
    m_actionSkipTaskbar->setChecked(value);
    doSkipTaskbar();
}

void TrayItem::setSkipPager(bool value) {
    m_settings.opt[SkipPager] = value;
    m_actionSkipPager->setChecked(value);
    doSkipPager();
}

void TrayItem::setSticky(bool value) {
    m_settings.opt[Sticky] = value;
    m_actionSticky->setChecked(value);
    doSticky();
}

void TrayItem::setIconifyMinimized(bool value) {
    m_settings.opt[IconifyMinimized] = value;
    m_actionIconifyMinimized->setChecked(value);
}

void TrayItem::setIconifyObscured(bool value) {
    m_settings.opt[IconifyObscured] = value;
    m_actionIconifyObscured->setChecked(value);
}

void TrayItem::setIconifyFocusLost(bool value) {
    m_settings.opt[IconifyFocusLost] = value;
    m_actionIconifyFocusLost->setChecked(value);
    focusLostEvent();
}

void TrayItem::setLockToDesktop(bool value) {
    m_settings.opt[LockToDesktop] = value;
    m_actionLockToDesktop->setChecked(value);
}

void TrayItem::setBalloonTimeout(int value) {
    if (value < 0) {
        value = 0;
    }
    m_settings.iBalloonTimeout = value;
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
    if (m_iconified || m_window != XLibUtil::activeWindow(QX11Info::display())) {
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
    if (m_settings.opt[IconifyMinimized]) {
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

    Display *display = QX11Info::display();
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
    if (m_settings.opt[IconifyObscured]) {
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

    if (m_settings.opt[IconifyFocusLost] && m_window != XLibUtil::activeWindow(QX11Info::display())) {
        iconifyWindow();
    }
}

void TrayItem::set_NET_WM_STATE(const char *type, bool set) {    
    if (isBadWindow()) {
        return;
    }

    // set, true = add the state to the window. False, remove the state from
    // the window.
    Display *display = QX11Info::display();
    Atom atom = XInternAtom(display, type, False);

    qint64 l[2] = {set ? 1 : 0, static_cast<qint64>(atom)};
    XLibUtil::sendMessage(display, QX11Info::appRootWindow(), m_window, "_NET_WM_STATE", 32, SubstructureNotifyMask, l, sizeof (l));
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
    if (m_settings.iBalloonTimeout > 0) {
        showMessage(m_dockedAppName, title, QSystemTrayIcon::Information, m_settings.iBalloonTimeout);
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

    m_actionToggle = new QAction(tr("Toggle"), m_contextMenu);
    connect(m_actionToggle, SIGNAL(triggered()), this, SLOT(toggleWindow()));
    m_contextMenu->addAction(m_actionToggle);
    m_contextMenu->addAction(tr("Undock"), this, SLOT(doUndock()));
    m_contextMenu->addSeparator();

    // Options menu
    m_optionsMenu = new QMenu(tr("Options"), m_contextMenu);
    m_optionsMenu-> setIcon(QIcon(":/images/options.png"));

    m_actionSetIcon = new QAction(tr("Set icon..."), m_optionsMenu);
    connect(m_actionSetIcon, SIGNAL(triggered(bool)), this, SLOT(selectCustomIcon(bool)));
    m_optionsMenu->addAction(m_actionSetIcon);

    m_actionSkipTaskbar = new QAction(tr("Skip taskbar"), m_optionsMenu);
    m_actionSkipTaskbar->setCheckable(true);
    m_actionSkipTaskbar->setChecked(m_settings.opt[SkipTaskbar]);
    connect(m_actionSkipTaskbar, SIGNAL(triggered(bool)), this, SLOT(setSkipTaskbar(bool)));
    m_optionsMenu->addAction(m_actionSkipTaskbar);

    m_actionSkipPager = new QAction(tr("Skip pager"), m_optionsMenu);
    m_actionSkipPager->setCheckable(true);
    m_actionSkipPager->setChecked(m_settings.opt[SkipPager]);
    connect(m_actionSkipPager, SIGNAL(triggered(bool)), this, SLOT(setSkipPager(bool)));
    m_optionsMenu->addAction(m_actionSkipPager);

    m_actionSticky = new QAction(tr("Sticky"), m_optionsMenu);
    m_actionSticky->setCheckable(true);
    m_actionSticky->setChecked(m_settings.opt[Sticky]);
    connect(m_actionSticky, SIGNAL(triggered(bool)), this, SLOT(setSticky(bool)));
    m_optionsMenu->addAction(m_actionSticky);

    m_actionIconifyMinimized = new QAction(tr("Iconify when minimized"), m_optionsMenu);
    m_actionIconifyMinimized->setCheckable(true);
    m_actionIconifyMinimized->setChecked(m_settings.opt[IconifyMinimized]);
    connect(m_actionIconifyMinimized, SIGNAL(triggered(bool)), this, SLOT(setIconifyMinimized(bool)));
    m_optionsMenu->addAction(m_actionIconifyMinimized);

    m_actionIconifyObscured = new QAction(tr("Iconify when obscured"), m_optionsMenu);
    m_actionIconifyObscured->setCheckable(true);
    m_actionIconifyObscured->setChecked(m_settings.opt[IconifyObscured]);
    connect(m_actionIconifyObscured, SIGNAL(triggered(bool)), this, SLOT(setIconifyObscured(bool)));
    m_optionsMenu->addAction(m_actionIconifyObscured);

    m_actionIconifyFocusLost = new QAction(tr("Iconify when focus lost"), m_optionsMenu);
    m_actionIconifyFocusLost->setCheckable(true);
    m_actionIconifyFocusLost->setChecked(m_settings.opt[IconifyFocusLost]);
    connect(m_actionIconifyFocusLost, SIGNAL(toggled(bool)), this, SLOT(setIconifyFocusLost(bool)));
    m_optionsMenu->addAction(m_actionIconifyFocusLost);

    m_actionLockToDesktop = new QAction(tr("Lock to desktop"), m_optionsMenu);
    m_actionLockToDesktop->setCheckable(true);
    m_actionLockToDesktop->setChecked(m_settings.opt[LockToDesktop]);
    connect(m_actionLockToDesktop, SIGNAL(toggled(bool)), this, SLOT(setLockToDesktop(bool)));
    m_optionsMenu->addAction(m_actionLockToDesktop);

    m_actionBalloonTitleChanges = new QAction(tr("Balloon title changes"), m_optionsMenu);
    m_actionBalloonTitleChanges->setCheckable(true);
    m_actionBalloonTitleChanges->setChecked(m_settings.iBalloonTimeout ? true : false);
    connect(m_actionBalloonTitleChanges, SIGNAL(triggered(bool)), this, SLOT(setBalloonTimeout(bool)));
    m_optionsMenu->addAction(m_actionBalloonTitleChanges);

    m_contextMenu->addMenu(m_optionsMenu);

    // Save settings menu
    m_optionsMenu->addSeparator();
    m_defaultsMenu = new QMenu(tr("Save settings"), m_optionsMenu);
    m_defaultsMenu-> setIcon(QIcon(":/images/config.png"));

    m_actionSaveSettingsApp = new QAction(tr("%1 only").arg(m_dockedAppName), m_defaultsMenu);
    connect(m_actionSaveSettingsApp, SIGNAL(triggered()), this, SLOT(saveSettingsApp()));
    m_defaultsMenu->addAction(m_actionSaveSettingsApp);

    m_actionSaveSettingsGlobal = new QAction(tr("Global (all new)"), m_defaultsMenu);
    connect(m_actionSaveSettingsGlobal, SIGNAL(triggered()), this, SLOT(saveSettingsGlobal()));
    m_defaultsMenu->addAction(m_actionSaveSettingsGlobal);

    m_optionsMenu->addMenu(m_defaultsMenu);
    // ---

    m_contextMenu->addSeparator();

    m_contextMenu->addAction(QIcon(":/images/another.png"), tr("Dock Another"), this, SIGNAL(selectAnother()));
    m_contextMenu->addAction(tr("Undock All"), this, SIGNAL(undockAll()));
    m_contextMenu->addSeparator();

    m_contextMenu->addAction(QIcon(":/images/about.png"), tr("About %1").arg(qApp->applicationName()), this, SIGNAL(about()));
    m_contextMenu->addSeparator();

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
        appIcon = QPixmap(const_cast<const char **> (window_icon));
    }
    if (window_icon) {
        XpmFree(window_icon);
    }
    return QIcon(appIcon);
}

bool TrayItem::isBadWindow() {
    Display *display = QX11Info::display();

    if (!XLibUtil::isValidWindowId(display, m_window)) {
        destroyEvent();
        return true;
    }
    return false;
}

// Checks to see if the virtual desktop the window is on is currently 
// displayed. Returns true if it is, otherwise false
bool TrayItem::isOnCurrentDesktop() {
    Display *display = QX11Info::display();
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
