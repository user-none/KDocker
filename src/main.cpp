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

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QLocale>
#include <QObject>
#include <QTranslator>

#include "application.h"
#include "constants.h"
#include "kdocker.h"

#include <signal.h>

KDocker *kdocker = 0;

static void sighandler(int sig) {
    Q_UNUSED(sig);

    if (kdocker) {
        kdocker->undockAll();
    }
    //((KDocker *) qApp)->undockAll();
    ::exit(0);
}

int main(int argc, char *argv[]) {
    // setup signal handlers that undock and quit
    signal(SIGHUP, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGUSR1, sighandler);


    Application app("KDocker", argc, argv);

    // Setup Translator
    QTranslator translator;
    QString locale = QString("kdocker_%1").arg(QLocale::system().name());
    if (!translator.load(locale, TRANSLATIONS_PATH)) {
        if (!translator.load(locale, "./build/i18n/")) {
            translator.load(locale, "./i18n/");
        }
    }
    app.installTranslator(&translator);

    app.setOrganizationName(ORG_NAME);
    app.setOrganizationDomain(DOM_NAME);
    app.setApplicationName(APP_NAME);
    app.setApplicationVersion(APP_VERSION);
    //app.setQuitOnLastWindowClosed(false);
    
    kdocker = new KDocker();
    kdocker->preProcessCommand(argc, argv); // this can exit the application

    // Send the arguments in a message to another instance if there is one.
    if (app.sendMessage(QCoreApplication::arguments().join("\n"))) {
        return 0;
    }

    QObject::connect(&app, SIGNAL(messageReceived(const QString&)), kdocker, SLOT(handleMessage(const QString&)));
    // Run in the Qt event loopt.
    QMetaObject::invokeMethod(kdocker, "run", Qt::QueuedConnection);

    app.setKDockerInstance(kdocker);

    return app.exec();
}
