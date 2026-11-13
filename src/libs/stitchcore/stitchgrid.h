// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_STITCHGRID_H
#define STITCHCORE_STITCHGRID_H

#include <QPolygonF>
#include <QVector>

#include "gauge.h"

namespace stitchcore
{

/** One knitted row: stitches [firstStitch, firstStitch + count) relative to the widest point of the piece. */
struct Row
{
    int index       = 0;  //!< 0 = cast-on row (bottom)
    int firstStitch = 0;
    int count       = 0;

    int end() const { return firstStitch + count; }
};

/**
 * @brief Stitch grid of one piece.
 *
 * The outline is given in millimetres with the knitting direction along +y (row 0 is at the smallest y).
 * The caller rotates the piece so the grainline is vertical and flips y if the source uses y-down.
 */
class StitchGrid
{
public:
    static StitchGrid build(const QPolygonF &outlineMm, const Gauge &gauge);

    const QVector<Row> &rows() const { return m_rows; }
    int rowCount() const { return m_rows.size(); }
    int width() const { return m_width; }          //!< widest row, in stitches
    int castOn() const { return m_rows.isEmpty() ? 0 : m_rows.first().count; }
    int bindOff() const { return m_rows.isEmpty() ? 0 : m_rows.last().count; }
    bool isEmpty() const { return m_rows.isEmpty(); }

private:
    QVector<Row> m_rows;
    int          m_width = 0;
};

}

#endif
