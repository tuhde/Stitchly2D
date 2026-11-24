message("Entering guider.pro")

# File with common stuff for whole project
include(../../../common.pri)

# The Seamly2D libraries behind the pattern loader need multimedia (sound effects) and print support; network is not needed.
QT += core gui widgets xml printsupport svg multimedia

TARGET = guider
TEMPLATE = app
DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
RCC_DIR = rcc
UI_DIR = uic

include(guider.pri)

# German etc.: the same share/translations/stitchly2d_<locale>.ts as the editor (guider's strings are collected there too)
include(../stitchly2d/translations.pri)

include(../stitchly2d/warnings.pri)

# precompiled headers clash with the BUILD_REVISION define, thus disable here
CONFIG -= precompile_header

unix:!macx {
    QMAKE_RPATHDIR += $$[QT_INSTALL_LIBS]
    isEmpty(PREFIX): PREFIX = /usr/local
    target.path = $$PREFIX/bin
    INSTALLS += target

    # Desktop entry for the application menu; the autostart and systemd files are in share/guider (see docs/GUIDER_PI.md)
    desktop.path = $$PREFIX/share/applications
    desktop.files = $$PWD/../../../share/guider/guider.desktop
    INSTALLS += desktop
}

# Static libraries. Several of them refer to each other, so link them as a group.
LIBNAMES = vmcp tools vtools vwidgets vformat vpatterndb fervor ifc vmisc vgeometry vlayout qmuparser vpropertyexplorer vobj vdxf stitchload stitchcore yarnui
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
