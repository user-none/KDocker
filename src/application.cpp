/*
 *  Copyright (C) 2009 John Schember <john@nachtimwald.com>
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

#include "application.h"


Application::Application(const QString &appId, int &argc, char **argv) : QtSingleApplication(appId, argc, argv) {
    m_kdocker = 0;
    m_filter  = 0;
}

void Application::setKDockerInstance(KDocker *kdocker) {
    m_kdocker = kdocker;
    m_filter  = m_kdocker-> getTrayItemManager();
    installNativeEventFilter(m_filter);
}

void Application::close() {
    if (m_kdocker) {
        m_kdocker->undockAll();
    }
    removeNativeEventFilter(m_filter);
    quit();
}
