# ADD TO EACH PATH $$PWD VARIABLE!!!!!!

include(mcp/mcp.pri)

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/mainwindow.cpp \
    $$PWD/catalog_dialog.cpp \
    $$PWD/core/colors.cpp \
    $$PWD/core/cli.cpp

HEADERS += \
    $$PWD/mainwindow.h \
    $$PWD/catalog_dialog.h \
    $$PWD/core/colors.h \
    $$PWD/version.h \
    $$PWD/core/cli.h
