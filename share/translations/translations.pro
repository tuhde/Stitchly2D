# Only used by lupdate ("make lupdate" in the top-level project) so that it sees every source file.
TEMPLATE = app
TARGET = stitchly2d_translations
QT += core gui widgets network xml printsupport svg multimedia

LANGUAGES = de_DE
for(lang, LANGUAGES) {
    TRANSLATIONS += stitchly2d_$${lang}.ts
}

include(../../src/app/stitchly2d/stitchly2d.pri)

SOURCES += \
    $$files($$PWD/../../src/libs/stitchcore/*.cpp)
HEADERS += \
    $$files($$PWD/../../src/libs/stitchcore/*.h)
