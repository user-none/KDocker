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

#include <QDBusMetaType>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusError>

#include <signal.h>

#include "application.h"
#include "constants.h"
#include "trayitemmanager.h"
#include "commandlineargs.h"


static void sighandler([[maybe_unused]] int sig) {
    dynamic_cast<Application*> (qApp)->notifyCloseSignal();
}

static bool setupDbus(TrayItemManager *trayitemmanager) {
    auto connection = QDBusConnection::sessionBus();
    if (!connection.isConnected()) {
        qCritical() << "Cannot connect to the D-Bus session bus";
        ::exit(1);
    }

    bool registered = connection.registerService(Constants::DBUS_NAME);
    if (registered) {
        new KdockerInterfaceAdaptor(trayitemmanager);
        connection.registerObject(Constants::DBUS_PATH, trayitemmanager);
    }

   return registered;
}

static void sendDbusCommand(const Command &command, const TrayItemConfig &config, bool daemon) {
    QDBusInterface iface(Constants::DBUS_NAME, Constants::DBUS_PATH);
    if (!iface.isValid()) {
        qCritical() << "Could not create DBus interface for messaging other instance";
        ::exit(1);
    }

    if (daemon)
        iface.call(QDBus::NoBlock, "daemonize");

    switch (command.getType()) {
        case Command::CommandType::Select:
            iface.call(QDBus::NoBlock, "selectWindow", command.getCheckNormality(), QVariant::fromValue(config));
            break;
    }

    // Tell the other instance what the caller wants to do.
    //iface.call(QDBus::NoBlock, "cmd", QCoreApplication::arguments().sliced(1));
    //::exit(0);

}

int main(int argc, char *argv[]) {
    Application app(argc, argv);

    // setup signal handlers that undock and quit
    signal(SIGHUP, sighandler);
    signal(SIGSEGV, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);
    signal(SIGUSR1, sighandler);

    app.setOrganizationName(Constants::ORG_NAME);
    app.setOrganizationDomain(Constants::DOM_NAME);
    app.setApplicationName(Constants::APP_NAME);
    app.setApplicationVersion(Constants::APP_VERSION);

    // Quitting will be handled by the TrayItemManager in the KDocker instance.
    // It will determine when there is nothing left running.
    app.setQuitOnLastWindowClosed(false);

    // Parse the command line arguments so we know what to do
    Command command;
    TrayItemConfig config;
    bool daemon = false;
    if (!CommandLineArgs::processArgs(app.arguments(), command, config, daemon))
        return 1;

    TrayItemManager trayItemManager;

    qRegisterMetaType<TrayItemConfig>("TrayItemConfig");
    qDBusRegisterMetaType<TrayItemConfig>();

    // Setup Dbus so we'll only have 1 instance running
    if (!setupDbus(&trayItemManager)) {
        // Can't register means another instance already has. Any commands
        // were sent to that instance and there is nothing more for us to do.
        sendDbusCommand(command, config, daemon);
        return 0;
    }

    // Wait for the Qt event loop to be started before running.
    QMetaObject::invokeMethod(&trayItemManager, "processCommand", Qt::QueuedConnection,
            Q_ARG(const Command &, command),
            Q_ARG(const TrayItemConfig &, config));

    app.setTrayItemManagerInstance(&trayItemManager);

    return app.exec();
}
