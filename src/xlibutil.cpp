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

#include "xlibutil.h"

#include <QGuiApplication>

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xutil.h>
#include <X11/Xmu/WinUtil.h>

#if 0
#undef None
#undef Unsorted
#undef Bool
#endif

#define BIT0  (1 << 0)
#define BIT1  (1 << 1)
#define BIT2  (1 << 2)
#define BIT3  (1 << 3)

int ignoreXErrors(Display *, XErrorEvent *) {
    return 0;
}

void XLibUtil::silenceXErrors() {
    // Qt regeistereds the X error handler and writes the errors to
    // the console. We can cause errors to be logged that can be safely ignored.
    //
    // This will prevent x errors from being written to the console.
    // The isValidWindowId function in will generate errors if the
    // window is not valid while it is checking.
    XSetErrorHandler(ignoreXErrors);
}

/*
 * Assert validity of the window id. Get window attributes for the heck of it
 * and see if the request went through.
 */
bool XLibUtil::isValidWindowId(Window w) {
    XWindowAttributes attrib;
    return (XGetWindowAttributes(XLibUtil::display(), w, &attrib) != 0);
}

/*
 * Checks if this window is a normal window (i.e) 
 * - Has a WM_STATE
 * - Not modal window
 * - Not a purely transient window (with no window type set)
 * - Not a special window (desktop/menu/util) as indicated in the window type
 */
bool XLibUtil::isNormalWindow(Window w) {
    Atom type;
    int format;
    unsigned long left;
    Atom *data = NULL;
    unsigned long nitems;
    Window transient_for = 0;
    Display *display = XLibUtil::display();

    static Atom wmState      = XInternAtom(display, "WM_STATE", false);
    static Atom windowState  = XInternAtom(display, "_NET_WM_STATE", false);
    static Atom modalWindow  = XInternAtom(display, "_NET_WM_STATE_MODAL", false);
    static Atom windowType   = XInternAtom(display, "_NET_WM_WINDOW_TYPE", false);
    static Atom normalWindow = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", false);
    static Atom dialogWindow = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", false);

    int ret = XGetWindowProperty(display, w, wmState, 0, 10, false, AnyPropertyType, &type, &format, &nitems, &left, (unsigned char **) & data);

    if (ret != Success || data == NULL) {
        if (data != NULL)
            XFree(data);
        return false;
    }
    if (data) {
        XFree(data);
    }

    ret = XGetWindowProperty(display, w, windowState, 0, 10, false, AnyPropertyType, &type, &format, &nitems, &left, (unsigned char **) & data);
    if (ret == Success) {
        unsigned int i;
        for (i = 0; i < nitems; i++) {
            if (data[i] == modalWindow) {
                break;
            }
        }
        XFree(data);
        if (i < nitems) {
            return false;
        }
    }

    XGetTransientForHint(display, w, &transient_for);

    ret = XGetWindowProperty(display, w, windowType, 0, 10, false, AnyPropertyType, &type, &format, &nitems, &left, (unsigned char **) & data);

    if ((ret == Success) && data) {
        unsigned int i;
        for (i = 0; i < nitems; i++) {
            if (data[i] != normalWindow && data[i] != dialogWindow) {
                break;
            }
        }
        XFree(data);
        return (i == nitems);
    } else {
        return (transient_for == 0);
    }
}

/*
 * Returns the contents of the _NET_WM_PID (which is supposed to contain the
 * process id of the application that created the window)
 */
static pid_t pid(Display *display, Window w) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, leftover;
    unsigned char *pid;
    pid_t pid_return = -1;

    if (XGetWindowProperty(display, w, XInternAtom(display, "_NET_WM_PID", false), 0, 1, false, XA_CARDINAL, &actual_type, &actual_format, &nitems, &leftover, &pid) == Success) {
        if (pid) {
            pid_return = *(pid_t *) pid;
        }
        XFree(pid);
    }
    return pid_return;
}

static Window pidToWidEx(Display *display, Window window, bool checkNormality, pid_t epid, QList<Window> dockedWindows = QList<Window>()) {
    Window w = 0;
    Window root;
    Window parent;
    Window *child;
    unsigned int num_child;

    if (XQueryTree(display, window, &root, &parent, &child, &num_child) != 0) {
        for (unsigned int i = 0; i < num_child; i++) {
            if (epid == pid(display, child[i])) {
                if (!dockedWindows.contains(child[i])) {
                    if (checkNormality) {
                        if (XLibUtil::isNormalWindow(child[i])) {
                            return child[i];
                        }
                    } else {
                        return child[i];
                    }
                }
            }
            w = pidToWidEx(display, child[i], checkNormality, epid);
            if (w != 0) {
                break;
            }
        }
    }

    return w;
}

Window XLibUtil::pidToWid(bool checkNormality, pid_t epid, QList<Window> dockedWindows) {
    return pidToWidEx(XLibUtil::display(), XLibUtil::appRootWindow(), checkNormality, epid, dockedWindows);
}

/*
 * The Grand Window Analyzer. Checks if window w has matching name
 */
static bool analyzeWindow(Display *display, Window w, const QRegularExpression &ename) {
    XClassHint ch;

    // no plans to analyze windows without a name
    char *window_name = NULL;
    if (!XFetchName(display, w, &window_name)) {
        return false;
    }
    if (window_name) {
        XFree(window_name);
    } else {
        return false;
    }

    bool this_is_our_man = false;
    // lets try the program name
    if (XGetClassHint(display, w, &ch)) {
        if (QString(ch.res_name).contains(ename)) {
            this_is_our_man = true;
        } else if (QString(ch.res_class).contains(ename)) {
            this_is_our_man = true;
        } else {
            // sheer desperation
            char *wm_name = NULL;
            XFetchName(display, w, &wm_name);
            if (wm_name && QString(wm_name).contains(ename)) {
                this_is_our_man = true;
            }
        }

        if (ch.res_class) {
            XFree(ch.res_class);
        }
        if (ch.res_name) {
            XFree(ch.res_name);
        }
    }

    // it's probably a good idea to check (obsolete) WM_COMMAND here
    return this_is_our_man;
}

/*
 * Given a starting window look though all children and try to find a window
 * that matches the ename.
 */
static Window findWindowEx(Display *display, Window window, bool checkNormality, const QRegularExpression &ename, QList<Window> dockedWindows = QList<Window>()) {
    Window w = 0;
    Window root;
    Window parent;
    Window *child;
    unsigned int num_child;
    if (XQueryTree(display, window, &root, &parent, &child, &num_child) != 0) {
        for (unsigned int i = 0; i < num_child; i++) {
            if (analyzeWindow(display, child[i], ename)) {
                if (!dockedWindows.contains(child[i])) {
                    if (checkNormality) {
                        if (XLibUtil::isNormalWindow(child[i])) {
                            return child[i];
                        }
                    } else {
                        return child[i];
                    }
                }
            }
            w = findWindowEx(display, child[i], checkNormality, ename);
            if (w != 0) {
                break;
            }
        }
    }
    return w;
}

Window XLibUtil::findWindow(bool checkNormality, const QRegularExpression &ename, QList<Window> dockedWindows) {
    return findWindowEx(XLibUtil::display(), XLibUtil::appRootWindow(), checkNormality, ename, dockedWindows);
}

/*
 * Sends ClientMessage to a window.
 */
static void sendMessage(Display* display, Window to, Window w, const char *type, int format, long mask, void* data, int size) {
    XEvent ev;
    memset(&ev, 0, sizeof (ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = XInternAtom(display, type, true);
    ev.xclient.format = format;
    memcpy((char *) & ev.xclient.data, (const char *) data, size);
    XSendEvent(display, to, false, mask, &ev);
    XSync(display, false);
}

void XLibUtil::sendMessageWMState(Window w, const char *type, bool set) {
    // set, true = add the state to the window. false, remove the state from
    // the window.
    Display *display = XLibUtil::display();
    Atom atom = XInternAtom(display, type, false);

    qint64 l[2] = {set ? 1 : 0, static_cast<qint64>(atom)};
    sendMessage(display, XLibUtil::appRootWindow(), w, "_NET_WM_STATE", 32, SubstructureNotifyMask, l, sizeof (l));
}

void XLibUtil::sendMessageCurrentDesktop(long desktop, Window w) {
    Window root = XLibUtil::appRootWindow();
    long l_currDesk[2] = {desktop, CurrentTime};
    sendMessage(XLibUtil::display(), root, root, "_NET_CURRENT_DESKTOP", 32, SubstructureNotifyMask | SubstructureRedirectMask, l_currDesk, sizeof (l_currDesk));
}

void XLibUtil::sendMessageWMDesktop(long desktop, Window w) {
    long l_wmDesk[2] = {desktop, 1}; // 1 == request sent from application. 2 == from pager
    sendMessage(XLibUtil::display(), XLibUtil::appRootWindow(), w, "_NET_WM_DESKTOP", 32, SubstructureNotifyMask | SubstructureRedirectMask, l_wmDesk, sizeof (l_wmDesk));
}

void XLibUtil::sendMessageActiveWindow(Window w) {
    // 1 == request sent from application. 2 == from pager.
    // We use 2 because KWin doesn't always give the window focus with 1.
    long l_active[2] = {2, CurrentTime};
    sendMessage(XLibUtil::display(), XLibUtil::appRootWindow(), w, "_NET_ACTIVE_WINDOW", 32, SubstructureNotifyMask | SubstructureRedirectMask, l_active, sizeof (l_active));
    XSetInputFocus(XLibUtil::display(), w, RevertToParent, CurrentTime);
}

void XLibUtil::sendMessageCloseWindow(Window w) {
    long l[5] = {0, 0, 0, 0, 0};
    sendMessage(XLibUtil::display(), XLibUtil::appRootWindow(), w, "_NET_CLOSE_WINDOW", 32, SubstructureNotifyMask | SubstructureRedirectMask, l, sizeof (l));
}

/*
 * Returns the id of the currently active window.
 */
Window XLibUtil::activeWindow() {
    Display *display = XLibUtil::display();
    Atom active_window_atom = XInternAtom(display, "_NET_ACTIVE_WINDOW", true);
    Atom type = 0;
    int format;
    unsigned long nitems, after;
    unsigned char *data = NULL;
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    int r = XGetWindowProperty(display, root, active_window_atom, 0, 1, false, AnyPropertyType, &type, &format, &nitems, &after, &data);

    Window w = 0;
    if ((r == Success) && data && (*reinterpret_cast<Window *> (data) != 0)) {
        w = *(Window *) data;
    } else {
        int revert;
        XGetInputFocus(display, &w, &revert);
    }
    return w;
}

/*
 * Requests user to select a window by grabbing the mouse.
 * A left click will select the application.
 * Clicking any other mouse button or the Escape key will abort the selection.
 */
Window XLibUtil::selectWindow(GrabInfo &grabInfo, QString &error) {
    Display *display = XLibUtil::display();
    int screen  = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    Cursor cursor = XCreateFontCursor(display, XC_draped_box);
    if (cursor == 0) {
        error = tr("Failed to create XC_draped_box");
        return 0;
    }
    if (XGrabPointer(display, root, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, 0, cursor, CurrentTime) != GrabSuccess) {
        error = tr("Failed to grab mouse");
        return 0;
    }
    //
    //  X11 treats Scroll_Lock & Num_Lock as 'modifiers'; each exact combination has to be grabbed
    //   from [ESC + No Modifier] (raw) through to [ESC + CAPS_lock + NUM_lock + SCROLL_lock]
    //  Cannot use 'AnyModifier' here in case, say, CTRL+ESC is in use (results in failure to grab at all)
    //
    KeyCode keyEsc = XKeysymToKeycode(display, XK_Escape);
    for (int b = 0; b < BIT3; b++) {  // 000..111 (grab eight Escape key combinations)
        int modifiers = ((b & BIT0) ? LockMask : 0) |   // CAPS_lock
                        ((b & BIT1) ? Mod2Mask : 0) |   // NUM_lock
                        ((b & BIT2) ? Mod5Mask : 0);    // SCROLL_lock
        XGrabKey(display, keyEsc, modifiers, root, False, GrabModeAsync, GrabModeAsync);
    }
    XSelectInput(display, root, KeyPressMask);
    XAllowEvents(display, SyncPointer, CurrentTime);

    grabInfo.window = 0;
    grabInfo.button = 0;
    grabInfo.qtimer->setSingleShot(true);
    //grabInfo.qtimer-> start(20000);   // 20 second timeout
    grabInfo.qtimer->start(5000);   // 5 second timeout

    XSync(display, false);

    grabInfo.isGrabbing = true;    // Enable XCB_BUTTON_PRESS code in event filter
    grabInfo.qloop->exec();        // block until button pressed or timeout

    XUngrabPointer(display, CurrentTime);
    XUngrabKey(display, keyEsc, AnyModifier, root);
    XSelectInput(display, root, NoEventMask);
    XFreeCursor(display, cursor);

    if (grabInfo.button != Button1 || !grabInfo.window || !grabInfo.qtimer-> isActive()) {
        return 0;
    }

    return XmuClientWindow(display, grabInfo.window);
}

/*
 * Have events associated with mask for the window set in the X11 Event loop
 * to the application.
 */
void XLibUtil::subscribe(Window w, long mask) {
    Display *display = XLibUtil::display();
    Window root = RootWindow(display, DefaultScreen(display));
    XWindowAttributes attr;

    XGetWindowAttributes(display, w == 0 ? root : w, &attr);

    XSelectInput(display, w == 0 ? root : w, attr.your_event_mask | mask);
    XSync(display, false);
}

void XLibUtil::unSubscribe(Window w) {
    Display *display = XLibUtil::display();
    XSelectInput(display, w, NoEventMask);
    XSync(display, false);
}

/*
 * Sets data to the value of the requested window property.
 */
bool XLibUtil::getCardinalProperty(Display *display, Window w, Atom prop, long *data) {
    Atom type;
    int format;
    unsigned long nitems, bytes;
    unsigned char *d = NULL;

    if (XGetWindowProperty(display, w, prop, 0, 1, false, XA_CARDINAL, &type, &format, &nitems, &bytes, &d) == Success && d) {
        if (data) {
            *data = *reinterpret_cast<long *> (d);
        }
        XFree(d);
        return true;
    }
    return false;
}

void XLibUtil::iconifyWindow(Window w) {
    Display *display = XLibUtil::display();
    int screen = DefaultScreen(display);
    /*
     * A simple call to XWithdrawWindow wont do. Here is what we do:
     * 1. Iconify. This will make the application hide all its other windows. For
     *    example, xmms would take off the playlist and equalizer window.
     * 2. Withdraw the window to remove it from the taskbar.
     */
    XIconifyWindow(display, w, screen); // good for effects too
    XSync(display, false);
    XWithdrawWindow(display, w, screen);
}

void XLibUtil::mapWindow(Window w) {
    XMapWindow(XLibUtil::display(), w);
}

void XLibUtil::mapRaised(Window w) {
    XMapRaised(XLibUtil::display(), w);
}

void XLibUtil::flush() {
    XFlush(XLibUtil::display());
}

Display *XLibUtil::display() {
    return qApp->nativeInterface<QNativeInterface::QX11Application>()->display();
}

Window XLibUtil::appRootWindow() {
    return DefaultRootWindow(XLibUtil::display());
}
