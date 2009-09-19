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

#ifndef _SCANNER_H
#define	_SCANNER_H

#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>

#include "trayitem.h"

#include <sys/types.h>
#include <X11/Xlib.h>

struct ProcessId {
    QString command;
    pid_t pid;
    TrayItemSettings settings;
    int count;
    int maxCount;
    bool checkNormality;
    bool windowNameMatch;
    QString windowName;
};

// Launches commands and looks for the window ids they create.

class Scanner : public QObject {
    Q_OBJECT

public:
    Scanner();
    ~Scanner();
    void enqueue(const QString &command, const QStringList &arguments, TrayItemSettings settings, int maxTime = 30, bool checkNormality = true, bool windowNameMatch = false, const QString &windowName = QString());
    bool isRunning();
private slots:
    void check();
signals:
    void windowFound(Window, TrayItemSettings);
    void stopping();
private:
    QTimer *m_timer;
    QList<ProcessId> m_processes;
};

#endif	/* _SCANNER_H */
