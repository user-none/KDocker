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

#include <sys/types.h>

#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <Xmu/WinUtil.h>


#undef Bool

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
    static bool isNormalWindow(Display *display, Window w);
    static bool isValidWindowId(Display *display, Window w);
    static pid_t pid(Display *display, Window w);
    static Window pidToWid(Display *display, Window window, bool checkNormality, pid_t epid, QList<Window> dockedWindows = QList<Window>());
    static bool analyzeWindow(Display *display, Window w, const QRegularExpression &ename);
    static Window findWindow(Display *display, Window w, bool checkNormality, const QRegularExpression &ename, QList<Window> dockedWindows = QList<Window>());
    static void sendMessage(Display *display, Window to, Window w, const char *type, int format, long mask, void *data, int size);
    static Window activeWindow(Display *display);
    static Window selectWindow(Display *display, GrabInfo &grabInfo, QString &error);
    static void subscribe(Display *display, Window w, long mask);
    static void unSubscribe(Display *display, Window w);
    static bool getCardinalProperty(Display *display, Window w, Atom prop, long *data);
    static Display *display();
    static Window appRootWindow();
};

#endif /* _XLIBUTIL_H */
