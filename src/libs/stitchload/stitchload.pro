message("Entering stitchload.pro")

# File with common stuff for whole project
include(../../../common.pri)

# Loads Seamly2D patterns without any window (used by the editor and by guider).
QT += core gui widgets xml printsupport svg

TARGET = stitchload
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= precompile_header

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj

SOURCES += \
    core/application_2d.cpp \
    core/pattern_loader.cpp \
    xml/vpattern.cpp

HEADERS += \
    core/application_2d.h \
    core/headless_window.h \
    core/pattern_loader.h \
    xml/vpattern.h

include(../../app/stitchly2d/warnings.pri)

include(../libs.pri)
