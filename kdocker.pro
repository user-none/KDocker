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
icons.files = icons/kdocker.png

desktop.path = /usr/share/applications
desktop.files = kdocker.desktop

target.path = /usr/local/bin

INSTALLS += target icons desktop

# Input
HEADERS += src/kdocker.h \
           src/trayitem.h \
           src/trayitemmanager.h \
           src/util.h
SOURCES += src/kdocker.cpp \
           src/main.cpp \
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
