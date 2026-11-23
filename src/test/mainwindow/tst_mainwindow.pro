message("Entering tst_mainwindow.pro")

include(../../../common.pri)

QT += core gui widgets network xml printsupport svg multimedia testlib

TARGET = tst_mainwindow
TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle precompile_header
DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
RCC_DIR = rcc
UI_DIR = uic

# The application's sources, without its main()
include(../../app/stitchly2d/stitchly2d.pri)
SOURCES ~= s,.*/main\.cpp$,,
SOURCES += tst_mainwindow.cpp

include(../../app/stitchly2d/warnings.pri)

DEFINES += TEST_DATA_DIR=\\\"$$PWD/../data\\\"

LIBNAMES = vmcp tools vtools vwidgets vformat vpatterndb fervor ifc vmisc vgeometry vlayout qmuparser vpropertyexplorer vobj vdxf stitchload stitchcore
unix: LIBS += -Wl,--start-group
for(lib, LIBNAMES) {
    LIBS += -L$$OUT_PWD/../../libs/$${lib}/bin/ -l$${lib}
    INCLUDEPATH += $$PWD/../../libs/$${lib}
    DEPENDPATH += $$PWD/../../libs/$${lib}
    PRE_TARGETDEPS += $$OUT_PWD/../../libs/$${lib}/bin/lib$${lib}.a
}
INCLUDEPATH += $$OUT_PWD/../../libs/vtools/uic
unix: LIBS += -Wl,--end-group
LIBS += -lxerces-c
