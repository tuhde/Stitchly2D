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
    parser.addOptions({windowed, piece, rowOption, shot, size});
    parser.process(app);

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

    if (parser.isSet(shot))
    {
        const QString file = parser.value(shot);
        QTimer::singleShot(400, &app, [&window, &app, file]() { app.exit(window.grab().save(file) ? 0 : 1); });
    }
    return app.exec();
}
