// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_INSTRUCTIONS_H
#define STITCHCORE_INSTRUCTIONS_H

#include <QString>
#include <QStringList>

#include "shaping.h"

namespace stitchcore
{

/** Written instructions for one piece, one line per step. Row numbers are one-based. */
QStringList writeInstructions(const StitchGrid &grid, const ShapingOptions &options = ShapingOptions());

/** Row table as CSV: row;first stitch;count;change left;change right. */
QString rowTableCsv(const StitchGrid &grid);

}

#endif
