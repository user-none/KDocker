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

// $Id: util.h,v 1.3 2004/11/19 15:44:51 cs19713 Exp $

#ifndef _UTIL_H
#define _UTIL_H

#include <QString>
#include <X11/Xlib.h>
#include <sys/types.h>

extern bool isNormalWindow(Display *display, Window w);
extern bool isValidWindowId(Display *display, Window w);
extern pid_t pid(Display *display, Window w);
extern Window pidToWid(Display *display, Window window, bool checkNormality, pid_t epid);
extern bool analyzeWindow(Display *display, Window w, const QString &ename);
extern Window findWindow(Display *display, Window w, bool checkNormality, const QString &ename);
extern void sendMessage(Display *display, Window to, Window w, const char *type, int format, long mask, void *data, int size);
extern Window activeWindow(Display *display);
extern Window selectWindow(Display *display, const char **err = 0);
extern void subscribe(Display *display, Window w, long mask, bool set);
extern bool getCardinalProperty(Display *display, Window w, Atom prop, long *data);

#endif
