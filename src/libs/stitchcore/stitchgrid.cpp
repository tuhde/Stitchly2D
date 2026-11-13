#include "stitchgrid.h"

#include <QtMath>

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

struct Resolved
{
    QString    name;
    StitchSize size;
    qreal      y0 = 0;
    qreal      y1 = 0;
};
}

StitchGrid StitchGrid::build(const QPolygonF &outlineMm, const StitchSize &size)
{
    StitchLibrary lib;
    lib.set({QStringLiteral("Stitch"), size});
    return build(outlineMm, QVector<BandSpec>{BandSpec{QStringLiteral("Stitch"), 0}}, lib);
}

StitchGrid StitchGrid::build(const QPolygonF &outlineMm, const QVector<BandSpec> &bands,
                             const StitchLibrary &library)
{
    StitchGrid grid;
    if (outlineMm.size() < 3 || bands.isEmpty())
    {
        return grid;
    }

    const QRectF box = outlineMm.boundingRect();

    // Resolve the bands to y ranges. The last band is stretched to the top.
    QVector<Resolved> res;
    qreal y = box.top();
    for (int i = 0; i < bands.size(); ++i)
    {
        const StitchType t = library.find(bands.at(i).stitchType);
        if (t.name.isEmpty() || !t.size.isValid())
        {
            return StitchGrid();
        }
        const bool last = i == bands.size() - 1 || bands.at(i).heightMm <= 0;
        Resolved r;
        r.name = t.name;
        r.size = t.size;
        r.y0 = y;
        r.y1 = last ? box.bottom() : qMin(box.bottom(), y + bands.at(i).heightMm);
        res.append(r);
        y = r.y1;
        if (last || y >= box.bottom())
        {
            break;
        }
    }

    for (int b = 0; b < res.size(); ++b)
    {
        const Resolved &r = res.at(b);
        const qreal h = r.y1 - r.y0;
        if (h <= 0)
        {
            continue;
        }
        const int rowCount = qMax(1, qRound(h / r.size.rowHeightMm));
        // Spread the rows evenly over the band so rounding does not shift the band edges.
        const qreal step = h / rowCount;
        const qreal sw = r.size.stitchWidthMm;

        QVector<Row> rows;
        int minFirst = 0;
        bool haveFirst = false;
        for (int i = 0; i < rowCount; ++i)
        {
            qreal x0 = 0, x1 = 0;
            Row row;
            row.band = b;
            if (spanAt(outlineMm, r.y0 + (i + 0.5) * step, x0, x1))
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
            rows.append(row);
        }
        for (Row &row : rows)
        {
            if (row.count > 0)
            {
                row.firstStitch -= minFirst;
            }
            grid.m_rows.append(row);
        }
    }

    // Drop empty rows at either end, then number the rows and fill in the bands.
    while (!grid.m_rows.isEmpty() && grid.m_rows.first().count == 0)
    {
        grid.m_rows.removeFirst();
    }
    while (!grid.m_rows.isEmpty() && grid.m_rows.last().count == 0)
    {
        grid.m_rows.removeLast();
    }
    int previous = -1;
    for (int i = 0; i < grid.m_rows.size(); ++i)
    {
        Row &row = grid.m_rows[i];
        const int original = row.band;
        row.index = i;
        grid.m_width = qMax(grid.m_width, row.end());
        if (original != previous)
        {
            previous = original;
            GridBand gb;
            gb.stitchType = res.at(original).name;
            gb.size = res.at(original).size;
            gb.firstRow = i;
            grid.m_bands.append(gb);
        }
        grid.m_bands.last().rowCount++;
        // Band index in rows refers to the final band list.
        row.band = grid.m_bands.size() - 1;
    }
    return grid;
}

}
