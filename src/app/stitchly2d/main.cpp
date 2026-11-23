// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
//
// Starts the main window. With --cli it loads a Seamly2D pattern and prints a stitch chart summary and the written
// instructions for every piece instead, without needing a display.
#include <QCommandLineParser>
#include <QTextStream>
#include <QTimer>

#include "../stitchload/core/application_2d.h"
#include "core/cli.h"
#include "mainwindow.h"
#include "../stitchload/core/pattern_loader.h"


int main(int argc, char *argv[])
{
    // Resources of the static libraries are only linked in when they are registered by hand.
    Q_INIT_RESOURCE(schema);   // XSD schemas the pattern parser validates against
    Q_INIT_RESOURCE(icon);
    Q_INIT_RESOURCE(theme);
    Q_INIT_RESOURCE(flags);
    Q_INIT_RESOURCE(sounds);
    Q_INIT_RESOURCE(diagrams);

    // Command-line modes need no display: run on the offscreen platform (must be chosen before the application starts).
    bool noDisplay = false;
    for (int i = 1; i < argc; ++i)
    {
        const QByteArray a(argv[i]);
        noDisplay = noDisplay || a == "--cli" || a == "--screenshot" || a.startsWith("--screenshot=");
    }
    if (noDisplay && qgetenv("QT_QPA_PLATFORM").isEmpty())
    {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    Application2D app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Prepares Seamly2D patterns for knitting."));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("pattern"),
                                 QCoreApplication::translate("main", "Seamly2D pattern (.sm2d) or Stitchly2D project (.stitchly), optional."));
    QCommandLineOption stitchW({QStringLiteral("w"), QStringLiteral("stitch-width")},
                               QCoreApplication::translate("main", "Width of one stitch in mm (default 5)."), QStringLiteral("mm"),
                               QStringLiteral("5"));
    QCommandLineOption rowH({QStringLiteral("r"), QStringLiteral("row-height")},
                            QCoreApplication::translate("main", "Height of one row in mm (default 3.5)."), QStringLiteral("mm"),
                            QStringLiteral("3.5"));
    QCommandLineOption meas({QStringLiteral("m"), QStringLiteral("measurements")},
                            QCoreApplication::translate("main", "Measurement file to use instead of the one in the pattern."),
                            QStringLiteral("file"));
    QCommandLineOption allowance(QStringLiteral("seam-allowance"), QCoreApplication::translate("main", "Include the seam allowance."));
    QCommandLineOption csv(QStringLiteral("csv"), QCoreApplication::translate("main", "Print the row table as CSV instead of instructions."));
    QCommandLineOption cli(QStringLiteral("cli"), QCoreApplication::translate("main", "Print the result as text instead of opening the window."));
    QCommandLineOption shot(QStringLiteral("screenshot"),
                            QCoreApplication::translate("main", "Open the window, save a picture of it to this file and quit."),
                            QStringLiteral("file"));
    QCommandLineOption shapeOnOption(QStringLiteral("shape-on"),
                                     QCoreApplication::translate("main", "With --cli: shape the edges on \"outside\" rows only (default) or on \"every\" row."),
                                     QStringLiteral("rows"));
    QCommandLineOption methodOption(QStringLiteral("method"),
                                    QCoreApplication::translate("main", "With --cli: knit all pieces flat, round, circular_machine or double_bed_machine."),
                                    QStringLiteral("method"));
    QCommandLineOption needlesOption(QStringLiteral("needles"),
                                     QCoreApplication::translate("main", "With --cli: needles of the circular machine, or per bed of the double-bed machine."),
                                     QStringLiteral("n"));
    QCommandLineOption pieceOption(QStringLiteral("piece"), QCoreApplication::translate("main", "With --cli: only this piece."),
                                   QStringLiteral("name"));
    QCommandLineOption langOption(QStringLiteral("lang"),
                                  QCoreApplication::translate("main", "Language for this run, for example de_DE (default: the saved choice)."),
                                  QStringLiteral("locale"));
    parser.addOptions({cli, shot, stitchW, rowH, meas, allowance, csv, pieceOption, shapeOnOption, methodOption, needlesOption,
                       langOption});
    parser.process(app);
    if (parser.isSet(langOption))
    {
        app.loadTranslations(parser.value(langOption));
    }

    if (!parser.isSet(cli))
    {
        MainWindow window;
        if (!parser.positionalArguments().isEmpty())
        {
            QString openError;
            const QString first = parser.positionalArguments().first();
            const bool project = first.endsWith(QStringLiteral(".stitchly"), Qt::CaseInsensitive);
            if (!(project ? window.openProject(first, &openError) : window.openPattern(first, &openError)))
            {
                QTextStream(stderr) << QCoreApplication::translate("main", "Could not load the pattern: %1").arg(openError) << "\n";
                if (parser.isSet(shot)) return 1;
            }
        }
        window.show();
        if (parser.isSet(shot))
        {
            const QString file = parser.value(shot);
            QTimer::singleShot(300, &app, [&window, &app, file]() {
                const bool ok = window.grab().save(file);
                app.exit(ok ? 0 : 1);
            });
        }
        return app.exec();
    }

    QTextStream out(stdout);
    QTextStream err(stderr);
    if (parser.positionalArguments().size() != 1)
    {
        err << QCoreApplication::translate("main", "--cli needs exactly one pattern (.sm2d) or project (.stitchly) file. See --help.") << "\n";
        return 2;
    }
    CliOptions options;
    options.input = parser.positionalArguments().first();
    options.measurements = parser.value(meas);
    options.hasStitchWidth = parser.isSet(stitchW);
    options.stitchWidthMm = parser.value(stitchW).toDouble();
    options.hasRowHeight = parser.isSet(rowH);
    options.rowHeightMm = parser.value(rowH).toDouble();
    options.seamAllowance = parser.isSet(allowance);
    options.csv = parser.isSet(csv);
    options.piece = parser.value(pieceOption);
    options.shapeOn = parser.value(shapeOnOption);
    options.method = parser.value(methodOption);
    if (parser.isSet(needlesOption))
    {
        bool ok = false;
        options.needles = parser.value(needlesOption).toInt(&ok);
        if (!ok || options.needles < 0)
        {
            err << QCoreApplication::translate("main", "--needles must be a number, 0 or more.") << "\n";
            return 2;
        }
    }
    return runCli(options, out, err);
}
