TEMPLATE = app
CONFIG  += qt 
TARGET   = kdocker

isEmpty(SYSTEMQTSA) {
	include(3rdparty/qtsingleapplication/src/qtsingleapplication.pri)
} else {
	CONFIG += qtsingleapplication
}

DEPENDPATH += src
INCLUDEPATH += . src  /usr/include/X11
QMAKE_LIBDIR += /usr/X11/lib
LIBS = -lX11 -lXmu -lxcb -lX11-xcb

INSTALL_PATH = /usr/share/kdocker

TRANSLATIONS += resources/i18n/pl.ts

isEmpty(QMAKE_LRELEASE) {
    QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
    !exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease-qt4 }
}

#updateqm.input = TRANSLATIONS
#updateqm.output = build/i18n/${QMAKE_FILE_BASE}.qm
#updateqm.commands = $$QMAKE_LRELEASE -silent ${QMAKE_FILE_IN} -qm build/i18n/${QMAKE_FILE_BASE}.qm
#updateqm.CONFIG += no_link target_predeps
#QMAKE_EXTRA_COMPILERS += updateqm

TRANSLATIONS_PATH = $$INSTALL_PATH
DEFINES += TRANSLATIONS_PATH=\\\"$${TRANSLATIONS_PATH}/i18n\\\"
#translations.path = $$TRANSLATIONS_PATH
#translations.files = build/i18n

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
INSTALLS += target icons appdata desktop completion

# Input
HEADERS += src/application.h \
           src/constants.h \
           src/kdocker.h \
           src/scanner.h \
           src/trayitem.h \
           src/trayitemmanager.h \
           src/xlibutil.h \
           src/xcbeventreceiver.h
SOURCES += src/application.cpp \
           src/constants.cpp \
           src/kdocker.cpp \
           src/main.cpp \
           src/scanner.cpp \
           src/trayitem.cpp \
           src/trayitemmanager.cpp \
           src/xlibutil.cpp \
           src/xcbeventreceiver.cpp

RESOURCES += resources/resources.qrc

# Output
MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build
DESTDIR = .
