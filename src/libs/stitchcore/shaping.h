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

/** Change of the left and right edge going from the previous row to this one. Positive = stitches added. */
struct Shaping
{
    int row   = 0;
    int left  = 0;
    int right = 0;
    int stitchesAfter = 0;
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
