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

// $Id: util.cpp,v 1.7 2005/01/29 09:56:08 cs19713 Exp $

#include "util.h"

#include <X11/Xutil.h>
#include <Xmu/WinUtil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#include <stdio.h>

/*
 * Assert validity of the window id. Get window attributes for the heck of it
 * and see if the request went through.
 */
bool isValidWindowId(Display *display, Window w) {
    XWindowAttributes attrib;
    return (XGetWindowAttributes(display, w, &attrib) != 0);
}

/*
 * Checks if this window is a normal window (i.e) 
 * - Has a WM_STATE
 * - Not modal window
 * - Not a purely transient window (with no window type set)
 * - Not a special window (desktop/menu/util) as indicated in the window type
 */
bool isNormalWindow(Display *display, Window w) {
    Atom __attribute__((unused)) type;
    int __attribute__((unused)) format;
    unsigned long __attribute__((unused)) left;
    Atom *data = NULL;
    unsigned long nitems;
    Window transient_for = None;

    static Atom wmState = XInternAtom(display, "WM_STATE", False);
    static Atom windowState = XInternAtom(display, "_NET_WM_STATE", False);
    static Atom modalWindow =
            XInternAtom(display, "_NET_WM_STATE_MODAL", False);

    static Atom windowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    static Atom normalWindow =
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);
    static Atom dialogWindow =
            XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    int ret = XGetWindowProperty(display, w, wmState, 0,
            10, False, AnyPropertyType, &type, &format,
            &nitems, &left, (unsigned char **) & data);

    if (ret != Success || data == NULL) return false;
    if (data) XFree(data);

    ret = XGetWindowProperty(display, w, windowState, 0,
            10, False, AnyPropertyType, &type, &format,
            &nitems, &left, (unsigned char **) & data);
    if (ret == Success) {
        unsigned int i;
        for (i = 0; i < nitems; i++) {
            if (data[i] == modalWindow) break;
        }
        XFree(data);
        if (i < nitems) {
            return false;
        }
    }

    XGetTransientForHint(display, w, &transient_for);

    ret = XGetWindowProperty(display, w, windowType, 0,
            10, False, AnyPropertyType, &type, &format,
            &nitems, &left, (unsigned char **) & data);

    if ((ret == Success) && data) {
        unsigned int i;
        for (i = 0; i < nitems; i++) {
            if (data[i] != normalWindow && data[i] != dialogWindow) break;
        }
        XFree(data);
        return (i == nitems);
    } else
        return (transient_for == None);
}

/*
 * Returns the contents of the _NET_WM_PID (which is supposed to contain the
 * process id of the application that created the window)
 */
pid_t pid(Display *display, Window w) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, leftover;
    unsigned char *pid;
    pid_t pid_return = -1;

    if (XGetWindowProperty(display, w,
            XInternAtom(display, "_NET_WM_PID", False), 0,
            1, False, XA_CARDINAL, &actual_type,
            &actual_format, &nitems, &leftover, &pid) == Success) {
        if (pid) pid_return = *(pid_t *) pid;
        XFree(pid);
    }
    return pid_return;
}

Window pidToWid(Display *display, pid_t epid, Window window) {
    Window w = None;

    Window root;
    Window parent;
    Window *child;
    unsigned int num_child;
    if (XQueryTree(display, window, &root, &parent, &child, &num_child) != 0) {
        for (unsigned int i = 0; i < num_child; i++) {
            if (epid == pid(display, child[i]) && isNormalWindow(display, child[i])) {
                return child[i];
            }
            else {
                w = pidToWid(display, epid, child[i]);
            }
            if (w != None) {
                return w;
            }
        }
    }

    return w;
}

/*
 * Sends ClientMessage to a window
 */
void sendMessage(Display* display, Window to, Window w, const char *type,
        int format, long mask, void* data, int size) {
    XEvent ev;
    memset(&ev, 0, sizeof (ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = XInternAtom(display, type, True);
    ev.xclient.format = format;
    memcpy((char *) & ev.xclient.data, (const char *) data, size);
    XSendEvent(display, to, False, mask, &ev);
    XSync(display, False);
}

Window activeWindow(Display *display) {
    Atom active_window_atom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
    Atom type = None;
    int format;
    unsigned long nitems, after;
    unsigned char *data = NULL;
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
    int r = XGetWindowProperty(display, root, active_window_atom, 0, 1,
            False, AnyPropertyType, &type, &format, &nitems, &after, &data);

    Window w = None;
    if ((r == Success) && data && (*(Window *) data != None))
        w = *(Window *) data;
    else {
        int revert;
        XGetInputFocus(display, &w, &revert);
    }
    return w;
}

/*
 * Requests user to select a window by grabbing the mouse. A left click will
 * select the application. Clicking any other button will abort the selection
 */
Window selectWindow(Display *display, const char **err) {
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    if (err) *err = NULL;

    Cursor cursor = XCreateFontCursor(display, XC_draped_box);
    if (cursor == None) {
        if (err) *err = "Failed to create XC_draped_box";
        return None;
    }

    if (XGrabPointer(display, root, False, ButtonPressMask | ButtonReleaseMask,
            GrabModeSync, GrabModeAsync, None, cursor, CurrentTime)
            != GrabSuccess) {
        if (err) *err = "Failed to grab mouse";
        return None;
    }

    XAllowEvents(display, SyncPointer, CurrentTime);
    XEvent event;
    XWindowEvent(display, root, ButtonPressMask, &event);
    Window selected_window =
            (event.xbutton.subwindow == None) ? RootWindow(display, screen)
            : event.xbutton.subwindow;
    XUngrabPointer(display, CurrentTime);
    XFreeCursor(display, cursor);

    if (event.xbutton.button != Button1) return None;
    return XmuClientWindow(display, selected_window);
}

void subscribe(Display *display, Window w, long mask, bool set) {
    Window root = RootWindow(display, DefaultScreen(display));
    XWindowAttributes attr;

    XGetWindowAttributes(display, w == None ? root : w, &attr);

    if (set && ((attr.your_event_mask & mask) == mask)) return;
    if (!set && ((attr.your_event_mask | mask) == attr.your_event_mask)) return;

    XSelectInput(display, w == None ? root : w,
            set ? attr.your_event_mask | mask : attr.your_event_mask & mask);
    XSync(display, False);
}

bool getCardinalProperty(Display *display, Window w, Atom prop, long *data) {
    Atom type;
    int format;
    unsigned long nitems, bytes;
    unsigned char *d = NULL;

    if (XGetWindowProperty(display, w, prop, 0, 1, False, XA_CARDINAL, &type,
            &format, &nitems, &bytes, &d) == Success && d) {
        if (data) *data = *(long *) d;
        XFree(d);
        return true;
    }
    return false;
}

