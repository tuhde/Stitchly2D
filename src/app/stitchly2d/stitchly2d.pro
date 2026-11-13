message("Entering stitchly2d.pro")

# File with common stuff for whole project
include(../../../common.pri)

QT += core gui widgets network xml printsupport svg multimedia

TARGET = stitchly2d
TEMPLATE = app
DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
RCC_DIR = rcc
UI_DIR = uic

include(stitchly2d.pri)

include(warnings.pri)

# precompiled headers clash with the BUILD_REVISION define, thus disable here
CONFIG -= precompile_header

DVCS_HESH=$$FindBuildRevision()
DEFINES += "BUILD_REVISION=$${DVCS_HESH}"

unix:!macx {
    QMAKE_RPATHDIR += $$[QT_INSTALL_LIBS]
    isEmpty(PREFIX): PREFIX = /usr/local
    target.path = $$PREFIX/bin
    INSTALLS += target
}

# Static libraries. Several of them refer to each other, so link them as a group.
LIBNAMES = vmcp tools vtools vwidgets vformat vpatterndb fervor ifc vmisc vgeometry vlayout qmuparser vpropertyexplorer vobj vdxf stitchcore
unix: LIBS += -Wl,--start-group
for(lib, LIBNAMES) {
    LIBS += -L$$OUT_PWD/../../libs/$${lib}/$${DESTDIR}/ -l$${lib}
    INCLUDEPATH += $$PWD/../../libs/$${lib}
    DEPENDPATH += $$PWD/../../libs/$${lib}
    PRE_TARGETDEPS += $$OUT_PWD/../../libs/$${lib}/$${DESTDIR}/lib$${lib}.a
}
INCLUDEPATH += $$OUT_PWD/../../libs/vtools/$${UI_DIR} # for UI files of vtools
unix: LIBS += -Wl,--end-group

# xerces library (Linux: system package)
LIBS += -lxerces-c
