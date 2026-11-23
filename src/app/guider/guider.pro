message("Entering guider.pro")

# File with common stuff for whole project
include(../../../common.pri)

# Only what the pattern loader needs; no multimedia, network or print support of our own.
QT += core gui widgets xml printsupport svg network multimedia

TARGET = guider
TEMPLATE = app
DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
RCC_DIR = rcc
UI_DIR = uic

SOURCES += \
    main.cpp \
    guide_window.cpp \
    step_view.cpp \
    open_dialog.cpp \
    guide_scale.cpp

HEADERS += \
    guide_window.h \
    step_view.h \
    open_dialog.h \
    guide_scale.h

include(../stitchly2d/warnings.pri)

# precompiled headers clash with the BUILD_REVISION define, thus disable here
CONFIG -= precompile_header

unix:!macx {
    QMAKE_RPATHDIR += $$[QT_INSTALL_LIBS]
    isEmpty(PREFIX): PREFIX = /usr/local
    target.path = $$PREFIX/bin
    INSTALLS += target
}

# Static libraries. Several of them refer to each other, so link them as a group.
LIBNAMES = vmcp tools vtools vwidgets vformat vpatterndb fervor ifc vmisc vgeometry vlayout qmuparser vpropertyexplorer vobj vdxf stitchload stitchcore
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
