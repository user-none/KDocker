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

#include "xlibtypes.h"

#include <QObject>
#include <QEventLoop>
#include <QTimer>

class GrabInfo : public QObject
{
    Q_OBJECT

public:
    GrabInfo();

    void exec();

    bool isGrabbing();
    bool isActive();
    Window getWindow();
    unsigned int getButton();

    void setWindow(Window w);
    void setButton(unsigned int button);

    void stopGrabbing();

public slots:
    void stop();

private:
    QTimer m_qtimer;
    QEventLoop m_qloop;

    Window m_window;
    unsigned int m_button;
    bool m_isGrabbing;

};

#endif // _GRABINFO_H
