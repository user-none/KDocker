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

#include <QtCore/QAbstractNativeEventFilter>
#include <QList>
#include <QObject>
#include <QStringList>

#include <sys/types.h>

#include "trayitem.h"
#include "scanner.h"
#include "xcbeventreceiver.h"


class Scanner;

class TrayItemManager : public QObject, public QAbstractNativeEventFilter {
    Q_OBJECT

    // The Scanner needs to know which windows are docked.
    friend class Scanner;

public:
    TrayItemManager();
    ~TrayItemManager();
    virtual bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
    void processCommand(const QStringList &args);

public slots:
    void dockWindow(Window window, const TrayItemArgs settings);
    Window userSelectWindow(bool checkNormality = true);
    void remove(TrayItem *trayItem);
    void undock(TrayItem *trayItem);
    void undockAll();
    void about();

private slots:
    void selectAndIconify();
    void checkCount();

signals:
    void quitMouseGrab();

private:
    QList<Window> dockedWindows();
    bool isWindowDocked(Window window);

    Scanner *m_scanner;
    TrayItemArgs m_initArgs;  // 'const' initializer (unset values)
    QList<TrayItem*> m_trayItems;
    GrabInfo m_grabInfo;
};

#endif	/* _TRAYITEMMANAGER_H */
