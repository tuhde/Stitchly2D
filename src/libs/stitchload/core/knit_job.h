// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef KNIT_JOB_H
#define KNIT_JOB_H

#include <QString>
#include <QVector>

#include "../../stitchcore/instructions.h"
#include "../../stitchcore/passes.h"
#include "../../stitchcore/shaping.h"

/** Where the user's catalog (catalog.json) is kept. */
QString userCatalogPath();

/** One piece ready to knit: its instructions as steps tied to rows. */
struct KnitPiece
{
    QString                                   name;
    int                                       rowCount = 0;     //!< rows of the grid; 0 = no stitches (steps is empty)
    int                                       castOn = 0;
    int                                       bindOff = 0;
    stitchcore::KnitMethod                    method = stitchcore::KnitMethod::Flat;
    QVector<stitchcore::InstructionStep>      steps;
    /** What the machine does for each row, indexed like the instruction rows: 0 = cast on, 1..rowCount, rowCount + 1 = bind off. */
    QVector<stitchcore::RowPasses>            rowPasses;
};

struct KnitJob
{
    QString            input;            //!< the file that was opened (.sm2d or .stitchly)
    QString            patternFile;
    QVector<KnitPiece> pieces;
};

/**
 * Loads a Seamly2D pattern (.sm2d, default stitch 5 x 3.5 mm) or a Stitchly2D project (.stitchly, with its stitches,
 * bands, patterns and knitting methods) and writes the instructions of every piece. Same result as the editor
 * and `stitchly2d --cli` without options. Needs an Application2D. Returns false and fills error on failure.
 */
bool buildKnitJob(const QString &input, KnitJob &job, QString *error);

#endif
