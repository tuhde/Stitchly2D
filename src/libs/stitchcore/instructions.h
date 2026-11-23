// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_INSTRUCTIONS_H
#define STITCHCORE_INSTRUCTIONS_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "shaping.h"

namespace stitchcore
{

/**
 * One line of the written instructions together with the rows it applies to (one-based, inclusive).
 * Lines before the first row (cast on, notes, warnings) have firstRow = lastRow = 0; the bind-off line
 * has firstRow = lastRow = rowCount + 1. A "pattern:" line follows the row line it belongs to and has the same rows.
 */
struct InstructionStep
{
    int     firstRow = 0;
    int     lastRow  = 0;
    QString text;
};

/** The instructions as steps. writeInstructions() returns just their texts. */
QVector<InstructionStep> writeInstructionSteps(const StitchGrid &grid, const ShapingOptions &options = ShapingOptions());

/** Indexes of the steps that cover this row (0 = before the first row, rowCount + 1 = after the last), in order. */
QVector<int> stepsForRow(const QVector<InstructionStep> &steps, int row);

/**
 * Where the next block of instructions starts: the first row after `row` at which a step begins (a block is what one
 * step covers, e.g. "Rows 5-12: work even"). Returns row itself if there is no later block.
 */
int nextStepRow(const QVector<InstructionStep> &steps, int row);

/**
 * Where the current block starts if `row` is inside it, else where the block before starts (the last start of a step
 * before `row`), like "previous" in a music player; row itself if there is none.
 */
int previousStepRow(const QVector<InstructionStep> &steps, int row);

/** Written instructions for one piece, one line per step. Row numbers are one-based. */
QStringList writeInstructions(const StitchGrid &grid, const ShapingOptions &options = ShapingOptions());

/** Row table as CSV: row;band;stitch type;first stitch;count;change left;change right (count and changes as planned). */
QString rowTableCsv(const StitchGrid &grid, const ShapingOptions &options = ShapingOptions());

}

#endif
