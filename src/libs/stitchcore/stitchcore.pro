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
    project.h \
    pieceoutline.h \
    patternfile.h \
    stitchgrid.h \
    shaping.h \
    instructions.h \
    passes.h

SOURCES += \
    stitchsize.cpp \
    stitchtype.cpp \
    catalog.cpp \
    project.cpp \
    pieceoutline.cpp \
    patternfile.cpp \
    stitchgrid.cpp \
    shaping.cpp \
    instructions.cpp \
    passes.cpp
