# ADD TO EACH PATH $$PWD VARIABLE!!!!!!

HEADERS += \
    $$PWD/mcp_integration.h \
    $$PWD/mcp_panel.h \
    $$PWD/stitch_tools.h

SOURCES += \
    $$PWD/mcp_integration.cpp \
    $$PWD/mcp_panel.cpp \
    $$PWD/stitch_tools.cpp

# The embedded "Claude shell" terminal panel needs QTermWidget (Qt 6). It is optional: without it the panel is
# simply not built. Install e.g. libqtermwidget-dev (Debian/Ubuntu) and re-run qmake.
unix:!macx {
    CONFIG += link_pkgconfig
    packagesExist(qtermwidget6) {
        PKGCONFIG += qtermwidget6
        DEFINES += STITCHLY_HAVE_QTERMWIDGET
        HEADERS += $$PWD/claude_shell_panel.h
        SOURCES += $$PWD/claude_shell_panel.cpp
        message("Claude shell panel: enabled (QTermWidget found)")
    } else {
        message("Claude shell panel: disabled (QTermWidget not found, install libqtermwidget-dev)")
    }
}
