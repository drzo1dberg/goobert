QT += core gui widgets openglwidgets network

CONFIG += c++20
TARGET = goobert
TEMPLATE = app

# Version
DEFINES += GOOBERT_VERSION=\\\"1.0.0\\\"

# libmpv
CONFIG += link_pkgconfig
PKGCONFIG += mpv

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/mpvwidget.cpp \
    src/mpvcontroller.cpp \
    src/gridcell.cpp \
    src/controlpanel.cpp \
    src/filescanner.cpp

HEADERS += \
    src/mainwindow.h \
    src/mpvwidget.h \
    src/mpvcontroller.h \
    src/gridcell.h \
    src/controlpanel.h \
    src/filescanner.h

# Install
target.path = /usr/local/bin
INSTALLS += target
