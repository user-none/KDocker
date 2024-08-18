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


#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include "commandlineargs.h"

bool CommandLineArgs::processArgs(const QStringList &arguments, Command &command, TrayItemConfig &config, bool &daemon) {
    QCommandLineParser parser;

    parser.setApplicationDescription("" \
            "Dock almost anything\n\n" \
            "Kdocker can dock a window in a few different ways\n" \
            "1. By specifying an application to run (app argument)\n" \
            "2. Specifying a window title to match (-n)\n" \
            "3. Specifying a window id (-w)\n" \
            "4. Specifying a pid (-x)\n" \
            "5. Not specifying any of the above. KDocker will have you select the window to dock. Negated by the -z option\n\n" \
            "Only one of 1-4 methods can be used. They are mutually exclusive. "
            "All other options apply to the docked window that's started or found.");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("app", "Optional application to run. Use -- followed by any application arguments");
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);

    parser.addOptions({
        { { "b", "blind" }, "Suppress the warning dialog when docking non-normal windows (blind mode)" },
        { { "d", "timeout" }, "Maximum time in seconds to allow for a command to start and open a window", "sec", "5" },
        { { "e", "title-match" }, "Name matching syntax. n = normal, substring matching (default). r = regular expression. w = wildcard.", "match", "n" },
        { { "f", "dock-focused" }, "Dock the window that has focus (active window)" },
        // Don't use h or help because they're already handled by the parser object.
        { { "i", "icon" }, "Custom icon path", "file" },
        { { "I", "attention-icon" }, "Custom attention icon path. This icon is set if the title  of the application window changes while it is iconified", "file" },
        { { "j", "case-match" }, "Case sensitive title matching" },
        { { "l", "iconify-focus-lost" }, "Iconify when focus lost" },
        { "m", "Don't iconfiy when minimized" },
        { { "n", "title" }, "Match window based on its title. Used with -e and -j options", "title" },
        { { "o", "iconify-obscured" }, "Iconify when obscured by other windows" },
        { { "p", "notify-time" }, "Maximum time in seconds to display a notification when window title changes", "sec", "4" },
        { { "q", "quiet" }, "Disable notifying window title changes" },
        { { "r", "skip-pager" }, "Remove this application from the pager" },
        { { "s", "sticky" }, "Make the window sticky (appears on all desktops)" },
        { { "t", "skip-taskbar" }, "Remove this application from the taskbar" },
        // Don't use v or version because they're already handled by the parser object.
        { { "w", "window-id" }, "Window id of the application to dock. Hex number formatted (0x###...)", "window-id" },
        { { "x", "pid" }, "Process id of the application to dock. Decimal number (###...)", "pid" },
        { { "z", "daemon" }, "Run in the background and don't exit if no windows are docked" },
    });

    parser.process(arguments);

    if (!validateParserArgs(parser)) {
        return false;
    }

    buildConfig(parser, config);
    buildCommand(parser, command);

    daemon = false;
    if (parser.isSet("daemon"))
        daemon = true;

    return true;
}

bool CommandLineArgs::validateParserArgs(const QCommandLineParser &parser) {
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
        uint id = QString(parser.value("window-id")).toUInt(&ok, 0);
        if (!ok || id == 0) {
            qCritical() << "Failed to parse window id";
            return false;
        }
    }

    // Verify the pid is a valid number
    if (parser.isSet("pid")) {
        bool ok;
        uint id = QString(parser.value("pid")).toUInt(&ok, 0);
        if (!ok || id == 0) {
            qCritical() << "Failed to parse pid";
            return false;
        }
    }

    return true;
}

void CommandLineArgs::buildConfig(const QCommandLineParser &parser, TrayItemConfig &config) {
    if (parser.isSet("icon")) {
        config.setIconPath(parser.value("icon"));
    }
    if (parser.isSet("attention-icon")) {
        config.setAttentionIconPath(parser.value("attention-icon"));
    }
    if (parser.isSet("iconify-focus-lost")) {
        config.setIconifyFocusLost(TrayItemConfig::TriState::SetTrue);
    }
    if (parser.isSet("m")) {
        config.setIconifyMinimized(TrayItemConfig::TriState::SetFalse);
    }
    if (parser.isSet("iconify-obscured")) {
        config.setIconifyObscured(TrayItemConfig::TriState::SetTrue);
    }
    if (parser.isSet("notify-time")) {
        bool ok;
        config.setNotifyTime(QString(parser.value("notify-time")).toUInt(&ok, 0) * 1000);
    }
    if (parser.isSet("quiet")) {
        config.setQuiet(TrayItemConfig::TriState::SetTrue);
    }
    if (parser.isSet("skip-pager")) {
        config.setSkipPager(TrayItemConfig::TriState::SetTrue);
    }
    if (parser.isSet("sticky")) {
        config.setSticky(TrayItemConfig::TriState::SetTrue);
    }
    if (parser.isSet("skip-taskbar")) {
        config.setSkipTaskbar(TrayItemConfig::TriState::SetTrue);
    }
}

void CommandLineArgs::buildCommand(const QCommandLineParser &parser, Command &command) {
    // Title is separate from the rest because a title can be used when launching an app.
    // If launching an app the command type will be changed.
    if (parser.isSet("title")) {
        QRegularExpression pattern;
        QString title = parser.value("title");

        QString matchType = parser.value("title-match");
        if (QString::compare(matchType, "r", Qt::CaseInsensitive) == 0) {
            pattern.setPattern(title);
        } else if (QString::compare(matchType, "w", Qt::CaseInsensitive) == 0) {
            pattern.setPattern(QRegularExpression::wildcardToRegularExpression(title));
        } else {
            pattern.setPattern(QRegularExpression::escape(title));
        }

        QRegularExpression::PatternOptions patternOptions = QRegularExpression::CaseInsensitiveOption;
        if (parser.isSet("case-match")) {
            patternOptions = QRegularExpression::NoPatternOption;
        }
        pattern.setPatternOptions(patternOptions);
        command.setSearchPattern(pattern);
    } 
    
    if (parser.isSet("window-id")) {
        bool ok;
        command.setType(Command::CommandType::WindowId);
        command.setWindowId(QString(parser.value("window-id")).toUInt(&ok, 0));
    } else if (parser.isSet("pid")) {
        bool ok;
        command.setType(Command::CommandType::Pid);
        command.setPid(QString(parser.value("pid")).toUInt(&ok, 0));
    } else if (parser.positionalArguments().size() > 0) {
        command.setType(Command::CommandType::Run);
        if (parser.isSet("timeout")) {
            bool ok;
            uint timeout = QString(parser.value("timeout")).toUInt(&ok, 0);
            if (timeout == 0) {
                timeout = 1;
            }
            if (timeout > 100) {
                timeout = 100;
            }
            command.setTimeout(timeout);
        }
        command.setRunApp(parser.positionalArguments().at(0));
        command.setRunAppArguments(parser.positionalArguments().sliced(1));
    } else if (parser.isSet("dock-focused")) {
        command.setType(Command::CommandType::Focused);
        if (parser.isSet("blind")) {
            command.setCheckNormality(false);
        }
    } else {
        if (parser.isSet("blind")) {
            command.setCheckNormality(false);
        }
        command.setType(Command::CommandType::Select);
    }
}
