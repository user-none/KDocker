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

#ifndef _COMMANDLINEARGS_H
#define _COMMANDLINEARGS_H

#include "command.h"
#include "trayitemoptions.h"

#include <QCommandLineParser>

class CommandLineArgs
{
public:
    static bool processArgs(const QStringList &arguments, Command &command, TrayItemOptions &config, bool &keepRunning);

private:
    static bool validateParserArgs(const QCommandLineParser &parser);
    static void buildConfig(const QCommandLineParser &parser, TrayItemOptions &config);
    static void buildCommand(const QCommandLineParser &parser, Command &command);
};

#endif // _COMMANDLINEARGS_H
