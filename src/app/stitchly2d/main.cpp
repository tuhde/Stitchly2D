// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
//
// Command-line front end for now: loads a Seamly2D pattern and prints a stitch chart summary and the written
// instructions for every piece. The main window follows.
#include <QCommandLineParser>
#include <QTextStream>

#include "core/application_2d.h"
#include "core/pattern_loader.h"

#include "../../libs/stitchcore/instructions.h"
#include "../../libs/stitchcore/pieceoutline.h"
#include "../../libs/stitchcore/stitchgrid.h"

int main(int argc, char *argv[])
{
    // Resources of the static libraries are only linked in when they are registered by hand.
    Q_INIT_RESOURCE(schema);   // XSD schemas the pattern parser validates against
    Q_INIT_RESOURCE(icon);
    Q_INIT_RESOURCE(theme);
    Q_INIT_RESOURCE(flags);
    Q_INIT_RESOURCE(sounds);
    Q_INIT_RESOURCE(diagrams);

    // No main window yet: run on the offscreen platform so no display is needed.
    if (qgetenv("QT_QPA_PLATFORM").isEmpty())
    {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    Application2D app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Prepares Seamly2D patterns for knitting."));
    parser.addHelpOption();
    parser.addPositionalArgument(QStringLiteral("pattern"), QStringLiteral("Seamly2D pattern (.sm2d)."));
    QCommandLineOption stitchW({QStringLiteral("w"), QStringLiteral("stitch-width")},
                               QStringLiteral("Width of one stitch in mm (default 5)."), QStringLiteral("mm"),
                               QStringLiteral("5"));
    QCommandLineOption rowH({QStringLiteral("r"), QStringLiteral("row-height")},
                            QStringLiteral("Height of one row in mm (default 3.5)."), QStringLiteral("mm"),
                            QStringLiteral("3.5"));
    QCommandLineOption meas({QStringLiteral("m"), QStringLiteral("measurements")},
                            QStringLiteral("Measurement file to use instead of the one in the pattern."),
                            QStringLiteral("file"));
    QCommandLineOption allowance(QStringLiteral("seam-allowance"), QStringLiteral("Include the seam allowance."));
    QCommandLineOption csv(QStringLiteral("csv"), QStringLiteral("Print the row table as CSV instead of instructions."));
    parser.addOptions({stitchW, rowH, meas, allowance, csv});
    parser.process(app);

    QTextStream out(stdout);
    QTextStream err(stderr);
    if (parser.positionalArguments().size() != 1)
    {
        err << "Give exactly one pattern file. See --help.\n";
        return 2;
    }
    stitchcore::StitchSize size;
    size.stitchWidthMm = parser.value(stitchW).toDouble();
    size.rowHeightMm = parser.value(rowH).toDouble();
    if (!size.isValid())
    {
        err << "Stitch width and row height must be greater than 0.\n";
        return 2;
    }

    LoadedPattern pattern;
    QString error;
    if (!loadPattern(parser.positionalArguments().first(), pattern, &error, parser.value(meas)))
    {
        err << "Could not load the pattern: " << error << "\n";
        return 1;
    }
    out << "Pattern: " << pattern.fileName << "\n";
    if (!pattern.measurementsFile.isEmpty())
    {
        out << "Measurements: " << pattern.measurementsFile << "\n";
    }
    out << pattern.pieces.size() << " piece(s), stitch " << size.stitchWidthMm << " x row " << size.rowHeightMm
        << " mm (" << size.stitchesPer10cm() << " sts x " << size.rowsPer10cm() << " rows per 10 cm)\n";

    for (const LoadedPiece &piece : pattern.pieces)
    {
        const QPolygonF &source = (parser.isSet(allowance) && !piece.cutLine.isEmpty()) ? piece.cutLine
                                                                                         : piece.seamLine;
        const stitchcore::StitchGrid grid =
            stitchcore::StitchGrid::build(stitchcore::knittingOutline(source, piece.grainline), size);
        out << "\n== " << piece.name << " ==\n";
        if (grid.isEmpty())
        {
            out << "(no stitches)\n";
            continue;
        }
        out << grid.rowCount() << " rows, cast on " << grid.castOn() << ", widest " << grid.width()
            << ", bind off " << grid.bindOff()
            << (piece.grainline.isNull() ? "  [no grainline: drawn orientation used]" : "") << "\n";
        if (parser.isSet(csv))
        {
            out << stitchcore::rowTableCsv(grid);
        }
        else
        {
            out << stitchcore::writeInstructions(grid).join(QLatin1Char('\n')) << "\n";
        }
    }
    return 0;
}
