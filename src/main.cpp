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

#include "adaptor.h"
#include "application.h"
#include "commandlineargs.h"
#include "constants.h"
#include "kdocker_adaptor.h"
#include "trayitemmanager.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QLocale>
#include <QObject>

#include <signal.h>

static void sighandler([[maybe_unused]] int sig)
{
    dynamic_cast<Application *>(qApp)->notifyCloseSignal();
}

static bool setupDbus(TrayItemManager *trayitemmanager)
{
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

static void sendDbusCommand(const Command &command, const TrayItemOptions &config, bool daemon)
{
    QDBusInterface iface(Constants::DBUS_NAME, Constants::DBUS_PATH);
    if (!iface.isValid()) {
        qCritical() << "Could not create DBus interface for messaging other instance";
        ::exit(1);
    }

    // Non command command
    if (daemon)
        iface.call(QDBus::NoBlock, "daemonize");

    switch (command.getType()) {
    case Command::Type::NoCommand:
        break;
    case Command::Type::Title:
        iface.call(QDBus::NoBlock, "dockWindowTitle", command.getSearchPattern(), command.getTimeout(),
                   command.getCheckNormality(), QVariant::fromValue(config));
        break;
    case Command::Type::Launch:
        iface.call(QDBus::NoBlock, "dockLaunchApp", command.getLaunchApp(), command.getLaunchAppArguments(),
                   command.getSearchPattern(), command.getTimeout(), command.getCheckNormality(),
                   QVariant::fromValue(config));
        break;
    case Command::Type::WindowId:
        iface.call(QDBus::NoBlock, "dockWindowId", command.getWindowId(), QVariant::fromValue(config));
        break;
    case Command::Type::Pid:
        iface.call(QDBus::NoBlock, "dockPid", command.getPid(), command.getCheckNormality(),
                   QVariant::fromValue(config));
        break;
    case Command::Type::Select:
        if (daemon) {
            break;
        }
        iface.call(QDBus::NoBlock, "dockSelectWindow", command.getCheckNormality(), QVariant::fromValue(config));
        break;
    case Command::Type::Focused:
        iface.call(QDBus::NoBlock, "dockFocused", QVariant::fromValue(config));
        break;
    default:
        qFatal("COMMAND ERROR!!!!");
    }
}

static void registerTypes()
{
    qRegisterMetaType<WindowNameMap>("WindowNameMap");
    qRegisterMetaType<TrayItemOptions>("TrayItemOptions");
    qDBusRegisterMetaType<TrayItemOptions>();
    qDBusRegisterMetaType<WindowNameMap>();
}

int main(int argc, char *argv[])
{
    // Register all our meta types so they're available
    registerTypes();

    XLibUtil::silenceXErrors();

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
    TrayItemOptions config;
    bool daemon = false;
    if (!CommandLineArgs::processArgs(app.arguments(), command, config, daemon))
        return 1;

    TrayItemManager trayItemManager;
    app.setTrayItemManagerInstance(&trayItemManager);

    // Setup Dbus so we'll only have 1 instance running
    bool dbus_registered = setupDbus(&trayItemManager);
    // Send the requested action through DBus regardless if this is the only instance.
    sendDbusCommand(command, config, daemon);

    // Can't register dbus means another instance already has. Requests
    // were handled by the other instance and there is nothing more for us to do.
    if (!dbus_registered)
        return 0;

    return app.exec();
}
