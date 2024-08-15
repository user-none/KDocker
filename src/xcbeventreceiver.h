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

#ifndef _XCBEVENTRECEIVER_H
#define	_XCBEVENTRECEIVER_H

#include <QThread>

// Receiver for xcb events that we can pass
// on to a listener waiting for events to process.
class XcbEventReciver : public QThread {
    Q_OBJECT

    public slots:
        void run() override;
        void quit();

    signals:
        // Events sent by this signal are allocated memory
        // that must be freed by the receiver. There can
        // only ever be one receiver for this signal due
        // to needing to free in the receiver.
        void xcbEvent(void *event);
};

#endif // _XCBEVENTRECEIVER_H
