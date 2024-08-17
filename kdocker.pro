TEMPLATE = app
CONFIG  += qt
QT      += widgets dbus core
TARGET   = kdocker

INCLUDEPATH += . src /usr/include/X11
LIBS = -lX11 -lXmu -lxcb

INSTALL_PATH = /usr/share/kdocker

DBUS_ADAPTORS += resources/dbus.kdocker.xml

TRANSLATIONS += resources/i18n/pl.ts

#updateqm.input = TRANSLATIONS
#updateqm.output = build/i18n/${QMAKE_FILE_BASE}.qm
#updateqm.commands = lrelease -silent ${QMAKE_FILE_IN} -qm build/i18n/${QMAKE_FILE_BASE}.qm
#updateqm.CONFIG += no_link target_predeps
#QMAKE_EXTRA_COMPILERS += updateqm

TRANSLATIONS_PATH = $$INSTALL_PATH
DEFINES += TRANSLATIONS_PATH=\\\"$${TRANSLATIONS_PATH}/i18n\\\"
#translations.path = $$TRANSLATIONS_PATH
#translations.files = build/i18n

MANPODS += helpers/kdocker.pod

man.name = "Compile man page"
man.input = MANPODS
man.output = build/man/${QMAKE_FILE_BASE}.1
man.CONFIG = no_link target_predeps
man.path = /usr/share/man/man1/
man.commands = helpers/kdocker_man.sh ${QMAKE_FILE_NAME} ${QMAKE_FILE_OUT} "5.99"
QMAKE_EXTRA_COMPILERS += man

icons.path = /usr/share/icons/hicolor/128x128/apps
icons.files = resources/images/kdocker.png

desktop.path = /usr/share/applications
desktop.files = helpers/kdocker.desktop

appdata.path = /usr/share/metainfo
appdata.files = helpers/appdata/kdocker.appdata.xml

completion.path = /usr/share/bash-completion/completions
completion.files = helpers/kdocker

target.path = /usr/bin

#INSTALLS += target icons desktop completion translations
INSTALLS += target icons appdata desktop completion man

# Input
HEADERS += src/application.h \
           src/constants.h \
           src/kdocker.h \
           src/scanner.h \
           src/trayitem.h \
           src/trayitemmanager.h \
           src/xlibutil.h
SOURCES += src/application.cpp \
           src/constants.cpp \
           src/kdocker.cpp \
           src/main.cpp \
           src/scanner.cpp \
           src/trayitem.cpp \
           src/trayitemmanager.cpp \
           src/xlibutil.cpp

RESOURCES += resources/resources.qrc

# Output
MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build
DESTDIR = .
