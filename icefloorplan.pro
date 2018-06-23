lessThan(QT_MAJOR_VERSION, 5) {
    error("Qt $${QT_MAJOR_VERSION} is not supported.")
}

CONFIG  += c++11
QT      += core gui widgets

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
    circuitbuilder.cpp \
    floorplanbuilder.cpp

HEADERS += \
    floorplanwindow.h \
    floorplanwidget.h \
    chipdb.h \
    ascparser.h \
    bitstream.h \
    chipdbloader.h \
    bitstreamloader.h \
    circuitbuilder.h \
    floorplanbuilder.h

FORMS += \
    floorplanwindow.ui

RESOURCES += \
    builtins.qrc
