QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

QMAKE_TARGET_COPYRIGHT = "\\251 Wouldy Mods 2026"
VERSION = 2.1.2.17

INCLUDEPATH += $$PWD/include
LIBS += -L$$PWD/lib/
LIBS += -lfmod64
LIBS += -lfsbank64

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    about.cpp \
    bank_extract.cpp \
    extract_worker.cpp \
    fileio.cpp \
    fmod_fsb_list.cpp \
    main.cpp \
    mainwindow.cpp \
    rebuild_worker.cpp

HEADERS += \
    about.h \
    bank_extract.h \
    bank_header.h \
    extract_worker.h \
    fileio.h \
    fmod_fsb_list.h \
    mainwindow.h \
    rebuild_worker.h \
    version.h

FORMS += \
    about.ui \
    fmod_fsb_list.ui \
    mainwindow.ui

TRANSLATIONS += \
    Fmod_Bank_Tools_en_AU.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rc.qrc

RC_ICONS = resource\fmod_bank_tools.ico
