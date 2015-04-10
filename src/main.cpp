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

#include <QCoreApplication>
#include <QLocale>
#include <QObject>
#include <QTranslator>

#include <signal.h>

#include "application.h"
#include "constants.h"
#include "kdocker.h"


static void sighandler(int sig) {
    Q_UNUSED(sig);

    dynamic_cast<Application*> (qApp)->close();
}

int main(int argc, char *argv[]) {
    Application app(Constants::APP_NAME, argc, argv);

    // setup signal handlers that undock and quit
    signal(SIGHUP, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGUSR1, sighandler);

    // Setup the translator
    QTranslator translator;
    QString locale = QString("kdocker_%1").arg(QLocale::system().name());
    if (!translator.load(locale, TRANSLATIONS_PATH)) {
        if (!translator.load(locale, "./build/i18n/")) {
            translator.load(locale, "./i18n/");
        }
    }
    app.installTranslator(&translator);

    app.setOrganizationName(Constants::ORG_NAME);
    app.setOrganizationDomain(Constants::DOM_NAME);
    app.setApplicationName(Constants::APP_NAME);
    app.setApplicationVersion(Constants::APP_VERSION);
    // Quitting will be handled by the TrayItemManager in the KDocker instance.
    // It will determine when there is nothing left running.
    app.setQuitOnLastWindowClosed(false);

    KDocker kdocker;
    // This can exit the application. We want the output of any help text output
    // on the tty the instance has started from so we call this here.
    kdocker.preProcessCommand(argc, argv);

    // Send the arguments in a message to another instance if there is one.
    if (app.sendMessage(QCoreApplication::arguments().join("\n"))) {
        return 0;
    }

    // Handle messages from another instance so this can be a single instance app.
    QObject::connect(&app, SIGNAL(messageReceived(const QString&)), &kdocker, SLOT(handleMessage(const QString&)));

    // Wait for the Qt event loop to be started before running.
    QMetaObject::invokeMethod(&kdocker, "run", Qt::QueuedConnection);

    app.setKDockerInstance(&kdocker);

    return app.exec();
}
