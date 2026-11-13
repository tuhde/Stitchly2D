// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_SHAPING_H
#define STITCHCORE_SHAPING_H

#include <QString>
#include <QVector>

#include "stitchgrid.h"

namespace stitchcore
{

enum class IncreaseMethod { M1, YarnOver, KFB };
enum class DecreaseMethod { K2tog_Ssk, K2tog };

struct ShapingOptions
{
    IncreaseMethod increase = IncreaseMethod::M1;
    DecreaseMethod decrease = DecreaseMethod::K2tog_Ssk;
    int edgeStitches = 1;       //!< plain stitches kept at each edge before a shaping stitch
};

/**
 * Change from the previous row to this one.
 * Inside a band: left/right are stitches added (positive) or removed at each edge.
 * At a band change (bandChange): the stitch type changes and the count goes from stitchesBefore to stitchesAfter;
 * left and right are not used, because stitch positions of different bands cannot be compared.
 */
struct Shaping
{
    int  row   = 0;
    int  left  = 0;
    int  right = 0;
    bool bandChange = false;
    int  band  = 0;               //!< band that starts at this row (bandChange only)
    int  stitchesBefore = 0;
    int  stitchesAfter = 0;
};

/** A block of rows: either worked straight, or one shaping row. */
struct Segment
{
    int  firstRow = 0;      //!< zero-based
    int  rows     = 1;
    bool shaped   = false;
    Shaping shaping;
};

QVector<Shaping> planShaping(const StitchGrid &grid);
QVector<Segment> planSegments(const StitchGrid &grid);

}

#endif
