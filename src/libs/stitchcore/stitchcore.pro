message("Entering stitchcore.pro")

# File with common stuff for whole project
include(../../../common.pri)

QT += core gui

TARGET = stitchcore
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= precompile_header

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj

HEADERS += \
    stitchsize.h \
    stitchtype.h \
    catalog.h \
    stitchgrid.h \
    shaping.h \
    instructions.h

SOURCES += \
    stitchsize.cpp \
    stitchtype.cpp \
    catalog.cpp \
    stitchgrid.cpp \
    shaping.cpp \
    instructions.cpp
