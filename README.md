# KDocker

KDocker will help you dock most applications to the system tray. Just point and
click!

All you need to do is start KDocker and select an application using the mouse
and the application gets docked into the system tray.

## Website

https://github.com/user-none/KDocker

## Building from source

KDocker requires Qt 6. Unlike the name implies, it does not use any libraires
from KDE nor does KDE need to be installed.

Build dependencies for Ubuntu 24.04

- build-essential
- qt6-base-dev
- libxcb1-dev
- libx11-xcb-dev
- libxmu-dev

Building

1. `qmake6`
2. `make`
3. `make install` (optional)

*IMPORTANT*: Close all previous instances of KDocker that are running before running
a new build. KDocker is a single instance application.
