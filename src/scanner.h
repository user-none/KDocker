/*
 *  Copyright (C) 2009, 2012, 2015 John Schember <john@nachtimwald.com>
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
#include <QRegularExpression>
#include <QString>
#include <QTimer>

#include <sys/types.h>

#include "trayitemmanager.h"


class TrayItemManager;

struct ProcessId {
    QString command;
    pid_t pid;
    TrayItemArgs settings;
    int count;
    int maxCount;
    bool checkNormality;
    QRegularExpression windowName;
};

// Launches commands and looks for the window ids they create.

class Scanner : public QObject {
    Q_OBJECT

public:
    Scanner(TrayItemManager *manager);
    ~Scanner();
    void enqueue(const QString &command, const QStringList &arguments, TrayItemArgs settings, int maxTime = 30, bool checkNormality = true, const QRegularExpression &windowName = QRegularExpression());
    bool isRunning();

private slots:
    void check();

signals:
    void windowFound(Window, TrayItemArgs);
    void stopping();

private:
    TrayItemManager *m_manager;
    QTimer *m_timer;
    QList<ProcessId> m_processes;
};

#endif	/* _SCANNER_H */
