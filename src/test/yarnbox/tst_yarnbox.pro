message("Entering tst_yarnbox.pro")

include(../../../common.pri)

QT += core gui widgets testlib

TARGET = tst_yarnbox
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle precompile_header
DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj

# The program's sources, without its main()
include(../../app/yarnbox/yarnbox.pri)
SOURCES ~= s,.*/main\.cpp$,,
SOURCES += tst_yarnbox.cpp

LIBNAMES = yarnui stitchcore
for(lib, LIBNAMES) {
    LIBS += -L$$OUT_PWD/../../libs/$${lib}/bin/ -l$${lib}
    INCLUDEPATH += $$PWD/../../libs/$${lib}
    PRE_TARGETDEPS += $$OUT_PWD/../../libs/$${lib}/bin/lib$${lib}.a
}
