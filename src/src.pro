message("Entering src.pro")

TEMPLATE = subdirs

SUBDIRS = \
    libs \
    app

app.depends = libs
