#include "knit_job.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QMap>
#include <QStandardPaths>

#include "../../stitchcore/catalog.h"
#include "../../stitchcore/pieceoutline.h"
#include "../../stitchcore/project.h"
#include "../../stitchcore/stitchgrid.h"
#include "pattern_loader.h"

using namespace stitchcore;

QString userCatalogPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/catalog.json");
}

bool buildKnitJob(const QString &input, KnitJob &job, QString *error)
{
    auto fail = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };
    StitchCatalog catalog = StitchCatalog::withDefaults();
    StitchCatalog user;
    if (QFileInfo::exists(userCatalogPath()) && StitchCatalog::load(userCatalogPath(), user))
    {
        catalog = user;
    }

    ProjectSettings settings;
    settings.size = StitchSize{5.0, 3.5};
    QMap<QString, QVector<BandSpec>> bands;
    QMap<QString, KnitMethod> methods;
    QString patternFile = input;
    QString measurements;
    QMap<QString, YarnRef> yarns;
    if (input.endsWith(QStringLiteral(".stitchly"), Qt::CaseInsensitive))
    {
        Project project;
        QString projectError;
        if (!loadProject(input, project, &projectError))
        {
            return fail(QCoreApplication::translate("KnitJob", "Could not load the project: %1").arg(projectError));
        }
        // A project is knitted with the stitches and patterns it was made with.
        mergeCatalog(catalog, project.catalog, true);
        settings = project.settings;
        bands = project.bands;
        methods = project.methods;
        patternFile = project.patternFile;
        measurements = project.measurementsFile;
        yarns = project.yarns;
    }
    if (!settings.size.isValid())
    {
        return fail(QCoreApplication::translate("KnitJob", "Stitch width and row height must be greater than 0."));
    }
    catalog.basics().set({QString::fromLatin1(kDefaultStitchName), settings.size});

    LoadedPattern pattern;
    QString loadError;
    if (!loadPattern(patternFile, pattern, &loadError, measurements))
    {
        return fail(QCoreApplication::translate("KnitJob", "Could not load the pattern: %1").arg(loadError));
    }

    job = KnitJob();
    job.input = input;
    job.patternFile = patternFile;
    job.yarns = yarns;
    for (const LoadedPiece &piece : pattern.pieces)
    {
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
        shaping.method = methods.value(piece.name, settings.shaping.method);

        KnitPiece out;
        out.name = piece.name;
        out.method = shaping.method;
        out.rowCount = grid.rowCount();
        out.castOn = grid.castOn();
        out.bindOff = grid.bindOff();
        out.steps = writeInstructionSteps(grid, shaping);
        out.yarnUses = yarnUses(grid, settings.stitchType);
        out.rowPasses.resize(grid.rowCount() + 2);                    // cast on and bind off are one pass each
        for (int row = 1; row <= grid.rowCount(); ++row)
        {
            out.rowPasses[row] = passesForRow(grid, shaping.method, row - 1);
        }
        job.pieces << out;
    }
    return true;
}
