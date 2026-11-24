message("Entering yarnbox.pro")

# File with common stuff for whole project
include(../../../common.pri)

# The yarn inventory on its own: no pattern loader and none of the Seamly2D libraries, so it starts fast and is small.
QT += core gui widgets

TARGET = yarnbox
TEMPLATE = app
DESTDIR = bin
MOC_DIR = moc
OBJECTS_DIR = obj
RCC_DIR = rcc

include(yarnbox.pri)

# German etc.: the same share/translations/stitchly2d_<locale>.ts as the other programs
include(../stitchly2d/translations.pri)

include(../stitchly2d/warnings.pri)

CONFIG -= precompile_header

unix:!macx {
    QMAKE_RPATHDIR += $$[QT_INSTALL_LIBS]
    isEmpty(PREFIX): PREFIX = /usr/local
    target.path = $$PREFIX/bin
    INSTALLS += target

    desktop.path = $$PREFIX/share/applications
    desktop.files = $$PWD/../../../share/yarnbox/yarnbox.desktop
    INSTALLS += desktop
}

LIBNAMES = yarnui stitchcore
for(lib, LIBNAMES) {
    LIBS += -L$$OUT_PWD/../../libs/$${lib}/$${DESTDIR}/ -l$${lib}
    INCLUDEPATH += $$PWD/../../libs/$${lib}
    DEPENDPATH += $$PWD/../../libs/$${lib}
    PRE_TARGETDEPS += $$OUT_PWD/../../libs/$${lib}/$${DESTDIR}/lib$${lib}.a
}
