TEMPLATE = app
CONFIG  += qt
QT      += widgets dbus core
TARGET   = kdocker

INCLUDEPATH += . src /usr/include/X11
LIBS = -lX11 -lXmu -lxcb

# Application version that will be used any place the version is shown
VERSION = 5.99

# Build man page from pod
MANPODS += resources/man/kdocker.pod
man.input = MANPODS
man.output = build/man/${QMAKE_FILE_BASE}.1
man.CONFIG = no_link target_predeps
man.path = /usr/share/man/man1/
man.commands = pod2man --center \"General Commands Manual\" --release \"Version $${VERSION}\" --date \"`date +\'%e %B, %Y\'`\" ${QMAKE_FILE_NAME} ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += man

# Install icons
icons.path = /usr/share/icons/hicolor/128x128/apps
icons.files = resources/images/kdocker.png

# Install desktop file
desktop.path = /usr/share/applications
desktop.files = resources/kdocker.desktop

# Build the appdata xml
APPDATA += resources/appdata/kdocker.appdata.xml.in
appdata.input = APPDATA
appdata.output = build/appdata/${QMAKE_FILE_BASE}
appdata.CONFIG = no_link target_predeps
appdata.path = /usr/share/metainfo
appdata.commands = $$QMAKE_STREAM_EDITOR -e s/@VERSION@/$${VERSION}/g -e s/@TIMESTAMP@/`date +%s`/g ${QMAKE_FILE_NAME} > ${QMAKE_FILE_OUT}
QMAKE_EXTRA_COMPILERS += appdata

# Install bash completions
completion.path = /usr/share/bash-completion/completions
completion.files = resources/bash-completion/kdocker

# Application install location
target.path = /usr/bin

# Every target that should be installed
INSTALLS += target icons appdata desktop completion man

# Make the version available to to source
DEFINES += VERSION=\\\"$${VERSION}\\\"

# Input
DBUS_ADAPTORS += resources/dbus/kdocker.xml

HEADERS += src/application.h \
           src/command.h \
           src/commandlineargs.h \
           src/constants.h \
           src/scanner.h \
           src/trayitem.h \
           src/trayitemconfig.h \
           src/trayitemmanager.h \
           src/xlibutil.h
SOURCES += src/application.cpp \
           src/command.cpp \
           src/commandlineargs.cpp \
           src/constants.cpp \
           src/main.cpp \
           src/scanner.cpp \
           src/trayitem.cpp \
           src/trayitemconfig.cpp \
           src/trayitemmanager.cpp \
           src/xlibutil.cpp

RESOURCES += resources/resources.qrc

# Output
MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build
DESTDIR = .
