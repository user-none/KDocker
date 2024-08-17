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

#include "kdocker_adaptor.h"

#include <QCoreApplication>
#include <QLocale>
#include <QObject>
#include <QTranslator>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusError>

#include <signal.h>

#include "application.h"
#include "constants.h"
#include "kdocker.h"


static void sighandler(int sig) {
    Q_UNUSED(sig);

    dynamic_cast<Application*> (qApp)->notifyCloseSignal();
}

static void setupDbus(KDocker *kdocker) {
    auto connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) {
        qCritical() << "Cannot connect to the D-Bus session bus";
        ::exit(1);
    }

    // Can't register means another instance already has.
    if (!connection.registerService(Constants::DBUS_NAME)) {
        QDBusInterface iface(Constants::DBUS_NAME, Constants::DBUS_PATH);
        if (!iface.isValid()) {
            qCritical() << "Could not create DBus interface for messaging other instance";
            ::exit(1);
        }

        // Tell the other instance what the caller wants to do.
        iface.call(QDBus::NoBlock, "cmd", QCoreApplication::arguments().sliced(1));
        ::exit(0);
    }

    // Handle messages from another instance so this can be a single instance app.
    new KdockerInterfaceAdaptor(kdocker);
    connection.registerObject(Constants::DBUS_PATH, kdocker);
}

int main(int argc, char *argv[]) {
    //Application app(Constants::APP_NAME, argc, argv);
    Application app(argc, argv);

    // setup signal handlers that undock and quit
    signal(SIGHUP, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGUSR1, sighandler);

    // Setup the translator
    QTranslator translator;
    if (translator.load(QLocale::system().name(), ":/i18n"))
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

    // Setup Dbus so we'll only have 1 instance running. This can also exit
    // the application if KDocker is already running and the call is forwarded
    // to that instance.
    setupDbus(&kdocker);

    // Wait for the Qt event loop to be started before running.
    QMetaObject::invokeMethod(&kdocker, "cmd", Qt::QueuedConnection, Q_ARG(const QStringList &, QCoreApplication::arguments().sliced(1)));

    app.setKDockerInstance(&kdocker);

    return app.exec();
}
