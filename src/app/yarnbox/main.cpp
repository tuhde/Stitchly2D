// yarnbox - the yarn inventory of Stitchly2D on its own. GPLv3, see LICENSE.
//
// Edits yarns.json, the file the editor and the guider use. No pattern loader, so it starts fast, also on a small computer.
#include <QApplication>
#include <QCommandLineParser>
#include <QLocale>
#include <QTextStream>
#include <QTranslator>

#include "catalog.h"
#include "yarn.h"
#include "yarnbox_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    // The same names as the other programs: they share one configuration folder (yarns.json, catalog.json).
    app.setApplicationName(QStringLiteral("Stitchly2D"));
    app.setOrganizationName(QStringLiteral("Stitchly2D"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Edits the yarn inventory of Stitchly2D."));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("file"),
                                 QCoreApplication::translate("main", "Yarn file; default: the one the editor and the guider use."));
    QCommandLineOption langOption(QStringLiteral("lang"),
        QCoreApplication::translate("main", "Language for this run, for example de_DE (default: the system language)."), QStringLiteral("locale"));
    parser.addOption(langOption);
    parser.process(app);

    // The translations are built into the program (share/translations, embedded by translations.pri).
    const QString locale = parser.isSet(langOption) ? parser.value(langOption) : QLocale::system().name();
    QTranslator translator;
    if (translator.load(QStringLiteral("stitchly2d_") + locale, QStringLiteral(":/i18n")))
    {
        app.installTranslator(&translator);
    }

    // Gauges are entered for the basic stitches of the user's catalog.
    stitchcore::StitchCatalog catalog = stitchcore::StitchCatalog::withDefaults();
    stitchcore::StitchCatalog user;
    if (stitchcore::StitchCatalog::load(stitchcore::catalogFilePath(), user))
    {
        catalog = user;
    }
    QStringList stitchTypes;
    for (const stitchcore::StitchType &t : catalog.basics().types()) stitchTypes << t.name;

    const QString file = parser.positionalArguments().value(0, stitchcore::userYarnPath());
    YarnboxWindow window(file, stitchTypes);
    QString error;
    if (!window.load(&error))
    {
        QTextStream(stderr) << error << "\n";           // the window stays open and says so; it does not overwrite the file
    }
    window.show();
    return app.exec();
}
