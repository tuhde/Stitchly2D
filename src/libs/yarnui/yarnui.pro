message("Entering yarnui.pro")

# File with common stuff for whole project
include(../../../common.pri)

# Widgets for the yarn inventory, shared by the editor and the yarn program.
QT += core gui widgets

TARGET = yarnui
TEMPLATE = lib
CONFIG += staticlib
CONFIG -= precompile_header

DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj

INCLUDEPATH += $$PWD/../stitchcore

HEADERS += \
    yarn_inventory_widget.h \
    yarn_picker.h

SOURCES += \
    yarn_inventory_widget.cpp \
    yarn_picker.cpp
