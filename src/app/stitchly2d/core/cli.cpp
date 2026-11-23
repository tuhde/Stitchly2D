#include "cli.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QMap>
#include <QStandardPaths>
#include <QTextStream>

#include "../stitchload/core/knit_job.h"
#include "../stitchload/core/pattern_loader.h"
#include "../../../libs/stitchcore/instructions.h"
#include "../../../libs/stitchcore/pieceoutline.h"
#include "../../../libs/stitchcore/project.h"
#include "../../../libs/stitchcore/stitchgrid.h"

using namespace stitchcore;

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
    QMap<QString, KnitMethod> methods;

    if (isProject)
    {
        Project project;
        QString error;
        if (!loadProject(options.input, project, &error))
        {
            err << QCoreApplication::translate("Cli", "Could not load the project: %1").arg(error) << "\n";
            return 1;
        }
        // A project is run with the stitches and patterns it was made with.
        replaced = diffCatalog(catalog, project.catalog).differing;
        mergeCatalog(catalog, project.catalog, true);
        settings = project.settings;
        bands = project.bands;
        methods = project.methods;
        patternFile = project.patternFile;
        if (measurements.isEmpty())
        {
            measurements = project.measurementsFile;
        }
    }

    if (options.hasStitchWidth) settings.size.stitchWidthMm = options.stitchWidthMm;
    if (options.hasRowHeight) settings.size.rowHeightMm = options.rowHeightMm;
    if (options.seamAllowance) settings.includeSeamAllowance = true;
    KnitMethod forcedMethod = KnitMethod::Flat;
    const bool hasForcedMethod = !options.method.isEmpty();
    if (hasForcedMethod && !knitMethodFromName(options.method, forcedMethod))
    {
        err << QCoreApplication::translate("Cli", "--method must be flat, round, circular_machine or double_bed_machine.") << "\n";
        return 2;
    }
    if (options.needles >= 0) settings.shaping.machineNeedles = options.needles;
    if (!options.shapeOn.isEmpty())
    {
        if (options.shapeOn != QLatin1String("outside") && options.shapeOn != QLatin1String("every"))
        {
            err << QCoreApplication::translate("Cli", "--shape-on must be \"outside\" or \"every\".") << "\n";
            return 2;
        }
        settings.shaping.shapeOn = options.shapeOn == QLatin1String("every") ? ShapingRows::EveryRow : ShapingRows::OutsideRows;
    }
    if (!settings.size.isValid())
    {
        err << QCoreApplication::translate("Cli", "Stitch width and row height must be greater than 0.") << "\n";
        return 2;
    }
    catalog.basics().set({QString::fromLatin1(kDefaultStitchName), settings.size});

    LoadedPattern pattern;
    QString error;
    if (!loadPattern(patternFile, pattern, &error, measurements))
    {
        err << QCoreApplication::translate("Cli", "Could not load the pattern: %1").arg(error) << "\n";
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
            err << QCoreApplication::translate("Cli", "No piece named \"%1\". The pieces are: %2").arg(options.piece, names.join(QStringLiteral(", "))) << "\n";
            return 2;
        }
    }

    if (isProject)
    {
        out << QCoreApplication::translate("Cli", "Project: %1").arg(options.input) << "\n";
        if (!replaced.isEmpty())
        {
            out << QCoreApplication::translate("Cli", "Catalog entries used as stored in the project: %1").arg(replaced.join(QStringLiteral(", "))) << "\n";
        }
    }
    out << QCoreApplication::translate("Cli", "Pattern: %1").arg(pattern.fileName) << "\n";
    if (!pattern.measurementsFile.isEmpty())
    {
        out << QCoreApplication::translate("Cli", "Measurements: %1").arg(pattern.measurementsFile) << "\n";
    }
    out << QCoreApplication::translate("Cli", "%1 piece(s), default stitch %2 x row %3 mm (%4 sts x %5 rows per 10 cm)")
               .arg(pattern.pieces.size()).arg(settings.size.stitchWidthMm, 0, 'g', 6)
               .arg(settings.size.rowHeightMm, 0, 'g', 6).arg(settings.size.stitchesPer10cm(), 0, 'g', 6)
               .arg(settings.size.rowsPer10cm(), 0, 'g', 6)
        << "\n";

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
        ShapingOptions shaping = settings.shaping;
        shaping.method = hasForcedMethod ? forcedMethod : methods.value(piece.name, settings.shaping.method);
        out << "\n== " << piece.name << " ==\n";
        if (grid.isEmpty())
        {
            out << (isProject ? QCoreApplication::translate("Cli", "(no stitches: check the bands of this piece)")
                              : QCoreApplication::translate("Cli", "(no stitches)")) << "\n";
            continue;
        }
        out << QCoreApplication::translate("Cli", "%1 rows, cast on %2, widest %3, bind off %4")
                   .arg(grid.rowCount()).arg(grid.castOn()).arg(grid.width()).arg(grid.bindOff())
            << (isRound(shaping.method) ? QStringLiteral("  [%1]").arg(knitMethodName(shaping.method)) : QString())
            << (piece.grainline.isNull() ? QCoreApplication::translate("Cli", "  [no grainline: drawn orientation used]")
                                         : QString())
            << "\n";
        if (options.csv)
        {
            out << rowTableCsv(grid, shaping);
        }
        else
        {
            out << writeInstructions(grid, shaping).join(QLatin1Char('\n')) << "\n";
        }
    }
    return 0;
}
