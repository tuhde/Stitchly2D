message("Entering src.pro")

TEMPLATE = subdirs

SUBDIRS = \
    libs \
    app \
    test

app.depends = libs
test.depends = libs
