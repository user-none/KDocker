TEMPLATE = app
CONFIG += qt debug
TARGET = kdocker

include(solutions/qtsingleapplication/qtsingleapplication.pri)

DEPENDPATH += src
INCLUDEPATH += . src  /usr/include/X11
LIBPATH += /usr/X11/lib
LIBS = -lX11 -lXpm -lXmu

INSTALL_PATH = /usr/local/share/kdocker

TRANSLATIONS += i18n/kdocker_it_IT.ts

isEmpty(QMAKE_LRELEASE) {
    QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
    !exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease-qt4 }
}

updateqm.input = TRANSLATIONS
updateqm.output = build/i18n/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$QMAKE_LRELEASE -silent ${QMAKE_FILE_IN} -qm build/i18n/${QMAKE_FILE_BASE}.qm
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm

TRANSLATIONS_PATH = $$INSTALL_PATH
DEFINES += TRANSLATIONS_PATH=\\\"$${TRANSLATIONS_PATH}/i18n\\\"
translations.path = $$TRANSLATIONS_PATH
translations.files = build/i18n

ICONS_PATH = $$INSTALL_PATH/icons
icons.path = $$ICONS_PATH
icons.files = resources/images/kdocker.png

desktop.path = /usr/share/applications
desktop.files = helpers/kdocker.desktop

completion.path = /etc/bash_completion.d
completion.files = helpers/kdocker

target.path = /usr/local/bin

INSTALLS += target icons desktop completion translations

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
