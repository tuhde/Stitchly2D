// guider - fullscreen knitting guide for a touch screen (Raspberry Pi). GPLv3, see LICENSE.
//
// Opens a Stitchly2D project (.stitchly) or a Seamly2D pattern (.sm2d) and shows its instructions as rolling text.
#include <QCommandLineParser>
#include <QFileInfo>
#include <QSettings>
#include <QTextStream>
#include <QTimer>

#include "../../libs/stitchload/core/application_2d.h"
#include "guide_scale.h"
#include "guide_window.h"
#include "open_dialog.h"
#include "row_counter.h"

int main(int argc, char *argv[])
{
    // Resources of the static libraries are only linked in when they are registered by hand.
    Q_INIT_RESOURCE(schema);   // XSD schemas the pattern parser validates against
    Q_INIT_RESOURCE(icon);
    Q_INIT_RESOURCE(theme);
    Q_INIT_RESOURCE(flags);
    Q_INIT_RESOURCE(sounds);
    Q_INIT_RESOURCE(diagrams);

    Application2D app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "Shows knitting instructions row by row."));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("file"),
                                 QCoreApplication::translate("main", "Project (.stitchly) or pattern (.sm2d); default: the last one used."));
    QCommandLineOption windowed(QStringLiteral("windowed"), QCoreApplication::translate("main", "Do not start fullscreen."));
    QCommandLineOption piece(QStringLiteral("piece"), QCoreApplication::translate("main", "Start with this piece."), QStringLiteral("name"));
    QCommandLineOption shot(QStringLiteral("screenshot"),
                            QCoreApplication::translate("main", "Save a picture of the window to this file and quit (for tests)."),
                            QStringLiteral("file"));
    QCommandLineOption size(QStringLiteral("size"), QCoreApplication::translate("main", "Window size WxH with --windowed."),
                            QStringLiteral("WxH"));
    QCommandLineOption rowOption(QStringLiteral("row"), QCoreApplication::translate("main", "Start at this row (0 = cast on)."), QStringLiteral("n"));
    QCommandLineOption counterOption(QStringLiteral("counter"),
        QCoreApplication::translate("main", "Row counter: manual (buttons only), gpio or simulated. Saved for the next start."),
        QStringLiteral("kind"));
    QCommandLineOption chipOption(QStringLiteral("gpio-chip"), QCoreApplication::translate("main", "GPIO chip (default gpiochip0)."),
                                  QStringLiteral("chip"));
    QCommandLineOption lineOption(QStringLiteral("gpio-line"), QCoreApplication::translate("main", "GPIO line (BCM pin number) of the sensor."),
                                  QStringLiteral("n"));
    QCommandLineOption edgeOption(QStringLiteral("gpio-edge"),
        QCoreApplication::translate("main", "Which edge is one pulse: rising, falling (default) or both."), QStringLiteral("edge"));
    QCommandLineOption pullOption(QStringLiteral("gpio-pull"),
        QCoreApplication::translate("main", "Internal pull resistor: up (default), down or none."), QStringLiteral("pull"));
    QCommandLineOption debounceOption(QStringLiteral("gpio-debounce"),
        QCoreApplication::translate("main", "Ignore changes shorter than this many ms (default 5, 0 = off)."), QStringLiteral("ms"));
    QCommandLineOption pulsesOption(QStringLiteral("pulses-per-row"),
        QCoreApplication::translate("main", "Sensor pulses that make one row (default 1)."), QStringLiteral("n"));
    QCommandLineOption simOption(QStringLiteral("sim-period"),
        QCoreApplication::translate("main", "Simulated counter: one pulse every this many ms (default 1000, 0 = never)."), QStringLiteral("ms"));
    parser.addOptions({windowed, piece, rowOption, shot, size, counterOption, chipOption, lineOption, edgeOption, pullOption,
                       debounceOption, pulsesOption, simOption});
    parser.process(app);

    // Row counter: what was used last, changed by the options given now (and saved again).
    QSettings settings(QStringLiteral("Stitchly2D"), QStringLiteral("guider"));
    RowCounterConfig counter;
    counter.load(settings);
    bool counterChanged = false;
    auto take = [&](const QCommandLineOption &option, auto &&assign) {
        if (parser.isSet(option))
        {
            assign(parser.value(option));
            counterChanged = true;
        }
    };
    take(counterOption, [&](const QString &v) { counter.kind = v; });
    take(chipOption, [&](const QString &v) { counter.chip = v; });
    take(lineOption, [&](const QString &v) { counter.line = v.toInt(); });
    take(edgeOption, [&](const QString &v) { counter.edge = v; });
    take(pullOption, [&](const QString &v) { counter.pull = v; });
    take(debounceOption, [&](const QString &v) { counter.debounceMs = v.toInt(); });
    take(pulsesOption, [&](const QString &v) { counter.pulsesPerRow = v.toInt(); });
    take(simOption, [&](const QString &v) { counter.simulatedPeriodMs = v.toInt(); });
    if (counterChanged)
    {
        const QString problem = counter.problem();
        if (!problem.isEmpty())
        {
            QTextStream(stderr) << problem << "\n";
            return 2;
        }
        counter.save(settings);
    }

    applyGuideStyle(app);
    GuideWindow window;

    QString path = parser.positionalArguments().value(0);
    if (path.isEmpty())
    {
        path = QSettings(QStringLiteral("Stitchly2D"), QStringLiteral("guider")).value(QStringLiteral("lastFile")).toString();
        if (!QFileInfo::exists(path)) path.clear();
    }
    if (parser.isSet(windowed))
    {
        const QStringList wh = parser.value(size).split(QLatin1Char('x'));
        window.resize(wh.size() == 2 ? QSize(wh[0].toInt(), wh[1].toInt()) : QSize(1280, 720));
        window.showOnSavedScreen(false);
    }
    else
    {
        window.setCursor(Qt::BlankCursor);   // touch screen: no pointer
        window.showOnSavedScreen(true);
    }

    if (!path.isEmpty())
    {
        QString error;
        if (!window.openFile(path, &error))
        {
            QTextStream(stderr) << QCoreApplication::translate("main", "Could not load the pattern: %1").arg(error) << "\n";
            if (parser.isSet(shot)) return 1;
        }
        if (parser.isSet(piece))
        {
            for (int i = 0; i < window.job().pieces.size(); ++i)
            {
                if (window.job().pieces.at(i).name == parser.value(piece)) window.setPiece(i);
            }
        }
    }
    if (!path.isEmpty() && parser.isSet(rowOption))
    {
        window.setRow(parser.value(rowOption).toInt());
    }
    if (path.isEmpty() && !parser.isSet(shot))
    {
        window.chooseFile();
        if (window.job().pieces.isEmpty()) return 0;     // nothing chosen: nothing to show
    }

    // A counter that cannot start (wrong pin, no permission) is shown in the window; the buttons keep working.
    QString counterError;
    if (!window.applyCounterConfig(counter, false, &counterError))
    {
        QTextStream(stderr) << counterError << "\n";
    }

    if (parser.isSet(shot))
    {
        const QString file = parser.value(shot);
        QTimer::singleShot(400, &app, [&window, &app, file]() { app.exit(window.grab().save(file) ? 0 : 1); });
    }
    return app.exec();
}
