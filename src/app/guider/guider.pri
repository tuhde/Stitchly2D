# ADD TO EACH PATH $$PWD VARIABLE!!!!!!

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/guide_window.cpp \
    $$PWD/step_view.cpp \
    $$PWD/open_dialog.cpp \
    $$PWD/yarn_dialog.cpp \
    $$PWD/guide_scale.cpp \
    $$PWD/guide_theme.cpp \
    $$PWD/settings_dialog.cpp \
    $$PWD/row_counter.cpp \
    $$PWD/simulated_row_counter.cpp \
    $$PWD/gpio_row_counter.cpp

HEADERS += \
    $$PWD/guide_window.h \
    $$PWD/step_view.h \
    $$PWD/open_dialog.h \
    $$PWD/yarn_dialog.h \
    $$PWD/guide_scale.h \
    $$PWD/guide_theme.h \
    $$PWD/settings_dialog.h \
    $$PWD/row_counter.h \
    $$PWD/simulated_row_counter.h \
    $$PWD/gpio_row_counter.h

# GPIO row counter: needs libgpiod 2.x (Debian/Raspberry Pi OS: libgpiod-dev). Without it the GPIO source reports
# "no GPIO support" and everything else works.
system(pkg-config --atleast-version=2.0 libgpiod) {
    CONFIG += link_pkgconfig
    PKGCONFIG += libgpiod
    DEFINES += HAVE_GPIOD
}
