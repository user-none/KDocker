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

#include "xcbeventreceiver.h"
#include "xlibutil.h"


// Atom denoting we should stop processing events.
// The number is a randomly generated unique identifier.
static xcb_atom_t STOP_PROCESSING_ATOM = 5678976;

void XcbEventReciver::run() {
    xcb_connection_t *connection = XLibUtil::xcbConnection();
    xcb_generic_event_t *event = nullptr;
    // xcb_wait_for_event is a blocking function that only returns
    // when there is an event pending. We use an atom sent via a
    // client message to tell us when we should stop processing.
    while ((event = xcb_wait_for_event(connection))) {
        // Check for our stop processing client message that
        // we can send telling us to stop processing events and break
        // out of the loop so the thread can exit.
        if ((event->response_type & ~0x80) == XCB_CLIENT_MESSAGE) {
            if (reinterpret_cast<xcb_client_message_event_t*>(event)->type == STOP_PROCESSING_ATOM) {
                free(event);
                break;
            }
        }

        emit xcbEvent(event);
    }
}

// Create a dummy window that we can send a client message to.
// Which we will receive telling us to stop processing events.
void XcbEventReciver::quit() {
    xcb_client_message_event_t event;
    memset(&event, 0, sizeof(event));

    xcb_connection_t *c = XLibUtil::xcbConnection();
    const xcb_window_t window = xcb_generate_id(c);
    xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(c));
    xcb_screen_t *screen = it.data;
    xcb_create_window(c, XCB_COPY_FROM_PARENT,
                      window, screen->root,
                      0, 0, 1, 1, 0, XCB_WINDOW_CLASS_INPUT_ONLY,
                      screen->root_visual, 0, nullptr);

    event.response_type = XCB_CLIENT_MESSAGE;
    event.format = 32;
    event.sequence = 0;
    event.window = window;
    event.type = STOP_PROCESSING_ATOM;
    event.data.data32[0] = 0;

    xcb_send_event(c, false, window, XCB_EVENT_MASK_NO_EVENT, reinterpret_cast<const char *>(&event));
    xcb_destroy_window(c, window);
    xcb_flush(c);
}
