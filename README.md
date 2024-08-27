# KDocker

KDocker will help you dock most applications to the system tray. Just point and
click!

All you need to do is start KDocker and select an application using the mouse
and the application gets docked into the system tray.


## Desktop Environment Behavior

Each desktop environment has slightly different behavior in how they handle the
system try. Depending on which desktop environment you're using KDocker might
work differently than you expect. The behavior of the system try is defined by
the desktop environment and KDocker has no control over these differences in
behavior.

Desktop | Left Click  | Right Click            | Double Click | Scroll Wheel
------- | ----------- | ---------------------- | ------------ | ------------
KDE     | show / hide | opens menu             | show / hide  | nothing
Gnome   | opens menu  | opens menu             | show / hide  | nothing
Mate    | opens menu  | opens Mate applet menu | opens menu   | nothing
LXDE    | show / hide | opens menu             | show / hide  | show / hide
XFCE    | show / hide | opens menu             | show / hide  | nothing

### Notes

Gnome, not all distros install the system tray plugin by default.


## DBus Interface

A DBus interface is available at `com.kdocker.KDocker/manage` and allows
performing various actions with KDocker. The interface can be used to script
working with KDocker.

### Docking methods

Two versions of every method are provided with one being a simple command
and the other supporting extended attributes for fine tuned control. DBus
does not allow default values to multiple versions of the methods with
different options is required.

Method           | input                                                                        | output
---------------- | ---------------------------------------------------------------------------- | ------
dockWindowTitle  | (s pattern)                                                                  | ()
dockWindowTitle  | (s pattern, u timeout, b checkNormality, a{ss} windowConfig)                 | ()
dockLaunchApp    | (s app, as args, s pattern)                                                  | ()
dockLaunchApp    | (s app, as args, s pattern, u timeout, b checkNormality, a{ss} windowConfig) | ()
dockWindowId     | (u windowId)                                                                 | (b found)
dockWindowId     | (u windowId, a{ss} windowConfig)                                             | (b found)
dockPid          | (i pid)                                                                      | (b found)
dockPid          | (i pid, b checkNormality, a{ss} windowConfig)                                | (b found)
dockSelectWindow | ()                                                                           | ()
dockSelectWindow | (b checkNormality, a{ss} windowConfig)                                       | ()
dockFocused      | ()                                                                           | ()
dockFocused      | (a{ss} windowConfig)                                                         | ()

### pattern

Pattern is a PCRE regular expression.

If `pattern` matching on the window name is not wanted with `dockLaunchApp` use `""` as the value.

#### windowConfig

Tray item options correspond to the options seen in the `options` menu. It is a list of
dictionary references. Keys and value are both strings.

Key                | value
------------------ | -----
icon               | file path
attention-icon     | file path
iconify-focus-lost | true / false
iconify-minimized  | true / false
iconify-obscured   | true / false
notify-time        | seconds
quiet              | true / false
skip-pager         | true / false
sticky             | true / false
skip-taskbar       | true / false

Invalid keys are ignored.

Entries in the dictionary are options and only need to be provided if desired. However,
DBus does not allow empty dictionary parameters. If no settings are desired either send
one with the default value or send a key that isn't valid with any value. E.g. `{ "a", "b" }`.

### Docked window management

Method       | input        | output
------------ | ------------ | ------
listWindows  | ()           | (a{us} windows)
closeWindow  | (u windowId) | (b found)
undockWindow | (u windowId) | (b found)
showWindow   | (u windowId) | (b found)
hideWindow   | (u windowId) | (b found)
undockAll    | ()           | ()

### Behavior

Method      | input | output
----------- | ----- | ------
keepRunning | ()    | ()
quit        | ()    | ()

### Auto start

KDocker installs itself as a DBus auto start service using DBus' service activation
functionality. If a message for KDocker is sent to the session bus and KDocker is
not running, DBus will auto start KDocker.

Not all systems support DBus service activation and will auto start KDocker.
The `--keep-running` command line flag is provided to prevent KDocker from closing when
no windows are docked. This is to ensure KDocker will always be running to respond
to DBus requests. Useful if scripting KDocker via DBus. This is not necessary if only
using KDocker via running `kdocker`.

### Cli Examples

Examples of interacting with KDocker via DBus using the `dbus-send` utility.

```sh
dbus-send --session --print-reply --type=method_call --dest=com.kdocker.KDocker /manage com.kdocker.KdockerInterface.dockSelectWindow
dbus-send --session --print-reply --type=method_call --dest=com.kdocker.KDocker /manage com.kdocker.KdockerInterface.listWindows
dbus-send --session --print-reply --type=method_call --dest=com.kdocker.KDocker /manage com.kdocker.KdockerInterface.hideWindow uint32:73400330
dbus-send --session --print-reply --type=method_call --dest=com.kdocker.KDocker /manage com.kdocker.KdockerInterface.dockWindowTitle string:kcalc uint32:4 boolean:true dict:string:string:"iconify-focus-lost","true"
```


## Project home

https://github.com/user-none/KDocker


## Window system

KDocker requires X11 and cannot dock Wayland windows. It is impossible to and
KDocker can never work with Wayland. This is due to Wayland's security model
and outside of KDocker's control

Some distros default starts the desktop using Wayland. In order to us KDocker,
the desktop must be started under X11 instead of Wayland. Please consult
your distro's documentation on how to make this change.


## Packages

Snap and Flatpak packages are available but due to design decisions surrounding the package
systems, KDocker has reduced functionality when installed using either of these.

### Snap

Snap isolates applications and limits system access. In order to support
application launching, the KDocker snap is built with `classic` confinement.
Otherwise, KDocker would not have access outside of it's isolation to start
other applications.

Snap discourages this level, but it is desirable to have full functionality.
In the future, most likely, the confinement level will be changed to `strict`
when building the KDocker snap. At that time the application launching
functionality will no longer work if using Snap.

DBus auto start does not function because Snap does not currently support this
with applications that use the session bus. Start KDocker with the
`--keep-running` option in order to keep KDocker accessible via DBus if no
windows are docked.

### Flatpak

Flatpak isolates applications and limits system access in much the same way as
Snap. Due to this, the KDocker flatpak package cannot launch other
applications. There is not currently a work around in Flatpak like there is with Snap.

KDocker will not stay running with the `--keep-running` option after all
windows are undocked. However, DBus auto start will start KDocker as needed,
reducing the need for this option.


## Building

### From source

KDocker requires Qt 6. Unlike the name implies, it does not use any libraries
from KDE nor does KDE need to be installed.

Build dependencies for Ubuntu 24.04

- build-essential
- cmake
- ninja-build
- qt6-base-dev
- libx11-dev
- libxcb1-dev

Building

```sh
$ mkdir build
$ cd build
$ cmake -G Ninja ..
$ ninja
```

*IMPORTANT*: Close all previous instances of KDocker that are running before running
a new build. KDocker is a single instance application.

### Snap

The `snapcraft.yaml` file is required to be at the root of the project.
It is not there by default to keep the source archive uncluttered. However,
it will need to be moved there before a Snap package can be built.

The following will build, install, and run the snap.

```sh
$ ln -s resources/snap/snapcraft.yaml snapcraft.yaml
$ snapcraft
$ sudo snap install --dangerous --classic kdocker*.snap
$ snap run kdocker
```

Installation requires the `--dangerous` flag because the Snap
is not signed. The `--classic` app is required because of the
Snap using `classic` confinement.

Uninstall

```sh
$ sudo snap remove kdocker
```

### Flatpak

When building Flatpak packages the default output is not a `.flatpak` file. Instead
the intended behavior is to upload a build to a repo. However, the following will
build, produce a `.flatpak` file, install, and run KDocker.

```sh
$ mkdir build-flatpak
$ flatpak-builder --force-clean --disable-rofiles-fuse build-flatpak/ resources/flatpak/com.kdocker.kdocker.yml
$ flatpak build-export export build-flatpak/
$ flatpak build-bundle export kdocker.flatpak com.kdocker.KDocker
$ flatpak install --user kdocker.flatpak
$ flatpak run com.kdocker.KDocker
```

Uninstall

```sh
$ flatpak remove --user com.kdocker.KDocker
```
