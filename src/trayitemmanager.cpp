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

#include <QByteArray>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTextStream>
#include <QX11Info>

#include "constants.h"
#include "trayitemmanager.h"
#include "util.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

int ignoreXErrors(Display *, XErrorEvent *) {
    return 0;
}

TrayItemManager::TrayItemManager() {
    m_scanner = new Scanner(this);
    connect(m_scanner, SIGNAL(windowFound(Window, TrayItemSettings)), this, SLOT(dockWindow(Window, TrayItemSettings)));
    connect(m_scanner, SIGNAL(stopping()), this, SLOT(checkCount()));
    // This will prevent x errors from being written to the console.
    // The isValidWindowId function in util.cpp will generate errors if the
    // window is not valid while it is checking.
    XSetErrorHandler(ignoreXErrors);
}

TrayItemManager::~TrayItemManager() {
    while (!m_trayItems.isEmpty()) {
        TrayItem *t = m_trayItems.takeFirst();
        delete t;
    }
    delete m_scanner;
}

/*
 * The X11 Event Filter. Pass on events to the TrayItems that we created.
 */
bool TrayItemManager::x11EventFilter(XEvent *ev) {
    QListIterator<TrayItem*> ti(m_trayItems);

    // We pass on the event to the tray item with the associated window.
    TrayItem *t;
    while (ti.hasNext()) {
        t = ti.next();
        if (ev->xclient.window == t->containerWindow() || ev->xclient.window == t->embedWindow()) {
            return t->x11EventFilter(ev);
        }
    }

    return false;
}

void TrayItemManager::processCommand(const QStringList &args) {
    int option;
    pid_t pid = 0;
    Window window = 0;
    bool checkNormality = true;
    bool windowNameMatch = false;
    int maxTime = 5;
    QString windowName;
    TrayItemSettings settings;
    settings.balloonTimeout = 4000;
    settings.iconify = true;
    settings.skipTaskbar = false;
    settings.skipPager = false;
    settings.sticky = false;
    settings.iconifyObscure = false;
    settings.iconifyFocusLost = false;
    settings.iconifyOnClose = true;

    // Turn the QStringList of arguments into something getopt can use.
    QList<QByteArray> bargs;

    Q_FOREACH(QString s, args) {
        bargs.append(s.toLocal8Bit());
    }
    int argc = bargs.count();
    // Use a const char * here and a const_cast later because it is faster.
    // Using char * will cause a deep copy.
    const char *argv[argc + 1];
    for (int i = 0; i < argc; i++) {
        argv[i] = bargs[i].data();
    }

    /* Options: a, h, u, v are all handled by the KDocker class because we
     * want them to print on the tty the instance was called from.
     */
    optind = 0; // initialise the getopt static
    while ((option = getopt(argc, const_cast<char **> (argv), OPTIONSTRING)) != -1) {
        switch (option) {
            case '?':
                checkCount();
                return;
            case 'b':
                checkNormality = false;
                break;
            case 'c':
                settings.iconifyOnClose = false;
                break;
            case 'd':
                maxTime = atoi(optarg);
                break;
            case 'f':
                window = activeWindow(QX11Info::display());
                if (!window) {
                    QMessageBox::critical(0, qApp->applicationName(), tr("Cannot dock the active window because no window has focus."));
                    checkCount();
                    return;
                }
                break;
            case 'i':
                settings.customIcon = QString::fromLocal8Bit(optarg);
                break;
            case 'l':
                settings.iconifyFocusLost = true;
                break;
            case 'm':
                settings.iconify = false;
                break;
            case 'n':
                windowName = QString::fromLocal8Bit(optarg);
                break;
            case 'o':
                settings.iconifyObscure = true;
                break;
            case 'p':
                settings.balloonTimeout = atoi(optarg) * 1000; // convert to ms
                break;
            case 'q':
                settings.balloonTimeout = 0; // same as '-p 0'
                break;
            case 'r':
                settings.skipPager = true;
                break;
            case 's':
                settings.sticky = true;
                break;
            case 't':
                settings.skipTaskbar = true;
                break;
            case 'w':
                if ((optarg[1] == 'x') || (optarg[1] == 'X'))
                    sscanf(optarg, "%x", (unsigned *) & window);
                else
                    window = (Window) atoi(optarg);
                if (!isValidWindowId(QX11Info::display(), window)) {
                    QMessageBox::critical(0, qApp->applicationName(), tr("Invalid window id."));
                    checkCount();
                    return;
                }
                break;
            case 'x':
                pid = atol(optarg);
                break;
            case 'y':
                windowNameMatch = true;
                break;
        } // switch (option)
    } // while (getopt)

    if (optind < argc) {
        QString command = argv[optind];
        QStringList arguments;
        for (int i = optind + 1; i < argc; i++) {
            arguments << QString::fromLocal8Bit(argv[i]);
        }
        m_scanner->enqueue(command, arguments, settings, maxTime, checkNormality, windowNameMatch, windowName);
        checkCount();
    } else {
        if (!window) {
            if (pid != 0) {
                window = pidToWid(QX11Info::display(), QX11Info::appRootWindow(), checkNormality, pid, dockedWindows());
            } else {
                window = userSelectWindow(checkNormality);
            }
        }
        if (window) {
            dockWindow(window, settings);
        } else {
            // No window was selected or set.
            checkCount();
        }
    }
}

void TrayItemManager::dockWindow(Window window, TrayItemSettings settings) {
    if (isWindowDocked(window)) {
        QMessageBox::information(0, qApp->applicationName(), tr("This window is already docked.\nClick on system tray icon to toggle docking."));
        checkCount();
        return;
    }

    TrayItem *ti = new TrayItem(window);

    if (!settings.customIcon.isEmpty()) {
        ti->setCustomIcon(settings.customIcon);
    }
    ti->setBalloonTimeout(settings.balloonTimeout);
    ti->setSticky(settings.sticky);
    ti->setSkipPager(settings.skipPager);
    ti->setSkipTaskbar(settings.skipTaskbar);
    ti->setIconifyObscure(settings.iconifyObscure);
    ti->setIconifyFocusLost(settings.iconifyFocusLost);
    ti->setIconifyOnClose(settings.iconifyOnClose);

    connect(ti, SIGNAL(selectAnother()), this, SLOT(selectAndIconify()));
    connect(ti, SIGNAL(dead(TrayItem*)), this, SLOT(remove(TrayItem*)));
    connect(ti, SIGNAL(undock(TrayItem*)), this, SLOT(undock(TrayItem*)));
    connect(ti, SIGNAL(undockAll()), this, SLOT(undockAll()));
    connect(ti, SIGNAL(about()), this, SLOT(about()));

    ti->show();

    if (settings.iconify) {
        ti->iconifyWindow();
    } else {
        if (settings.skipTaskbar) {
            ti->doSkipTaskbar();
        }
    }

    m_trayItems.append(ti);
}

Window TrayItemManager::userSelectWindow(bool checkNormality) {
    QTextStream out(stdout);
    out << tr("Select the application/window to dock with the left mouse button.") << endl;
    out << tr("Click any other mouse button to abort.") << endl;

    const char *error = 0;
    Window window = selectWindow(QX11Info::display(), &error);
    if (!window) {
        if (error) {
            QMessageBox::critical(0, qApp->applicationName(), tr(error));
        }
        checkCount();
        return 0;
    }

    if (checkNormality) {
        if (!isNormalWindow(QX11Info::display(), window)) {
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
    aboutBox.setIconPixmap(QPixmap(":/images/kdocker.png"));
    aboutBox.setWindowTitle(tr("About %1 - %2").arg(qApp->applicationName()).arg(qApp->applicationVersion()));
    aboutBox.setText(ABOUT);
    aboutBox.setInformativeText(tr("See %1 for more information.").arg("<a href=\"https://launchpad.net/kdocker\">https://launchpad.net/kdocker</a>"));
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}

void TrayItemManager::selectAndIconify() {
    Window window = userSelectWindow(true);

    if (window) {
        TrayItemSettings settings;
        settings.balloonTimeout = 4000;
        settings.iconify = true;
        settings.skipTaskbar = false;
        settings.skipPager = false;
        settings.sticky = false;
        settings.iconifyObscure = false;
        settings.iconifyFocusLost = false;
        settings.iconifyOnClose = true;

        dockWindow(window, settings);
    }
}

void TrayItemManager::checkCount() {
    if (m_trayItems.isEmpty() && !m_scanner->isRunning()) {
        qApp->quit();
    }
}

QList<Window> TrayItemManager::dockedWindows() {
    QList<Window> windows;

    QListIterator<TrayItem*> ti(m_trayItems);
    while (ti.hasNext()) {
        windows.append(ti.next()->containerWindow());
    }

    return windows;
}

bool TrayItemManager::isWindowDocked(Window window) {
    return dockedWindows().contains(window);
}
