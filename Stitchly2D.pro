message("Entering Stitchly2D.pro")

#Check if spaces are in directory names
LIST = $$split(PWD,' ')
count(LIST, 1, >): error("The build will fail. Path '$${PWD}' contains space!!!")

LIST = $$split(OUT_PWD,' ')
count(LIST, 1, >): error("The build will fail. Path '$${OUT_PWD}' contains space!!!")

TEMPLATE = subdirs
SUBDIRS = src

# make lupdate: collect the translatable strings into share/translations/*.ts
qtPrepareTool(LUPDATE, lupdate)
lupdate.commands = $$LUPDATE -noobsolete -locations none $$PWD/share/translations/translations.pro
QMAKE_EXTRA_TARGETS += lupdate
