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
XFCE    | N/A         | N/A                    | N/A          | N/A

### Notes

Gnaome, not all distros install the system tray plugin by default.

XFCE, KDocker items do not show in the system tray.


## Website

https://github.com/user-none/KDocker


## Building from source

KDocker requires Qt 6. Unlike the name implies, it does not use any libraires
from KDE nor does KDE need to be installed.

Build dependencies for Ubuntu 24.04

- build-essential
- qt6-base-dev
- libx11-dev
- libxcb1-dev
- libxmu-dev

Building

1. `qmake6`
2. `make`
3. `make install` (optional)

*IMPORTANT*: Close all previous instances of KDocker that are running before running
a new build. KDocker is a single instance application.
