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
#define	_TRAYITEMMANAGER_H

#include "trayitem.h"
#include "scanner.h"
#include "command.h"

#include <QtCore/QAbstractNativeEventFilter>
#include <QList>
#include <QObject>
#include <QStringList>

#include <sys/types.h>

class Scanner;

class TrayItemManager : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

    // The Scanner needs to know which windows are docked.
    friend class Scanner;

public:
    TrayItemManager();
    ~TrayItemManager();
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

public slots:
    void dockWindowTitle(const QString &searchPattern, uint timeout, bool checkNormality, const TrayItemConfig &config);
    void dockLaunchApp(const QString &app, const QStringList &appArguments, const QString &searchPattern, uint timeout, bool checkNormality, const TrayItemConfig &config);
    void dockWindowId(int wid, const TrayItemConfig &config);
    void dockPid(int pid, bool checkNormality, const TrayItemConfig &config);
    void dockSelectWindow(bool checkNormality, const TrayItemConfig &config);
    void dockFocused(const TrayItemConfig &config);

    void undockAll();
    void quit();
    void daemonize();

private slots:
    void dockWindow(Window window, const TrayItemConfig &settings);
    Window userSelectWindow(bool checkNormality = true);
    void remove(TrayItem *trayItem);
    void undock(TrayItem *trayItem);
    void selectAndIconify();
    void about();

    void checkCount();

signals:
    void quitMouseGrab();

private:
    QList<Window> dockedWindows();
    bool isWindowDocked(Window window);

    Scanner *m_scanner;
    TrayItemConfig m_initArgs;
    QList<TrayItem*> m_trayItems;
    GrabInfo m_grabInfo;
    bool m_daemon;
};

#endif	/* _TRAYITEMMANAGER_H */
