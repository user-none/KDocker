TEMPLATE = app
CONFIG += qt debug
TARGET = kdocker

DEPENDPATH += src
INCLUDEPATH += . src  /usr/include/X11
LIBPATH += /usr/X11/lib
LIBS = -lX11 -lXpm -lXmu

INSTALL_PATH = /usr/local/share/kdocker

ICONS_PATH = $$INSTALL_PATH/icons
icons.path = $$ICONS_PATH
icons.files = resources/images/kdocker.png

desktop.path = /usr/share/applications
desktop.files = helpers/kdocker.desktop

completion.path = /etc/bash_completion.d
completion.file = helpers/kdocker

target.path = /usr/local/bin

INSTALLS += target icons desktop completion

# Input
HEADERS += src/constants.h \
           src/kdocker.h \
           src/scanner.h \
           src/trayitem.h \
           src/trayitemmanager.h \
           src/util.h
SOURCES += src/kdocker.cpp \
           src/main.cpp \
           src/scanner.cpp \
           src/trayitem.cpp \
           src/trayitemmanager.cpp \
           src/util.cpp

RESOURCES += resources/resources.qrc
#TRANSLATIONS += i18n/kdocker_en.ts

# Output
MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build
DESTDIR = .
