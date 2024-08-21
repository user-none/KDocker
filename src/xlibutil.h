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

#include "grabinfo.h"

#include <QList>
#include <QObject>
#include <QPixmap>
#include <QRegularExpression>
#include <QString>

#include <xlibtypes.h>

// Hiding XSizeHints. X11 uses a typdef'ed anonymous struct
// so we can't forward declare the type. Instead we'll define
// it as void and use pointers. Not ideal but it will work.
typedef void XLibUtilSizeHints;

// XLibUtil is a helper class that wraps all X11 functions and
// isolates them from the reset of the application. The xcb header
// for the event filter is an exception.
//
// This class keeps all of the X11 functions in one place. All functions
// exposed are much simpler to use by reducing a lot of the verboseness
// of the X11 functions.
//
// The biggest reason for this class existing and not using X11 functions
// directly is major issues including X11 headers causes. X11 headers use
// numerous defines that aren't name spaced and are very generic. If header
// order isn't very specific when including X11 headers, we'll get compiler
// errors due to conflicts with name spaced types within Qt. Since X11 is using
// defines it's jsut a simple text replacement and it completely breaks
// multiple types. Such as, 'None', and 'Unsorted'. We've also had issues with
// 'Bool' if the X11 headers aren't included in the right order or if some
// aren't explicit included.
//
// If any X11 functions need to be added, they should be added here and the
// X11 headers included in the cpp file in order to avoid the above issues.

class XLibUtil : public QObject
{
    Q_OBJECT

public:
    // Qt registered the X error handler and writes the errors to the console.
    // This will prevent x errors from being written to the console.
    //
    // The isValidWindowId function in will generate errors if the
    // window is not valid while it is checking. Which is the point
    // of the function.
    static void silenceXErrors();

    // Helper function to allocate the size hints type.
    static XLibUtilSizeHints *newSizeHints();
    // Helper function to deallocate the size hints type.
    static void deleteSizeHints(XLibUtilSizeHints *sh);
    static void getWMSizeHints(Window w, XLibUtilSizeHints *sh);
    static void setWMSizeHints(Window w, XLibUtilSizeHints *sh);

    static bool isNormalWindow(Window w);
    static bool isValidWindowId(Window w);

    static Window pidToWid(bool checkNormality, pid_t epid, QList<Window> dockedWindows = QList<Window>());
    static Window findWindow(bool checkNormality, const QRegularExpression &ename,
                             QList<Window> dockedWindows = QList<Window>());

    // Get the currently focused window.
    static Window getActiveWindow();
    // Requests user to select a window by grabbing the mouse.
    // A left click will select the application.
    // Clicking any other mouse button or the Escape key will abort the selection.
    static Window selectWindow(GrabInfo &grabInfo, QString &error);

    // Have window events we care about sent to the X11 Event loop.
    // We're part of the event loop so we'll get the events.
    static void subscribe(Window w);
    // Stop receiving events from window.
    static void unSubscribe(Window w);

    // Get the desktop the window is on.
    static long getWindowDesktop(Window w);
    // Get the desktop the user is currently viewing.
    static long getCurrentDesktop();

    static void iconifyWindow(Window w);
    // Is the window in an iconified state.
    static bool isWindowIconic(Window w);
    // Shows the window regardless if it's minimized or iconified.
    static void raiseWindow(Window w);

    static QPixmap getWindowIcon(Window window);
    static QString getAppName(Window w);
    static QString getWindowTitle(Window w);

    static Atom getAtom(const char *name);

    static void setWindowSkipTaskbar(Window w, bool set);
    static void setWindowSkipPager(Window w, bool set);
    static void setWindowSticky(Window w, bool set);

    // Switch the desktop the user is currently viewing to another one..
    static void setCurrentDesktop(long desktop);
    // Set the desktop a given window should appear on.
    static void setWindowDesktop(long desktop, Window w);
    // Give a window focus.
    static void setActiveWindow(Window w);
    static void closeWindow(Window w);
};

#endif // _XLIBUTIL_H
