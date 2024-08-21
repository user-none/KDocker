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
#include <X11/Xmu/WinUtil.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#define BIT0 (1 << 0)
#define BIT1 (1 << 1)
#define BIT2 (1 << 2)
#define BIT3 (1 << 3)

static int ignoreXErrors(Display *, XErrorEvent *)
{
    return 0;
}

static Display *getDisplay()
{
    return qApp->nativeInterface<QNativeInterface::QX11Application>()->display();
}

static Window getDefaultRootWindow()
{
    return DefaultRootWindow(getDisplay());
}

void XLibUtil::silenceXErrors()
{
    // Qt regeistereds the X error handler and writes the errors to
    // the console. We can cause errors to be logged that can be safely ignored.
    //
    // This will prevent x errors from being written to the console.
    // The isValidWindowId function in will generate errors if the
    // window is not valid while it is checking.
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

void XLibUtil::getWMSizeHints(Window w, XLibUtilSizeHints *sh)
{
    XSizeHints *x = static_cast<XSizeHints *>(sh);
    long dummy;
    XGetWMNormalHints(getDisplay(), w, x, &dummy);
}

void XLibUtil::setWMSizeHints(Window w, XLibUtilSizeHints *sh)
{
    XSizeHints *x = static_cast<XSizeHints *>(sh);

    x->flags = USPosition;
    XSetWMNormalHints(getDisplay(), w, x);
}

/*
 * Assert validity of the window id. Get window attributes for the heck of it
 * and see if the request went through.
 */
bool XLibUtil::isValidWindowId(Window w)
{
    XWindowAttributes attrib;
    return (XGetWindowAttributes(getDisplay(), w, &attrib) != 0);
}

/*
 * Checks if this window is a normal window (i.e)
 * - Has a WM_STATE
 * - Not modal window
 * - Not a purely transient window (with no window type set)
 * - Not a special window (desktop/menu/util) as indicated in the window type
 */
bool XLibUtil::isNormalWindow(Window w)
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

    int ret = XGetWindowProperty(display, w, wmState, 0, 10, false, AnyPropertyType, &type, &format, &nitems, &left,
                                 (unsigned char **)&data);

    if (ret != Success || data == NULL) {
        if (data != NULL)
            XFree(data);
        return false;
    }
    if (data) {
        XFree(data);
    }

    ret = XGetWindowProperty(display, w, windowState, 0, 10, false, AnyPropertyType, &type, &format, &nitems, &left,
                             (unsigned char **)&data);
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

    ret = XGetWindowProperty(display, w, windowType, 0, 10, false, AnyPropertyType, &type, &format, &nitems, &left,
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

/*
 * Returns the contents of the _NET_WM_PID (which is supposed to contain the
 * process id of the application that created the window)
 */
static pid_t pid(Display *display, Window w)
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems, leftover;
    unsigned char *pid;
    pid_t pid_return = -1;

    if (XGetWindowProperty(display, w, XInternAtom(display, "_NET_WM_PID", false), 0, 1, false, XA_CARDINAL,
                           &actual_type, &actual_format, &nitems, &leftover, &pid) == Success)
    {
        if (pid) {
            pid_return = *(pid_t *)pid;
        }
        XFree(pid);
    }
    return pid_return;
}

static Window pidToWidEx(Display *display, Window window, bool checkNormality, pid_t epid,
                         QList<Window> dockedWindows = QList<Window>())
{
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

Window XLibUtil::pidToWid(bool checkNormality, pid_t epid, QList<Window> dockedWindows)
{
    return pidToWidEx(getDisplay(), getDefaultRootWindow(), checkNormality, epid, dockedWindows);
}

/*
 * The Grand Window Analyzer. Checks if window w has matching name
 */
static bool analyzeWindow(Display *display, Window w, const QRegularExpression &ename)
{
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
static Window findWindowEx(Display *display, Window window, bool checkNormality, const QRegularExpression &ename,
                           QList<Window> dockedWindows = QList<Window>())
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
            w = findWindowEx(display, child[i], checkNormality, ename);
            if (w != 0) {
                break;
            }
        }
    }
    return w;
}

Window XLibUtil::findWindow(bool checkNormality, const QRegularExpression &ename, QList<Window> dockedWindows)
{
    return findWindowEx(getDisplay(), getDefaultRootWindow(), checkNormality, ename, dockedWindows);
}

/*
 * Sends ClientMessage to a window.
 */
static void sendMessage(Display *display, Window to, Window w, const char *type, int format, long mask, void *data,
                        int size)
{
    XEvent ev;
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = XInternAtom(display, type, true);
    ev.xclient.format = format;
    memcpy((char *)&ev.xclient.data, (const char *)data, size);
    XSendEvent(display, to, false, mask, &ev);
    XSync(display, false);
}

void XLibUtil::sendMessageWMState(Window w, const char *type, bool set)
{
    // set, true = add the state to the window. false, remove the state from
    // the window.
    Display *display = getDisplay();
    Atom atom = XInternAtom(display, type, false);

    qint64 l[2] = {set ? 1 : 0, static_cast<qint64>(atom)};
    sendMessage(display, getDefaultRootWindow(), w, "_NET_WM_STATE", 32, SubstructureNotifyMask, l, sizeof(l));
}

void XLibUtil::sendMessageCurrentDesktop(long desktop, Window w)
{
    Window root = getDefaultRootWindow();
    long l_currDesk[2] = {desktop, CurrentTime};
    sendMessage(getDisplay(), root, root, "_NET_CURRENT_DESKTOP", 32, SubstructureNotifyMask | SubstructureRedirectMask,
                l_currDesk, sizeof(l_currDesk));
}

void XLibUtil::sendMessageWMDesktop(long desktop, Window w)
{
    long l_wmDesk[2] = {desktop, 1}; // 1 == request sent from application. 2 == from pager
    sendMessage(getDisplay(), getDefaultRootWindow(), w, "_NET_WM_DESKTOP", 32,
                SubstructureNotifyMask | SubstructureRedirectMask, l_wmDesk, sizeof(l_wmDesk));
}

void XLibUtil::sendMessageActiveWindow(Window w)
{
    // 1 == request sent from application. 2 == from pager.
    // We use 2 because KWin doesn't always give the window focus with 1.
    long l_active[2] = {2, CurrentTime};
    sendMessage(getDisplay(), getDefaultRootWindow(), w, "_NET_ACTIVE_WINDOW", 32,
                SubstructureNotifyMask | SubstructureRedirectMask, l_active, sizeof(l_active));
    XSetInputFocus(getDisplay(), w, RevertToParent, CurrentTime);
}

void XLibUtil::sendMessageCloseWindow(Window w)
{
    long l[5] = {0, 0, 0, 0, 0};
    sendMessage(getDisplay(), getDefaultRootWindow(), w, "_NET_CLOSE_WINDOW", 32,
                SubstructureNotifyMask | SubstructureRedirectMask, l, sizeof(l));
}

/*
 * Returns the id of the currently active window.
 */
Window XLibUtil::activeWindow()
{
    Display *display = getDisplay();
    Atom active_window_atom = XInternAtom(display, "_NET_ACTIVE_WINDOW", true);
    Atom type = 0;
    int format;
    unsigned long nitems, after;
    unsigned char *data = NULL;
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);

    int r = XGetWindowProperty(display, root, active_window_atom, 0, 1, false, AnyPropertyType, &type, &format, &nitems,
                               &after, &data);

    Window w = 0;
    if ((r == Success) && data && (*reinterpret_cast<Window *>(data) != 0)) {
        w = *(Window *)data;
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
Window XLibUtil::selectWindow(GrabInfo &grabInfo, QString &error)
{
    Display *display = getDisplay();
    int screen = DefaultScreen(display);
    Window root = RootWindow(display, screen);
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
    //
    //  X11 treats Scroll_Lock & Num_Lock as 'modifiers'; each exact combination has to be grabbed
    //   from [ESC + No Modifier] (raw) through to [ESC + CAPS_lock + NUM_lock + SCROLL_lock]
    //  Cannot use 'AnyModifier' here in case, say, CTRL+ESC is in use (results in failure to grab at all)
    //
    KeyCode keyEsc = XKeysymToKeycode(display, XK_Escape);
    for (int b = 0; b < BIT3; b++) {                  // 000..111 (grab eight Escape key combinations)
        int modifiers = ((b & BIT0) ? LockMask : 0) | // CAPS_lock
                        ((b & BIT1) ? Mod2Mask : 0) | // NUM_lock
                        ((b & BIT2) ? Mod5Mask : 0);  // SCROLL_lock
        XGrabKey(display, keyEsc, modifiers, root, False, GrabModeAsync, GrabModeAsync);
    }
    XSelectInput(display, root, KeyPressMask);
    XAllowEvents(display, SyncPointer, CurrentTime);

    grabInfo.window = 0;
    grabInfo.button = 0;
    grabInfo.qtimer->setSingleShot(true);
    // grabInfo.qtimer-> start(20000);   // 20 second timeout
    grabInfo.qtimer->start(5000); // 5 second timeout

    XSync(display, false);

    grabInfo.isGrabbing = true; // Enable XCB_BUTTON_PRESS code in event filter
    grabInfo.qloop->exec();     // block until button pressed or timeout

    XUngrabPointer(display, CurrentTime);
    XUngrabKey(display, keyEsc, AnyModifier, root);
    XSelectInput(display, root, NoEventMask);
    XFreeCursor(display, cursor);

    if (grabInfo.button != Button1 || !grabInfo.window || !grabInfo.qtimer->isActive()) {
        return 0;
    }

    return XmuClientWindow(display, grabInfo.window);
}

/*
 * Have events associated with mask for the window set in the X11 Event loop
 * to the application.
 */
void XLibUtil::subscribe(Window w)
{
    XLibUtil::subscribe(w, StructureNotifyMask | PropertyChangeMask | VisibilityChangeMask | FocusChangeMask);
}
void XLibUtil::subscribe(Window w, long mask)
{
    Display *display = getDisplay();
    Window root = RootWindow(display, DefaultScreen(display));
    XWindowAttributes attr;

    XGetWindowAttributes(display, w == 0 ? root : w, &attr);

    XSelectInput(display, w == 0 ? root : w, attr.your_event_mask | mask);
    XSync(display, false);
}

void XLibUtil::unSubscribe(Window w)
{
    Display *display = getDisplay();
    XSelectInput(display, w, NoEventMask);
    XSync(display, false);
}

/*
 * Sets data to the value of the requested window property.
 */
bool getCardinalProperty(Display *display, Window w, Atom prop, long *data)
{
    Atom type;
    int format;
    unsigned long nitems, bytes;
    unsigned char *d = NULL;

    if (XGetWindowProperty(display, w, prop, 0, 1, false, XA_CARDINAL, &type, &format, &nitems, &bytes, &d) ==
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

long XLibUtil::getWindowDesktop(Window w)
{
    long desktop = 0;
    Display *display = getDisplay();
    static Atom _NET_WM_DESKTOP = XInternAtom(display, "_NET_WM_DESKTOP", true);
    getCardinalProperty(display, w, _NET_WM_DESKTOP, &desktop);
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

void XLibUtil::iconifyWindow(Window w)
{
    Display *display = getDisplay();
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

bool XLibUtil::isWindowIconic(Window w)
{
    bool iconic = false;
    Atom type = 0;
    int format;
    unsigned long nitems, after;
    unsigned char *data = 0;
    Display *display = getDisplay();
    static Atom WM_STATE = XInternAtom(display, "WM_STATE", true);

    int r =
        XGetWindowProperty(display, w, WM_STATE, 0, 1, false, AnyPropertyType, &type, &format, &nitems, &after, &data);
    if ((r == Success) && data && (*reinterpret_cast<long *>(data) == IconicState))
        iconic = true;
    XFree(data);

    return iconic;
}

void XLibUtil::mapWindow(Window w)
{
    XMapWindow(getDisplay(), w);
}

void XLibUtil::mapRaised(Window w)
{
    XMapRaised(getDisplay(), w);
}

void XLibUtil::flush()
{
    XFlush(getDisplay());
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
    if (!iconData || dataLength < 2) {
        return QImage();
    }

    unsigned long width = iconData[0];
    unsigned long height = iconData[1];

    if (width == 0 || height == 0 || dataLength < width * height + 2) {
        return QImage();
    }

    QVector<QRgb> pixels(width * height);
    unsigned long *src = iconData + 2;
    for (unsigned long i = 0; i < width * height; ++i) {
        pixels[i] = convertToQColor(src[i]);
    }

    QImage iconImage((uchar *)pixels.data(), width, height, QImage::Format_ARGB32);
    return iconImage.copy();
}

static QImage imageFromX11Pixmap(Display *display, Pixmap pixmap, int width, int height)
{
    XImage *ximage = XGetImage(display, pixmap, 0, 0, width, height, AllPlanes, ZPixmap);
    if (!ximage) {
        return QImage();
    }

    QImage image((uchar *)ximage->data, width, height, QImage::Format_ARGB32);
    QImage result = image.copy(); // Make a copy of the image data
    XDestroyImage(ximage);

    return result;
}

QPixmap XLibUtil::createIcon(Window window)
{
    if (!window)
        return QPixmap();

    Display *display = getDisplay();
    QPixmap appIcon;

    // First try to get the icon from WM_HINTS
    XWMHints *wm_hints = XGetWMHints(display, window);
    if (wm_hints != nullptr) {
        if (!(wm_hints->flags & IconMaskHint)) {
            wm_hints->icon_mask = 0;
        }

        if ((wm_hints->flags & IconPixmapHint) && (wm_hints->icon_pixmap)) {
            Window root;
            int x, y;
            unsigned int width, height, border_width, depth;
            XGetGeometry(display, wm_hints->icon_pixmap, &root, &x, &y, &width, &height, &border_width, &depth);

            QImage image = imageFromX11Pixmap(display, wm_hints->icon_pixmap, width, height);
            appIcon = QPixmap::fromImage(image);
        }

        XFree(wm_hints);
    }

    // Fallback to _NET_WM_ICON if WM_HINTS icon is not available
    if (appIcon.isNull()) {
        Atom netWmIcon = XInternAtom(display, "_NET_WM_ICON", false);
        Atom actualType;
        int actualFormat;
        unsigned long nItems, bytesAfter;
        unsigned char *data = nullptr;

        if (XGetWindowProperty(display, window, netWmIcon, 0, LONG_MAX, false, XA_CARDINAL, &actualType, &actualFormat,
                               &nItems, &bytesAfter, &data) == Success &&
            data)
        {
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

            if (!largestImage.isNull()) {
                appIcon = QPixmap::fromImage(largestImage);
            }

            XFree(data);
        }
    }

    return appIcon;
}

QString XLibUtil::getAppName(Window w)
{
    XClassHint ch;
    QString name;

    if (XGetClassHint(getDisplay(), w, &ch)) {
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

QString XLibUtil::getWindowTitle(Window w)
{
    char *windowName = 0;
    QString title;

    XFetchName(getDisplay(), w, &windowName);
    title = windowName;

    if (windowName)
        XFree(windowName);

    return title;
}

Atom XLibUtil::getAtom(const char *name)
{
    return XInternAtom(getDisplay(), name, true);
}
