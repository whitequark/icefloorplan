CONFIG   += c++11

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = icefloorplan
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    floorplanwindow.cpp \
    floorplanwidget.cpp \
    chipdb.cpp \
    ascparser.cpp \
    bitstream.cpp \
    chipdbloader.cpp \
    bitstreamloader.cpp \
    circuitbuilder.cpp

HEADERS += \
    floorplanwindow.h \
    floorplanwidget.h \
    chipdb.h \
    ascparser.h \
    bitstream.h \
    chipdbloader.h \
    bitstreamloader.h \
    circuitbuilder.h

FORMS += \
    floorplanwindow.ui

RESOURCES += \
    builtins.qrc
