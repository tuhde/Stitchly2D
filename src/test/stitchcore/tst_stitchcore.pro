include(../../../common.pri)
QT += testlib gui
CONFIG += console testcase
CONFIG -= app_bundle precompile_header
TEMPLATE = app
TARGET = tst_stitchcore
SOURCES += tst_stitchcore.cpp
DEFINES += TS_DIR=\\\"$$PWD/../../../share/translations\\\"
INCLUDEPATH += $$PWD/../../libs/stitchcore
unix: LIBS += -L$$OUT_PWD/../../libs/stitchcore/bin -lstitchcore
PRE_TARGETDEPS += $$OUT_PWD/../../libs/stitchcore/bin/libstitchcore.a
