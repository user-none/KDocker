/*
 *  Copyright (C) 2024 John Schember <john@nachtimwald.com>
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

#ifndef _GRABINFO_H
#define _GRABINFO_H

#include <QObject>
#include <QEventLoop>
#include <QTimer>

#include "xlibtypes.h"

// This is used in a very bad and strange design that really needs to be replaced.
//
// TrayItemManager::userSelectWindow passes a GrabInfo object to XLibUtil::selectWindow.
// XLibUtil::selectWindow calls GrabInfo::exec which is blocking. Blocking happens by
// running an event loop that does nothing but block execution (m_qloop). Exection resumes
// when one of the following happens:
//
// 1. A timeout timer (m_qtimer) that starts when exec is called times out.
// 2. The native event filter receives an event of key press ESC (our cancel key).
// 3. The native event filter receives a button press event (mouse click).
//
// Number 1 just stops the event loop and resumes execution of
// XLibUtil::selectWindow. No window will have been stored in GrabInfo so no
// window is returned.
//
// Number 2 tells the GrabInfo object to stop the event loop and resumes
// execution of XLibUtil::selectWindow. No window will have been stored in
// GrabInfo so no window is returned.
//
// Number 3 will set the button click and the selected window (if there was
// one) in GrabInfo. Then it will tell the GrabInfo object to stop the event
// loop and resumes execution of XLibUtil::selectWindow. The button will
// be verified to a left mouse click, the window will be verified and if this
// all passes validation, the selected window will be docked.
//
// Essentially, we're going to use an object to broker data while also using it to
// block in a function while waiting for events that change the broker object
// and tell it when to stop blocking.

class GrabInfo : public QObject
{
    Q_OBJECT

public:
    GrabInfo();

    void exec();

    bool isGrabbing();
    bool isActive();
    windowid_t getWindow();
    unsigned int getButton();

    void setWindow(windowid_t window);
    void setButton(unsigned int button);

    void stopGrabbing();

public slots:
    void stop();

private:
    QTimer m_qtimer;
    QEventLoop m_qloop;

    windowid_t m_window;
    unsigned int m_button;
    bool m_isGrabbing;

};

#endif // _GRABINFO_H
