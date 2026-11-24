# Only used by lupdate ("make lupdate" in the top-level project) so that it sees every source file.
TEMPLATE = app
TARGET = stitchly2d_translations
QT += core gui widgets network xml printsupport svg multimedia

LANGUAGES = de_DE
for(lang, LANGUAGES) {
    TRANSLATIONS += stitchly2d_$${lang}.ts
}

include(../../src/app/stitchly2d/stitchly2d.pri)

# stitchcore (instructions, passes) and stitchload (pattern loader, Application2D, VPattern) are libraries of their
# own, so their files are not in stitchly2d.pri.
SOURCES += \
    $$files($$PWD/../../src/libs/stitchcore/*.cpp) \
    $$files($$PWD/../../src/libs/stitchload/core/*.cpp) \
    $$files($$PWD/../../src/libs/stitchload/xml/*.cpp) \
    $$files($$PWD/../../src/libs/yarnui/*.cpp)
HEADERS += \
    $$files($$PWD/../../src/libs/stitchcore/*.h) \
    $$files($$PWD/../../src/libs/stitchload/core/*.h) \
    $$files($$PWD/../../src/libs/stitchload/xml/*.h) \
    $$files($$PWD/../../src/libs/yarnui/*.h)

# guider (its own program, same translation files)
SOURCES += $$files($$PWD/../../src/app/guider/*.cpp)
HEADERS += $$files($$PWD/../../src/app/guider/*.h)
