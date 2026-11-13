#include "cli.h"

#include <QFileInfo>
#include <QMap>
#include <QStandardPaths>
#include <QTextStream>

#include "pattern_loader.h"
#include "../../../libs/stitchcore/instructions.h"
#include "../../../libs/stitchcore/pieceoutline.h"
#include "../../../libs/stitchcore/project.h"
#include "../../../libs/stitchcore/stitchgrid.h"

using namespace stitchcore;

QString userCatalogPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/catalog.json");
}

int runCli(const CliOptions &options, QTextStream &out, QTextStream &err)
{
    StitchCatalog catalog = StitchCatalog::withDefaults();
    StitchCatalog user;
    if (QFileInfo::exists(userCatalogPath()) && StitchCatalog::load(userCatalogPath(), user))
    {
        catalog = user;
    }

    ProjectSettings settings;
    settings.size = StitchSize{5.0, 3.5};
    QMap<QString, QVector<BandSpec>> bands;
    QString patternFile = options.input;
    QString measurements = options.measurements;
    const bool isProject = options.input.endsWith(QStringLiteral(".stitchly"), Qt::CaseInsensitive);
    QStringList replaced;

    if (isProject)
    {
        Project project;
        QString error;
        if (!loadProject(options.input, project, &error))
        {
            err << "Could not load the project: " << error << "\n";
            return 1;
        }
        // A project is run with the stitches and patterns it was made with.
        replaced = diffCatalog(catalog, project.catalog).differing;
        mergeCatalog(catalog, project.catalog, true);
        settings = project.settings;
        bands = project.bands;
        patternFile = project.patternFile;
        if (measurements.isEmpty())
        {
            measurements = project.measurementsFile;
        }
    }

    if (options.hasStitchWidth) settings.size.stitchWidthMm = options.stitchWidthMm;
    if (options.hasRowHeight) settings.size.rowHeightMm = options.rowHeightMm;
    if (options.seamAllowance) settings.includeSeamAllowance = true;
    if (!options.shapeOn.isEmpty())
    {
        if (options.shapeOn != QLatin1String("outside") && options.shapeOn != QLatin1String("every"))
        {
            err << "--shape-on must be \"outside\" or \"every\".\n";
            return 2;
        }
        settings.shaping.shapeOn = options.shapeOn == QLatin1String("every") ? ShapingRows::EveryRow : ShapingRows::OutsideRows;
    }
    if (!settings.size.isValid())
    {
        err << "Stitch width and row height must be greater than 0.\n";
        return 2;
    }
    catalog.basics().set({QString::fromLatin1(kDefaultStitchName), settings.size});

    LoadedPattern pattern;
    QString error;
    if (!loadPattern(patternFile, pattern, &error, measurements))
    {
        err << "Could not load the pattern: " << error << "\n";
        return 1;
    }
    if (!options.piece.isEmpty())
    {
        bool found = false;
        for (const LoadedPiece &p : pattern.pieces) found = found || p.name == options.piece;
        if (!found)
        {
            QStringList names;
            for (const LoadedPiece &p : pattern.pieces) names << p.name;
            err << "No piece named \"" << options.piece << "\". The pieces are: " << names.join(QStringLiteral(", ")) << "\n";
            return 2;
        }
    }

    if (isProject)
    {
        out << "Project: " << options.input << "\n";
        if (!replaced.isEmpty())
        {
            out << "Catalog entries used as stored in the project: " << replaced.join(QStringLiteral(", ")) << "\n";
        }
    }
    out << "Pattern: " << pattern.fileName << "\n";
    if (!pattern.measurementsFile.isEmpty())
    {
        out << "Measurements: " << pattern.measurementsFile << "\n";
    }
    out << pattern.pieces.size() << " piece(s), default stitch " << settings.size.stitchWidthMm << " x row "
        << settings.size.rowHeightMm << " mm (" << settings.size.stitchesPer10cm() << " sts x "
        << settings.size.rowsPer10cm() << " rows per 10 cm)\n";

    for (const LoadedPiece &piece : pattern.pieces)
    {
        if (!options.piece.isEmpty() && piece.name != options.piece)
        {
            continue;
        }
        const QPolygonF &source = (settings.includeSeamAllowance && !piece.cutLine.isEmpty()) ? piece.cutLine
                                                                                               : piece.seamLine;
        QVector<BandSpec> pieceBands = bands.value(piece.name);
        if (pieceBands.isEmpty())
        {
            BandSpec def;
            def.stitchType = QString::fromLatin1(kDefaultStitchName);
            pieceBands = {def};
        }
        const StitchGrid grid = StitchGrid::build(knittingOutline(source, piece.grainline), pieceBands, catalog);
        out << "\n== " << piece.name << " ==\n";
        if (grid.isEmpty())
        {
            out << "(no stitches" << (isProject ? ": check the bands of this piece" : "") << ")\n";
            continue;
        }
        out << grid.rowCount() << " rows, cast on " << grid.castOn() << ", widest " << grid.width()
            << ", bind off " << grid.bindOff()
            << (piece.grainline.isNull() ? "  [no grainline: drawn orientation used]" : "") << "\n";
        if (options.csv)
        {
            out << rowTableCsv(grid, settings.shaping);
        }
        else
        {
            out << writeInstructions(grid, settings.shaping).join(QLatin1Char('\n')) << "\n";
        }
    }
    return 0;
}
