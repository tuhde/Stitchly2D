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

/**
 * How a piece is knitted. Flat: by hand, back and forth (outside and inside rows). The other three knit a tube:
 * Round = by hand in the round, CircularMachine = on a circular knitting machine (a cylinder of needles),
 * DoubleBedMachine = tubular on a double-bed machine (half of the stitches on each bed). There are no inside rows
 * in a tube, and the width of the piece is the circumference.
 * DoubleBedFlat = a flat piece on a double-bed machine (large pieces, e.g. double-bed jacquard): both beds are
 * worked with the same needles, so there are edges and outside and inside rows, like Flat.
 * (Appended last so that saved numbers stay valid; projects store the names.)
 */
enum class KnitMethod { Flat, Round, CircularMachine, DoubleBedMachine, DoubleBedFlat };

/** Tube methods; DoubleBedFlat is worked back and forth like Flat. */
inline bool isRound(KnitMethod m) { return m != KnitMethod::Flat && m != KnitMethod::DoubleBedFlat; }
inline bool isMachine(KnitMethod m)
{
    return m == KnitMethod::CircularMachine || m == KnitMethod::DoubleBedMachine || m == KnitMethod::DoubleBedFlat;
}

QString knitMethodName(KnitMethod method);                         //!< "flat", "round", "circular_machine", "double_bed_machine", "double_bed_flat"
bool knitMethodFromName(const QString &name, KnitMethod &method);

/** On which rows the edges are shaped. Row 1 is an outside row, then they alternate: inside, outside, ... */
enum class ShapingRows { OutsideRows, EveryRow };

struct ShapingOptions
{
    IncreaseMethod increase = IncreaseMethod::M1;
    DecreaseMethod decrease = DecreaseMethod::K2tog_Ssk;
    int edgeStitches = 1;       //!< plain stitches kept at each edge before a shaping stitch
    KnitMethod  method = KnitMethod::Flat;
    int         machineNeedles = 0;      //!< needles of the circular machine, or needles per bed of the double-bed machine; 0 = not checked
    ShapingRows shapeOn = ShapingRows::OutsideRows;   //!< flat knitting: shaping is normally done on outside rows
    /** Tubes have no inside rows, so they are shaped on every round. */
    ShapingRows effectiveShapeOn() const { return isRound(method) ? ShapingRows::EveryRow : shapeOn; }
    int bulkFrom = 4;           //!< a change of at least this many stitches at one edge is cast on / bound off
                                //!< instead of increased / decreased in one row; 0 = never
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

/**
 * The shaping rows of a piece. With ShapingRows::OutsideRows a change that the outline needs on an inside row is
 * worked on the outside row before it (the first inside row: the one after it), so the stitch total is right from
 * then on; changes of two rows that end up on the same row are added. Changes of the stitch type are never moved.
 */
QVector<Shaping> planShaping(const StitchGrid &grid, ShapingRows rows = ShapingRows::EveryRow);
QVector<Segment> planSegments(const StitchGrid &grid, ShapingRows rows = ShapingRows::EveryRow);

/** Stitches on the needle after each row, following the plan (differs from the outline's count between moved rows). */
QVector<int> plannedCounts(const StitchGrid &grid, ShapingRows rows = ShapingRows::EveryRow);

}

#endif
