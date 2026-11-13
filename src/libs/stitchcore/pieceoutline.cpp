#include "pieceoutline.h"

#include <QtMath>

namespace stitchcore
{

QPolygonF knittingOutline(const QPolygonF &sceneMm, const QLineF &grainline)
{
    if (sceneMm.isEmpty())
    {
        return QPolygonF();
    }

    qreal phi = 0;       // rotation that makes the grainline vertical
    if (!grainline.isNull() && grainline.length() > 0)
    {
        qreal dx = grainline.dx();
        qreal dy = grainline.dy();
        if (dy < 0 || (qFuzzyIsNull(dy) && dx < 0))
        {
            dx = -dx;           // direction does not matter, keep the smaller turn
            dy = -dy;
        }
        phi = M_PI / 2 - qAtan2(dy, dx);
    }
    const qreal c = qCos(phi);
    const qreal s = qSin(phi);

    QPolygonF out;
    out.reserve(sceneMm.size());
    for (const QPointF &p : sceneMm)
    {
        const qreal x = p.x() * c - p.y() * s;
        const qreal y = p.x() * s + p.y() * c;
        out << QPointF(x, -y);      // y up
    }
    const QRectF box = out.boundingRect();
    return out.translated(-box.left(), -box.top());
}

}
