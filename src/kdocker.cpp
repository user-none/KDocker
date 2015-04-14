/*
 *  Copyright (C) 2009, 2012 John Schember <john@nachtimwald.com>
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

#include <QCoreApplication>
#include <QTextStream>

#include "constants.h"
#include "kdocker.h"
#include "trayitemmanager.h"

#include <getopt.h>

#include <Xlib.h>


#define ARG_MAX_LEN 30
#define ARG_PRE_PAD 2
#define ARG_POST_PAD 2
#define MAX_HELP_LINE_LEN 79

KDocker::KDocker() {
    m_trayItemManager = new TrayItemManager();
}

KDocker::~KDocker() {
    if (m_trayItemManager) {
        delete m_trayItemManager;
        m_trayItemManager = 0;
    }
}

void KDocker::undockAll() {
    if (m_trayItemManager) {
        m_trayItemManager->undockAll();
    }
}

void KDocker::run() {
    if (m_trayItemManager) {
        m_trayItemManager->processCommand(QCoreApplication::arguments());
    }
}

void KDocker::handleMessage(const QString &args) {
    if (m_trayItemManager) {
        m_trayItemManager->processCommand(args.split("\n"));
    }
}

/*
 * handle arguments that output information to the user. We want to handle
 * them here so they are printed on the tty that the application is run from.
 * If we left it up to TrayItemManager with the rest of the arguments these
 * would be printed on the tty the instance was started on not the instance the
 * user is calling from.
 */
void KDocker::preProcessCommand(int argc, char **argv) {
    int option;
    optind = 0; // initialise the getopt static
    while ((option = getopt(argc, argv, Constants::OPTIONSTRING)) != -1) {
        switch (option) {
            case '?':
                printUsage();
                ::exit(1);
                break;
            case 'a':
                printAbout();
                ::exit(0);
                break;
            case 'h':
                printHelp();
                ::exit(0);
                break;
            case 'u':
                printUsage();
                ::exit(0);
                break;
            case 'v':
                printVersion();
                ::exit(0);
                break;
        }
    }
}

// It would be much faster to just format help args appropriately but that would
// require that translators put new lines in the translation. We just run the args
// through this function to format the text properly so they don't have to worry
// about it.
QString KDocker::formatHelpArgs(QList<QPair<QString, QString> > commands) {
    int padding = 0;
    QString out;

    // Find the longest arg below max length.
    for (int i = 0; i < commands.count(); ++i) {
        int length = commands.at(i).first.length();
        if (length > padding && length < ARG_MAX_LEN) {
            padding = length;
        }
    }
    // Pad spaces before and after the longest arg.
    padding += ARG_PRE_PAD + ARG_POST_PAD;

    // Build a formatted string from the arg and its description.
    for (int i = 0; i < commands.count(); ++i) {
        QString arg = commands.at(i).first;
        QString desc = commands.at(i).second;
        
        // Add the arg to the output. If it's over our max arg length
        // start the description on a new line.
        out += QString(' ').repeated(ARG_PRE_PAD);
        if (arg.length() < ARG_MAX_LEN) {
            out += arg.leftJustified(padding - ARG_PRE_PAD, ' ');
        } else {
            out += arg + "\n";
            out += QString(' ').repeated(padding);
        }

        int desc_len = desc.length();
        int last_offset = 0;
        int offset = 0;
        // Move forward in the string then back until we hit a space to break at.
        // If we don't find a space in the segment force a split.
        while (offset < desc_len) {
            if (offset != 0) {
                out += QString(' ').repeated(padding);
            }

            offset += MAX_HELP_LINE_LEN - padding;
            if (offset < desc_len) {
                while (offset > last_offset && desc.at(offset) != ' ') {
                    offset--;
                }
                if (offset == last_offset) {
                    offset += MAX_HELP_LINE_LEN - padding;
                }
            } else {
                offset = desc_len;
            }

            out += desc.mid(last_offset, offset - last_offset) + '\n';
            // Skip any spaces because we're going to start on a new line.
            while (offset < desc_len && desc.at(offset) == ' ') {
                offset++;
            }
            last_offset = offset;
        }
    }

    return out;
}

void KDocker::printAbout() {
    QTextStream out(stdout);

    out << Constants::ABOUT_MESSAGE << endl;
}

void KDocker::printHelp() {
    QTextStream out(stdout);
    QList<QPair<QString, QString> > commands;

    out << tr("Usage: %1 [options] [command] -- [command options]").arg(qApp->applicationName().toLower()) << endl;
    out << tr("Docks any application into the system tray") << endl;
    out << endl;
    out << tr("Command") << endl;
    out << tr("Run command and dock window") << endl;
    out << tr("Use -- after command to specify options for command") << endl;
    out << endl;
    out << tr("Options") << endl;

    commands.append(qMakePair(QString("-a"),      tr("Show author information")));
    commands.append(qMakePair(QString("-b"),      tr("Don't warn about non-normal windows (blind mode)")));
    commands.append(qMakePair(QString("-d secs"), tr("Maximum time in seconds to allow for command to start and open a window (defaults to 5 sec)")));
    commands.append(qMakePair(QString("-e type"), tr("Name matting syntax. Choices are 'n', 'r', 'u', 'w', 'x'. n = normal, substring matching (default). r = regex. u = unix wildcard. w = wildcard. x = W3C XML Schema 1.1.")));
    commands.append(qMakePair(QString("-f"),      tr("Dock window that has focus (active window)")));
    commands.append(qMakePair(QString("-h"),      tr("Display this help")));
    commands.append(qMakePair(QString("-i file"), tr("Set the tray icon to file")));
    commands.append(qMakePair(QString("-j"),      tr("Case senstive name (title) matching")));
    commands.append(qMakePair(QString("-k"),      tr("Regex minimal matching")));
    commands.append(qMakePair(QString("-l"),      tr("Iconify when focus lost")));
    commands.append(qMakePair(QString("-m"),      tr("Keep application window showing (don't hide on dock)")));
    commands.append(qMakePair(QString("-n name"), tr("Match window based on window title")));
    commands.append(qMakePair(QString("-o"),      tr("Iconify when obscured")));
    commands.append(qMakePair(QString("-p secs"), tr("Set ballooning timeout (popup time)")));
    commands.append(qMakePair(QString("-q"),      tr("Disable ballooning title changes (quiet)")));
    commands.append(qMakePair(QString("-r"),      tr("Remove this application from the pager")));
    commands.append(qMakePair(QString("-s"),      tr("Make the window sticky (appears on all desktops)")));
    commands.append(qMakePair(QString("-t"),      tr("Remove this application from the taskbar")));
    commands.append(qMakePair(QString("-v"),      tr("Display version")));
    commands.append(qMakePair(QString("-w wid"),  tr("Window id of the application to dock. Assumes hex number of the form 0x###...")));
    commands.append(qMakePair(QString("-x pid"),  tr("Process id of the application to dock. Assumes decimal number of the form ###...")));
    out << formatHelpArgs(commands);

    out << endl;
    out << tr("Bugs and wishes to https://github.com/user-none/KDocker") << endl;
    out << tr("Project information at https://github.com/user-none/KDocker") << endl;
}

void KDocker::printUsage() {
    QTextStream out(stdout);
    out << tr("Usage: %1 [options] command").arg(qApp->applicationName().toLower()) << endl;
    out << tr("Try `%1 -h' for more information").arg(qApp->applicationName().toLower()) << endl;
}

void KDocker::printVersion() {
    QTextStream out(stdout);
    out << tr("%1 version: %2").arg(qApp->applicationName()).arg(qApp->applicationVersion()) << endl;
    out << tr("Using Qt version: %1").arg(qVersion()) << endl;
}
