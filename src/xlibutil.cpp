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
#include <QImage>

#include <stdio.h>
#include <stdlib.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
#define BIT2 (1 << 2)
#define BIT3 (1 << 3)

// A number of functions are declared static and not part of
// the XLibUtil class. They're private but can'tor shouldn't
// be in the public header because it would introduce additional
// X11 types we don't want exposed. These are all internal only
// and all X11 lib functions are static. So this works fine even
// though it's more c than cpp. But X11 is a c library.
//
// Most functions that look up Atom's will store the result in
// a function local static variable. This is an optimization so
// we don't have to keep calling a string lookup function. Atoms
// are unsigned longs and never change their value.

static int ignoreXErrors([[maybe_unused]] Display *, [[maybe_unused]] XErrorEvent *)
{
    return 0;
}

static Display *getDisplay()
{
    return qApp->nativeInterface<QNativeInterface::QX11Application>()->display();
}

static windowid_t getDefaultRootWindow()
{
    return DefaultRootWindow(getDisplay());
}

void XLibUtil::silenceXErrors()
{
    XSetErrorHandler(ignoreXErrors);
}

XLibUtilSizeHints *XLibUtil::newSizeHints()
{
    XSizeHints *x = new XSizeHints();
    return static_cast<XLibUtilSizeHints *>(x);
}

void XLibUtil::deleteSizeHints(XLibUtilSizeHints *sh)
{
    XSizeHints *x = static_cast<XSizeHints *>(sh);
    delete x;
}

void XLibUtil::getWMSizeHints(windowid_t window, XLibUtilSizeHints *sh)
{
    XSizeHints *x = static_cast<XSizeHints *>(sh);
    long dummy;
    XGetWMNormalHints(getDisplay(), window, x, &dummy);
}

void XLibUtil::setWMSizeHints(windowid_t window, XLibUtilSizeHints *sh)
{
    XSizeHints *x = static_cast<XSizeHints *>(sh);

    XMapWindow(getDisplay(), window);
    x->flags = USPosition;
    XSetWMNormalHints(getDisplay(), window, x);
}

bool XLibUtil::isValidWindowId(windowid_t window)
{
    XWindowAttributes attrib;
    // Check if we can get the window's attributes. If we can't that
    // indicates the window isn't valid.
    return (XGetWindowAttributes(getDisplay(), window, &attrib) != 0);
}

// Checks if this window is a normal window (i.e)
// - Has a WM_STATE
// - Not modal window
// - Not a purely transient window (with no window type set)
// - Not a special window (desktop/menu/util) as indicated in the window type
bool XLibUtil::isNormalWindow(windowid_t window)
{
    Atom type;
    int format;
    unsigned long left;
    Atom *data = NULL;
    unsigned long nitems;
    Window transient_for = 0;
    Display *display = getDisplay();

    static Atom wmState = XInternAtom(display, "WM_STATE", false);
    static Atom windowState = XInternAtom(display, "_NET_WM_STATE", false);
    static Atom modalWindow = XInternAtom(display, "_NET_WM_STATE_MODAL", false);
    static Atom windowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", false);
    static Atom normalWindow = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", false);
    static Atom dialogWindow = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", false);

    int ret = XGetWindowProperty(display, window, wmState, 0, 10, false, AnyPropertyType, &type, &format, &nitems,
                                 &left, (unsigned char **)&data);

    if (ret != Success || data == NULL) {
        if (data != NULL)
            XFree(data);
        return false;
    }
    if (data) {
        XFree(data);
    }

    ret = XGetWindowProperty(display, window, windowState, 0, 10, false, AnyPropertyType, &type, &format, &nitems,
                             &left, (unsigned char **)&data);
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

    XGetTransientForHint(display, window, &transient_for);

    ret = XGetWindowProperty(display, window, windowType, 0, 10, false, AnyPropertyType, &type, &format, &nitems, &left,
                             (unsigned char **)&data);

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

// Returns the contents of the _NET_WM_PID (which is supposed to contain the
// process id of the application that created the window)
static pid_t pid(Display *display, Window window)
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems, leftover;
    unsigned char *pid;
    pid_t pid_return = -1;
    static Atom type = XInternAtom(display, "_NET_WM_PID", false);

    if (XGetWindowProperty(display, window, type, 0, 1, false, XA_CARDINAL, &actual_type, &actual_format, &nitems,
                           &leftover, &pid) == Success)
    {
        if (pid) {
            pid_return = *(pid_t *)pid;
        }
        XFree(pid);
    }
    return pid_return;
}

// Walk the window's tree of subwindows until we find the window matching
// the window with the pid we're looking for.
static Window pidToWidEx(Display *display, Window window, bool checkNormality, pid_t epid)
{
    Window w = 0;
    Window root;
    Window parent;
    Window *child;
    unsigned int num_child;

    if (XQueryTree(display, window, &root, &parent, &child, &num_child) != 0) {
        for (unsigned int i = 0; i < num_child; i++) {
            if (epid == pid(display, child[i])) {
                if (checkNormality) {
                    if (XLibUtil::isNormalWindow(child[i])) {
                        return child[i];
                    }
                } else {
                    return child[i];
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

windowid_t XLibUtil::pidToWid(bool checkNormality, pid_t epid)
{
    // Walk from the top most (root) window going though all of them until we find
    // the one we want. Hopefully find the one we want.
    return pidToWidEx(getDisplay(), getDefaultRootWindow(), checkNormality, epid);
}

// Checks if window window has matching name
static bool analyzeWindow(Display *display, windowid_t window, const QRegularExpression &ename)
{
    // Can't analyze windows without a name
    char *window_name = NULL;
    if (!XFetchName(display, window, &window_name))
        return false;

    if (window_name) {
        XFree(window_name);
    } else {
        return false;
    }

    // lets try the program name
    bool this_is_our_man = false;
    XClassHint ch;
    memset(&ch, 0, sizeof(ch));
    if (XGetClassHint(display, window, &ch)) {
        // Checking res_name first because it is the window title and for
        // something like a text editor could show the document name or
        // something like "unsaved". This allows for the user to more
        // robustly match if there are multiple windows they're trying
        // to differentiate multiple windows of the same application.
        //
        // Fall back to res_class which is the application name.
        if (ch.res_name && QString(ch.res_name).contains(ename)) {
            this_is_our_man = true;
        } else if (ch.res_class && QString(ch.res_class).contains(ename)) {
            this_is_our_man = true;
        } else {
            // sheer desperation
            char *wm_name = NULL;
            XFetchName(display, window, &wm_name);
            if (wm_name && QString(wm_name).contains(ename)) {
                this_is_our_man = true;
            }
            XFree(wm_name);
        }

        if (ch.res_class) {
            XFree(ch.res_class);
        }
        if (ch.res_name) {
            XFree(ch.res_name);
        }
    }

    return this_is_our_man;
}

// Given a starting window look though all children and try to find a window
// that matches the ename.
static Window findWindowEx(Display *display, Window window, bool checkNormality, const QRegularExpression &ename,
                           QList<windowid_t> dockedWindows = QList<windowid_t>())
{
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
            w = findWindowEx(display, child[i], checkNormality, ename, dockedWindows);
            if (w != 0) {
                break;
            }
        }
    }
    return w;
}

windowid_t XLibUtil::findWindow(bool checkNormality, const QRegularExpression &ename, QList<windowid_t> dockedWindows)
{
    // Walk from the top most (root) window going though all of them until we find
    // the one we want. Hopefully find the one we want.
    return findWindowEx(getDisplay(), getDefaultRootWindow(), checkNormality, ename, dockedWindows);
}

// Sends a given ClientMessage to a window.
static void sendMessage(Display *display, Window to, Window window, Atom type, int format, long mask, void *data,
                        int size)
{
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = window;
    ev.xclient.message_type = type;
    ev.xclient.format = format;
    memcpy((char *)&ev.xclient.data, (const char *)data, size);
    XSendEvent(display, to, false, mask, &ev);
    XSync(display, false);
}

void sendMessageWMState(Window window, Atom state_type, bool set)
{
    Display *display = getDisplay();
    static Atom type = XInternAtom(display, "_NET_WM_STATE", true);
    // true = add the state to the window.
    // false, remove the state from the window.
    qint64 l[2] = {set ? 1 : 0, static_cast<qint64>(state_type)};
    sendMessage(display, getDefaultRootWindow(), window, type, 32, SubstructureNotifyMask, l, sizeof(l));
}

void XLibUtil::setWindowSkipTaskbar(windowid_t window, bool set)
{
    static Atom atom = XInternAtom(getDisplay(), "_NET_WM_STATE_SKIP_TASKBAR", false);
    sendMessageWMState(window, atom, set);
}

void XLibUtil::setWindowSkipPager(windowid_t window, bool set)
{
    static Atom atom = XInternAtom(getDisplay(), "_NET_WM_STATE_SKIP_PAGER", false);
    sendMessageWMState(window, atom, set);
}

void XLibUtil::setWindowSticky(windowid_t window, bool set)
{
    static Atom atom = XInternAtom(getDisplay(), "_NET_WM_STATE_STICKY", false);
    sendMessageWMState(window, atom, set);
}

void XLibUtil::setCurrentDesktop(long desktop)
{
    Display *display = getDisplay();
    static Atom type = XInternAtom(display, "_NET_CURRENT_DESKTOP", true);
    Window root = getDefaultRootWindow();
    long l_currDesk[2] = {desktop, CurrentTime};
    sendMessage(display, root, root, type, 32, SubstructureNotifyMask | SubstructureRedirectMask, l_currDesk,
                sizeof(l_currDesk));
}

void XLibUtil::setWindowDesktop(long desktop, windowid_t window)
{
    Display *display = getDisplay();
    static Atom type = XInternAtom(display, "_NET_WM_DESKTOP", true);
    long l_wmDesk[2] = {desktop, 1}; // 1 == request sent from application. 2 == from pager
    sendMessage(display, getDefaultRootWindow(), window, type, 32, SubstructureNotifyMask | SubstructureRedirectMask,
                l_wmDesk, sizeof(l_wmDesk));
}

void XLibUtil::setActiveWindow(windowid_t window)
{
    Display *display = getDisplay();
    static Atom type = XInternAtom(display, "_NET_ACTIVE_WINDOW", true);
    // 1 == request sent from application. 2 == from pager.
    // We use 2 because KWin doesn't always give the window focus with 1.
    long l_active[2] = {2, CurrentTime};
    sendMessage(getDisplay(), getDefaultRootWindow(), window, type, 32,
                SubstructureNotifyMask | SubstructureRedirectMask, l_active, sizeof(l_active));
    XSetInputFocus(display, window, RevertToParent, CurrentTime);
}

void XLibUtil::closeWindow(windowid_t window)
{
    static Atom type = XInternAtom(getDisplay(), "_NET_CLOSE_WINDOW", true);
    long l[5] = {0, 0, 0, 0, 0};
    sendMessage(getDisplay(), getDefaultRootWindow(), window, type, 32,
                SubstructureNotifyMask | SubstructureRedirectMask, l, sizeof(l));
}

// Returns the id of the currently active window.
windowid_t XLibUtil::getActiveWindow()
{
    Display *display = getDisplay();
    Atom active_window_atom = XInternAtom(getDisplay(), "_NET_ACTIVE_WINDOW", true);
    Atom type = 0;
    int format;
    unsigned long nitems, after;
    unsigned char *data = NULL;
    Window root = getDefaultRootWindow();

    int r = XGetWindowProperty(display, root, active_window_atom, 0, 1, false, AnyPropertyType, &type, &format, &nitems,
                               &after, &data);

    Window window = 0;
    if ((r == Success) && data && (*reinterpret_cast<windowid_t *>(data) != 0)) {
        window = *(windowid_t *)data;
    } else {
        int revert;
        XGetInputFocus(display, &window, &revert);
    }
    return window;
}

static windowid_t findWMStateWindowChildren(Display *display, windowid_t window, Atom wmState)
{
    Window w = 0;
    Window root;
    Window parent;
    Window *child;
    unsigned int num_child;

    if (XQueryTree(display, window, &root, &parent, &child, &num_child) != 0) {
        for (unsigned int i = 0; i < num_child; i++) {
            int format;
            unsigned long left;
            Atom *data = NULL;
            Atom type = 0;
            unsigned long nitems;

            int ret = XGetWindowProperty(display, child[i], wmState, 0, 0, false, AnyPropertyType, &type, &format,
                                         &nitems, &left, (unsigned char **)&data);

            if (data != NULL) {
                XFree(data);
            }

            if (ret == Success && type) {
                return child[i];
            }

            w = findWMStateWindowChildren(display, child[i], wmState);
            if (w != 0) {
                return w;
            }
        }
    }

    return w;
}

// Functional equivelent to libXmu's XmuClientWindow.
static windowid_t findWMStateWindow(Display *display, windowid_t window)
{
    int format;
    unsigned long left;
    Atom *data = NULL;
    Atom type = 0;
    unsigned long nitems;
    static Atom wmState = XInternAtom(display, "WM_STATE", false);

    int ret = XGetWindowProperty(display, window, wmState, 0, 0, false, AnyPropertyType, &type, &format, &nitems, &left,
                                 (unsigned char **)&data);
    if (data != NULL)
        XFree(data);

    if (ret == Success && type)
        return window;

    return findWMStateWindowChildren(display, window, wmState);
}

windowid_t XLibUtil::selectWindow(GrabInfo &grabInfo, QString &error)
{
    Display *display = getDisplay();
    Window root = getDefaultRootWindow();
    Cursor cursor = XCreateFontCursor(display, XC_draped_box);
    if (cursor == 0) {
        error = tr("Failed to create XC_draped_box");
        return 0;
    }
    if (XGrabPointer(display, root, false, ButtonPressMask | ButtonReleaseMask, GrabModeSync, GrabModeAsync, 0, cursor,
                     CurrentTime) != GrabSuccess)
    {
        error = tr("Failed to grab mouse");
        return 0;
    }

    //  X11 treats Scroll_Lock & Num_Lock as 'modifiers'; each exact combination has to be grabbed
    //   from [ESC + No Modifier] (raw) through to [ESC + CAPS_lock + NUM_lock + SCROLL_lock]
    //  Cannot use 'AnyModifier' here in case, say, CTRL+ESC is in use (results in failure to grab at all)
    KeyCode keyEsc = XKeysymToKeycode(display, XK_Escape);
    for (int b = 0; b < BIT3; b++) {                  // 000..111 (grab eight Escape key combinations)
        int modifiers = ((b & BIT0) ? LockMask : 0) | // CAPS_lock
                        ((b & BIT1) ? Mod2Mask : 0) | // NUM_lock
                        ((b & BIT2) ? Mod5Mask : 0);  // SCROLL_lock
        XGrabKey(display, keyEsc, modifiers, root, False, GrabModeAsync, GrabModeAsync);
    }
    XSelectInput(display, root, KeyPressMask);
    XAllowEvents(display, SyncPointer, CurrentTime);
    XSync(display, false);

    grabInfo.exec(); // Block waiting for a selection, cancel, or timeout.

    XUngrabPointer(display, CurrentTime);
    XUngrabKey(display, keyEsc, AnyModifier, root);
    XSelectInput(display, root, NoEventMask);
    XFreeCursor(display, cursor);

    if (grabInfo.getButton() != Button1 || !grabInfo.getWindow() || !grabInfo.isActive())
        return 0;

    // Find window (or subwindow) of the grabbed window with WM_STATE which should be a dockable window.
    windowid_t w = findWMStateWindow(display, grabInfo.getWindow());
    if (w != 0)
        return w;
    // Return the window that was grabbed even if it's not a proper dockable window.
    // It was grabbed and if the user want's to check if it's a valid window they have
    // that option which happens elsewhere.
    return grabInfo.getWindow();
}

void XLibUtil::subscribe(windowid_t window)
{
    Display *display = getDisplay();
    Window root = getDefaultRootWindow();
    XWindowAttributes attr;

    XGetWindowAttributes(display, window == 0 ? root : window, &attr);

    XSelectInput(display, window == 0 ? root : window,
                 attr.your_event_mask | StructureNotifyMask | PropertyChangeMask | VisibilityChangeMask |
                     FocusChangeMask);
    XSync(display, false);
}

void XLibUtil::unSubscribe(windowid_t window)
{
    Display *display = getDisplay();
    XSelectInput(display, window, NoEventMask);
    XSync(display, false);
}

// Sets data to the value of the requested window property.
bool getCardinalProperty(Display *display, windowid_t window, Atom prop, long *data)
{
    Atom type;
    int format;
    unsigned long nitems, bytes;
    unsigned char *d = NULL;

    if (XGetWindowProperty(display, window, prop, 0, 1, false, XA_CARDINAL, &type, &format, &nitems, &bytes, &d) ==
            Success &&
        d)
    {
        if (data) {
            *data = *reinterpret_cast<long *>(d);
        }
        XFree(d);
        return true;
    }
    return false;
}

long XLibUtil::getWindowDesktop(windowid_t window)
{
    long desktop = 0;
    Display *display = getDisplay();
    static Atom _NET_WM_DESKTOP = XInternAtom(display, "_NET_WM_DESKTOP", true);
    getCardinalProperty(display, window, _NET_WM_DESKTOP, &desktop);
    return desktop;
}

long XLibUtil::getCurrentDesktop()
{
    long desktop = 0;
    Display *display = getDisplay();
    Atom type = 0;
    int format;
    unsigned long nitems, after;
    unsigned char *data = 0;

    static Atom _NET_CURRENT_DESKTOP = XInternAtom(display, "_NET_CURRENT_DESKTOP", true);

    int r = XGetWindowProperty(display, DefaultRootWindow(display), _NET_CURRENT_DESKTOP, 0, 4, false, AnyPropertyType,
                               &type, &format, &nitems, &after, &data);
    if (r == Success && data)
        desktop = *reinterpret_cast<long *>(data);

    XFree(data);
    return desktop;
}

void XLibUtil::iconifyWindow(windowid_t window)
{
    Display *display = getDisplay();
    int screen = DefaultScreen(display);

    // A simple call to XWithdrawWindow wont do. Here is what we do:
    // 1. Iconify. This will make the application hide all its other windows. For
    //    example, xmms would take off the playlist and equalizer window.
    // 2. Withdraw the window to remove it from the taskbar.
    XIconifyWindow(display, window, screen); // good for effects too
    XSync(display, false);
    XWithdrawWindow(display, window, screen);
}

// Is the window in an iconified state.
bool XLibUtil::isWindowIconic(windowid_t window)
{
    bool iconic = false;
    Atom type = 0;
    int format;
    unsigned long nitems, after;
    unsigned char *data = 0;
    Display *display = getDisplay();
    static Atom WM_STATE = XInternAtom(display, "WM_STATE", true);

    int r = XGetWindowProperty(display, window, WM_STATE, 0, 1, false, AnyPropertyType, &type, &format, &nitems, &after,
                               &data);
    if ((r == Success) && data && (*reinterpret_cast<long *>(data) == IconicState))
        iconic = true;

    XFree(data);
    return iconic;
}

void XLibUtil::raiseWindow(windowid_t window)
{
    Display *display = getDisplay();
    XMapRaised(display, window);
    XFlush(display);
}

// We only consider images with at least 10% of pixels opaque. There have
// been issues with completely transparent icons being read.
// Either we'll try another icon location or we'll fallback to the no icon
// found icon.
static bool imageMeetsMinimumOpaque(size_t num_opaque, size_t width, size_t height)
{
    if (static_cast<double>(num_opaque) / static_cast<double>(width * height) > 0.1d)
        return true;
    return false;
}

static QRgb convertToQColor(unsigned long pixel)
{
    return qRgba((pixel & 0x00FF0000) >> 16,  // Red
                 (pixel & 0x0000FF00) >> 8,   // Green
                 (pixel & 0x000000FF),        // Blue
                 (pixel & 0xFF000000) >> 24); // Alpha
}

static QImage imageFromX11IconData(unsigned long *iconData, unsigned long dataLength)
{
    if (!iconData || dataLength < 2)
        return QImage();

    unsigned long width = iconData[0];
    unsigned long height = iconData[1];

    if (width == 0 || height == 0 || dataLength < width * height + 2)
        return QImage();

    size_t num_opaque = 0;
    QVector<QRgb> pixels(width * height);
    unsigned long *src = iconData + 2;
    for (unsigned long i = 0; i < width * height; ++i) {
        pixels[i] = convertToQColor(src[i]);
        if (qAlpha(pixels[i]) != 0) {
            num_opaque++;
        }
    }

    if (!imageMeetsMinimumOpaque(num_opaque, width, height))
        return QImage();

    QImage iconImage((uchar *)pixels.data(), width, height, QImage::Format_ARGB32);
    return iconImage.copy();
}

static QImage imageFromX11Pixmap(Display *display, Pixmap pixmap, int x, int y, unsigned int width, unsigned int height)
{
    XImage *ximage = XGetImage(display, pixmap, x, y, width, height, AllPlanes, ZPixmap);
    if (!ximage)
        return QImage();

    QImage image((uchar *)ximage->data, width, height, QImage::Format_ARGB32);
    size_t num_opaque = 0;
    for (size_t i = 0; i < width; i++) {
        for (size_t j = 0; j < height; j++) {
            if (qAlpha(image.pixel(i, j)) != 0) {
                num_opaque++;
            }
        }
    }

    QImage result;
    if (imageMeetsMinimumOpaque(num_opaque, width, height))
        result = image.copy(); // Make a copy of the image data

    XDestroyImage(ximage);
    return result;
}

static QPixmap getWindowIconNetWMIcon(windowid_t window)
{
    QPixmap appIcon;
    Display *display = getDisplay();
    static Atom netWmIcon = XInternAtom(display, "_NET_WM_ICON", false);
    Atom actualType;
    int actualFormat;
    unsigned long nItems, bytesAfter;
    unsigned char *data = nullptr;

    if (XGetWindowProperty(display, window, netWmIcon, 0, LONG_MAX, false, XA_CARDINAL, &actualType, &actualFormat,
                &nItems, &bytesAfter, &data) != Success || data == NULL)
        return appIcon;

    if (actualFormat != 32)
        return appIcon;

    unsigned long *iconData = reinterpret_cast<unsigned long *>(data);
    unsigned long dataLength = nItems;

    // Extract the largest icon available
    QImage largestImage;
    unsigned long maxIconSize = 0;

    for (unsigned long i = 0; i < dataLength;) {
        unsigned long width = iconData[i];
        unsigned long height = iconData[i + 1];
        unsigned long iconSize = width * height;

        if (iconSize > maxIconSize) {
            largestImage = imageFromX11IconData(&iconData[i], dataLength - i);
            maxIconSize = iconSize;
        }

        i += (2 + iconSize);
    }

    if (!largestImage.isNull())
        appIcon = QPixmap::fromImage(largestImage);

    XFree(data);
    return appIcon;
}

static QPixmap getWindowIconWMHints(windowid_t window)
{
    QPixmap appIcon;
    Display *display = getDisplay();

    XWMHints *wm_hints = XGetWMHints(display, window);
    if (wm_hints == nullptr)
        return appIcon;

    if (wm_hints->icon_pixmap) {
        Window root;
        int x = 0, y = 0;
        unsigned int width = 0, height = 0, border_width, depth;
        XGetGeometry(display, wm_hints->icon_pixmap, &root, &x, &y, &width, &height, &border_width, &depth);

        QImage image = imageFromX11Pixmap(display, wm_hints->icon_pixmap, x, y, width, height);
        if (!image.isNull()) {
            appIcon = QPixmap::fromImage(image);
        }
    }

    XFree(wm_hints);
    return appIcon;
}

QPixmap XLibUtil::getWindowIcon(windowid_t window)
{
    if (!window)
        return QPixmap();

    // First try _NET_WM_ICON
    QPixmap appIcon = getWindowIconNetWMIcon(window);

    // Fallback to WM_HINTS if _NET_WM_ICON wasn't set
    if (appIcon.isNull())
        appIcon = getWindowIconWMHints(window);

    return appIcon;
}

QString XLibUtil::getAppName(windowid_t window)
{
    XClassHint ch;
    memset(&ch, 0, sizeof(ch));
    QString name;

    if (XGetClassHint(getDisplay(), window, &ch)) {
        // res_class is the name of the application
        //
        // res_name is the text shown in the window title. Could include document
        // names or something like "unsaved" like you'll see with a text editor
        if (ch.res_class) {
            name = QString(ch.res_class);
        } else if (ch.res_name) {
            name = QString(ch.res_name);
        }

        if (ch.res_class) {
            XFree(ch.res_class);
        }
        if (ch.res_name) {
            XFree(ch.res_name);
        }
    }

    return name;
}

QString XLibUtil::getWindowTitle(windowid_t window)
{
    char *windowName = 0;
    QString title;

    XFetchName(getDisplay(), window, &windowName);
    title = windowName;

    if (windowName)
        XFree(windowName);

    return title;
}

atom_t XLibUtil::getAtom(const char *name)
{
    return XInternAtom(getDisplay(), name, true);
}
