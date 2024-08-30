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

#ifndef _TRAYITEMMANAGER_H
#define _TRAYITEMMANAGER_H

#include "adaptor.h"
#include "command.h"
#include "grabinfo.h"
#include "trayitem.h"
#include "xlibtypes.h"

#include <QHash>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QtCore/QAbstractNativeEventFilter>

class Scanner;

class TrayItemManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT

    // The Scanner needs to know which windows are docked.
    friend class Scanner;

public:
    TrayItemManager();
    ~TrayItemManager();
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

public slots:
    // Defaults are needed for overloading from DBus.
    // There are simplified versions of each of these exposed as well as
    // the full ones. The defaults allow us to have one function for each overload.
    void dockWindowTitle(const QString &searchPattern, uint timeout = 4, bool checkNormality = true,
                         const TrayItemOptions &options = TrayItemOptions());
    void dockLaunchApp(const QString &app, const QStringList &appArguments, const QString &searchPattern,
                       uint timeout = 4, bool checkNormality = true,
                       const TrayItemOptions &options = TrayItemOptions());
    bool dockWindowId(uint windowId, const TrayItemOptions &options = TrayItemOptions());
    bool dockPid(int pid, bool checkNormality = true, const TrayItemOptions &options = TrayItemOptions());
    void dockSelectWindow(bool checkNormality = true, const TrayItemOptions &options = TrayItemOptions());
    void dockFocused(const TrayItemOptions &options = TrayItemOptions());

    WindowNameMap listWindows();
    bool closeWindow(uint windowId);
    bool hideWindow(uint windowId);
    bool showWindow(uint windowId);
    bool undockWindow(uint windowId);
    void undockAll();

    void quit();
    void keepRunning();

private slots:
    void dockWindow(windowid_t window, const TrayItemOptions &settings);
    windowid_t userSelectWindow(bool checkNormality = true);
    void remove(TrayItem *trayItem);
    void undockRestore(TrayItem *trayItem);
    void selectAndIconify();
    void about();

    void checkCount();

signals:
    void quitMouseGrab();

private:
    QList<windowid_t> dockedWindows();
    bool isWindowDocked(windowid_t window);

    Scanner *m_scanner;
    TrayItemOptions m_initArgs;
    QList<TrayItem *> m_trayItems;
    GrabInfo m_grabInfo;
    bool m_keepRunning;
};

#endif // _TRAYITEMMANAGER_H
