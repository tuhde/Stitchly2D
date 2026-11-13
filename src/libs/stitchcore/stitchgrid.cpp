#include "stitchgrid.h"

#include <QtMath>
#include <algorithm>

namespace stitchcore
{

namespace
{
// Horizontal extent of the polygon at height y; false when the line misses it.
bool spanAt(const QPolygonF &poly, qreal y, qreal &xmin, qreal &xmax)
{
    bool found = false;
    const int n = poly.size();
    for (int i = 0; i < n; ++i)
    {
        const QPointF a = poly.at(i);
        const QPointF b = poly.at((i + 1) % n);
        if ((a.y() <= y && b.y() > y) || (b.y() <= y && a.y() > y))
        {
            const qreal x = a.x() + (y - a.y()) * (b.x() - a.x()) / (b.y() - a.y());
            if (!found)
            {
                xmin = xmax = x;
                found = true;
            }
            else
            {
                xmin = qMin(xmin, x);
                xmax = qMax(xmax, x);
            }
        }
    }
    return found;
}
}

StitchGrid StitchGrid::build(const QPolygonF &outlineMm, const StitchSize &size)
{
    StitchGrid grid;
    if (outlineMm.size() < 3 || !size.isValid())
    {
        return grid;
    }

    const QRectF box = outlineMm.boundingRect();
    const qreal sw = size.stitchWidthMm;
    const qreal rh = size.rowHeightMm;
    const int rowCount = qMax(1, qRound(box.height() / rh));
    // Spread the rows evenly over the real height so the piece is not cut short by rounding.
    const qreal step = box.height() / rowCount;

    int minFirst = 0;
    bool haveFirst = false;
    for (int r = 0; r < rowCount; ++r)
    {
        qreal x0 = 0, x1 = 0;
        Row row;
        row.index = r;
        if (spanAt(outlineMm, box.top() + (r + 0.5) * step, x0, x1))
        {
            // Round the edge positions, not the width, so a straight edge never jitters by a stitch.
            row.firstStitch = qRound((x0 - box.left()) / sw);
            const int last = qRound((x1 - box.left()) / sw);
            row.count = qMax(1, last - row.firstStitch);
            if (!haveFirst || row.firstStitch < minFirst)
            {
                minFirst = row.firstStitch;
                haveFirst = true;
            }
        }
        grid.m_rows.append(row);
    }

    // Drop empty rows at either end, then renumber.
    while (!grid.m_rows.isEmpty() && grid.m_rows.first().count == 0)
    {
        grid.m_rows.removeFirst();
    }
    while (!grid.m_rows.isEmpty() && grid.m_rows.last().count == 0)
    {
        grid.m_rows.removeLast();
    }
    int maxEnd = 0;
    for (int i = 0; i < grid.m_rows.size(); ++i)
    {
        Row &row = grid.m_rows[i];
        row.index = i;
        row.firstStitch -= minFirst;
        maxEnd = qMax(maxEnd, row.end());
    }
    grid.m_width = maxEnd;
    return grid;
}

}
