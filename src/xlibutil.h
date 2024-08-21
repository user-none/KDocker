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

#ifndef _XLIBUTIL_H
#define _XLIBUTIL_H

#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QTimer>
#include <QEventLoop>

#include <xlibtypes.h>

typedef struct GrabInfo {

    QTimer     *qtimer;
    QEventLoop *qloop;

    Window       window;
    unsigned int button;
    bool         isGrabbing;

} GrabInfo;


class XLibUtil : public QObject {
    Q_OBJECT

public:
    static void silenceXErrors();

    static bool isNormalWindow(Window w);
    static bool isValidWindowId(Window w);
    static Window pidToWid(bool checkNormality, pid_t epid, QList<Window> dockedWindows = QList<Window>());
    static Window findWindow(bool checkNormality, const QRegularExpression &ename, QList<Window> dockedWindows = QList<Window>());
    static Window activeWindow();
    static Window selectWindow(GrabInfo &grabInfo, QString &error);
    static void subscribe(Window w, long mask);
    static void unSubscribe(Window w);
    static bool getCardinalProperty(Display *display, Window w, Atom prop, long *data);
    static Display *display();
    static Window appRootWindow();

    static void iconifyWindow(Window w);

    static void mapWindow(Window w);
    static void mapRaised(Window w);
    static void flush();

    static void sendMessageWMState(Window w, const char *type, bool set);
    static void sendMessageCurrentDesktop(long desktop, Window w);
    static void sendMessageWMDesktop(long desktop, Window w);
    static void sendMessageActiveWindow(Window w);
    static void sendMessageCloseWindow(Window w);
};

#endif /* _XLIBUTIL_H */
