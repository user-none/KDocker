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

#include "commandlineargs.h"

#include <QRegularExpression>
#include <QString>
#include <QStringList>

bool CommandLineArgs::processArgs(const QStringList &arguments, Command &command, TrayItemOptions &config,
                                  bool &keepRunning)
{
    QCommandLineParser parser;

    parser.setApplicationDescription(
        "Dock almost anything\n\n"
        "Kdocker can dock a window in a few different ways\n"
        "1. By specifying an application to launch (app argument)\n"
        "2. Specifying a regular expression to find a window based on its title (-n)\n"
        "3. Specifying a window id (-w)\n"
        "4. Specifying a pid (-x)\n"
        "5. Not specifying any of the above. KDocker will have you select the window to dock. Negated by the -z option\n\n"
        "The -n option is compatible with application launching. Useful if the application spawns another process (E.g. a launcher)");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("app", "Optional application to launch. Use -- followed by any application arguments");
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);

    parser.addOptions({
        {{"b", "blind"}, "Suppress the warning dialog when docking non-normal windows (blind mode)"},
        {{"d", "timeout"}, "Maximum time in seconds to allow for a command to start and open a window", "sec", "5"},
        {{"f", "dock-focused"}, "Dock the window that has focus (active window)"},
        // Don't use h or help because they're already handled by the parser object.
        {{"i", "icon"}, "Custom icon path", "file"},
        {{"I", "attention-icon"},
         "Custom attention icon path. This icon is set if the title  of the application window changes while it is iconified",
         "file"},
        {{"l", "iconify-focus-lost"}, "Iconify when focus lost"},
        {"m", "Don't iconfiy when minimized"},
        {{"n", "search-pattern"}, "Match window based on its name (title) using a PCRE regular expression", "pattern"},
        {{"o", "iconify-obscured"}, "Iconify when obscured by other windows"},
        {{"p", "notify-time"},
         "Maximum time in seconds to display a notification when window title changes",
         "sec",
         "4"},
        {{"q", "quiet"}, "Disable notifying window title changes"},
        {{"r", "skip-pager"}, "Remove this application from the pager"},
        {{"s", "sticky"}, "Make the window sticky (appears on all desktops)"},
        {{"t", "skip-taskbar"}, "Remove this application from the taskbar"},
        // Don't use v or version because they're already handled by the parser object.
        {{"w", "window-id"}, "Window id of the application to dock. Hex number formatted (0x###...)", "window-id"},
        {{"x", "pid"}, "Process id of the application to dock. Decimal number (###...)", "pid"},
        {{"z", "keep-running"}, "Run in the background and don't exit if no windows are docked"},
    });

    parser.process(arguments);

    if (!validateParserArgs(parser))
        return false;

    buildConfig(parser, config);
    buildCommand(parser, command);

    keepRunning = false;
    if (parser.isSet("keep-running"))
        keepRunning = true;

    return true;
}

bool CommandLineArgs::validateParserArgs(const QCommandLineParser &parser)
{
    // Ensure only one of the target a specific window actions was specified
    int num_dock_requests = 0;
    if (parser.isSet("window-id"))
        num_dock_requests++;
    if (parser.isSet("pid"))
        num_dock_requests++;
    if (parser.positionalArguments().size() > 0)
        num_dock_requests++;
    if (num_dock_requests > 1) {
        qCritical() << "Only one of, -w, -x, or application can be specified";
        return false;
    }

    // Verify the window id is a valid number
    if (parser.isSet("window-id")) {
        bool ok;
        windowid_t id = QString(parser.value("window-id")).toUInt(&ok, 0);
        if (!ok || id == 0) {
            qCritical() << "Failed to parse window id";
            return false;
        }
    }

    // Verify the pid is a valid number
    if (parser.isSet("pid")) {
        bool ok;
        pid_t id = QString(parser.value("pid")).toInt(&ok, 0);
        if (!ok || id <= 0) {
            qCritical() << "Failed to parse pid";
            return false;
        }
    }

    return true;
}

void CommandLineArgs::buildConfig(const QCommandLineParser &parser, TrayItemOptions &config)
{
    if (parser.isSet("icon"))
        config.setIconPath(parser.value("icon"));

    if (parser.isSet("attention-icon"))
        config.setAttentionIconPath(parser.value("attention-icon"));

    if (parser.isSet("iconify-focus-lost"))
        config.setIconifyFocusLost(TrayItemOptions::TriState::SetTrue);

    if (parser.isSet("m"))
        config.setIconifyMinimized(TrayItemOptions::TriState::SetFalse);

    if (parser.isSet("iconify-obscured"))
        config.setIconifyObscured(TrayItemOptions::TriState::SetTrue);

    if (parser.isSet("notify-time")) {
        bool ok;
        config.setNotifyTime(QString(parser.value("notify-time")).toUInt(&ok, 0) * 1000);
    }

    if (parser.isSet("quiet"))
        config.setQuiet(TrayItemOptions::TriState::SetTrue);

    if (parser.isSet("skip-pager"))
        config.setSkipPager(TrayItemOptions::TriState::SetTrue);

    if (parser.isSet("sticky"))
        config.setSticky(TrayItemOptions::TriState::SetTrue);

    if (parser.isSet("skip-taskbar"))
        config.setSkipTaskbar(TrayItemOptions::TriState::SetTrue);
}

void CommandLineArgs::buildCommand(const QCommandLineParser &parser, Command &command)
{
    // Title is separate from the rest because a title can be used when launching an app.
    // If launching an app the command type will be changed.
    if (parser.isSet("search-pattern")) {
        command.setType(Command::Type::Title);
        command.setSearchPattern(parser.value("search-pattern"));
    }

    // Timeout can be used by search pattern and launch
    if (parser.isSet("timeout")) {
        bool ok;
        quint32 timeout = QString(parser.value("timeout")).toUInt(&ok, 0);
        if (!ok || timeout == 0) {
            timeout = 1;
        }
        if (timeout > 100) {
            timeout = 100;
        }
        command.setTimeout(timeout);
    }

    // Can be used by search-pattern, launch, select, focused
    if (parser.isSet("blind"))
        command.setCheckNormality(false);

    // Arguments that dictate specific actionst that are mutually exclusive
    if (parser.isSet("window-id")) {
        bool ok;
        command.setType(Command::Type::WindowId);
        command.setWindowId(QString(parser.value("window-id")).toUInt(&ok, 0));
    } else if (parser.isSet("pid")) {
        bool ok;
        command.setType(Command::Type::Pid);
        command.setPid(QString(parser.value("pid")).toInt(&ok, 0));
    } else if (parser.positionalArguments().size() > 0) {
        command.setType(Command::Type::Launch);
        command.setLaunchApp(parser.positionalArguments().at(0));
        command.setLaunchAppArguments(parser.positionalArguments().sliced(1));
    } else if (parser.isSet("dock-focused")) {
        command.setType(Command::Type::Focused);
    }

    // None of the other commands were specified leaving select window as the only option
    if (command.getType() == Command::Type::NoCommand)
        command.setType(Command::Type::Select);
}
