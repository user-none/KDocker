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

#include "grabinfo.h"

GrabInfo::GrabInfo()
{
    connect(&m_qtimer, &QTimer::timeout, &m_qloop, &QEventLoop::quit);
}

void GrabInfo::exec()
{
    m_window = 0;
    m_button = 0;
    m_isGrabbing = true; // Enable XCB_BUTTON_PRESS code in event filter
                         //
    m_qtimer.setSingleShot(true);
    // m_qtimer-> start(20000);   // 20 second timeout
    m_qtimer.start(5000); // 5 second timeout

    m_qloop.exec();     // block until button pressed or timeout
}

void GrabInfo::stop()
{
    m_isGrabbing = false;
    m_qloop.quit();
}

void GrabInfo::stopGrabbing()
{
    m_isGrabbing = false;
}

bool GrabInfo::isGrabbing()
{
    return m_isGrabbing;
}

bool GrabInfo::isActive()
{
    return m_qtimer.isActive();
}

Window GrabInfo::getWindow()
{
    return m_window;
}

unsigned int GrabInfo::getButton()
{
    return m_button;
}

void GrabInfo::setWindow(Window w)
{
    m_window = w;
}

void GrabInfo::setButton(unsigned int button)
{
    m_button = button;
}
