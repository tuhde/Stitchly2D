TRANSLATIONS_PATH = $${PWD}/../../../share/translations

# Languages of the program (English is the source language and needs no file). To add one, add its locale here and
# in share/translations/translations.pro, run "make lupdate", and translate stitchly2d_<locale>.ts in Qt Linguist.
# An example of using LOCALES="de_DE fr_FR"
isEmpty(LOCALES) {
    LANGUAGES = de_DE
} else {
    LANGUAGES = $$LOCALES
}

# lrelease turns the .ts files into .qm at build time and embed_translations puts them into the program as
# resources under ":/i18n/" (VAbstractApplication::translationsPath()).
CONFIG += lrelease embed_translations
for(lang, LANGUAGES) {
    EXTRA_TRANSLATIONS += $${TRANSLATIONS_PATH}/stitchly2d_$${lang}.ts
}
