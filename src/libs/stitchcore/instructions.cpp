#include "instructions.h"

#include <QCoreApplication>

namespace stitchcore
{

namespace
{
QString incText(IncreaseMethod m)
{
    switch (m)
    {
        case IncreaseMethod::M1:       return QStringLiteral("M1");
        case IncreaseMethod::YarnOver: return QStringLiteral("yo");
        case IncreaseMethod::KFB:      return QStringLiteral("kfb");
    }
    return QString();
}

QString decText(DecreaseMethod m, bool leftEdge)
{
    if (m == DecreaseMethod::K2tog)
    {
        return QStringLiteral("k2tog");
    }
    return leftEdge ? QStringLiteral("ssk") : QStringLiteral("k2tog");
}

QString edge(const QString &side, int change, const ShapingOptions &o)
{
    if (change == 0)
    {
        return QString();
    }
    const bool left = side == QLatin1String("left");
    const int n = qAbs(change);
    const QString what = change > 0 ? QStringLiteral("increase %1 (%2)").arg(n).arg(incText(o.increase))
                                    : QStringLiteral("decrease %1 (%2)").arg(n).arg(decText(o.decrease, left));
    return QStringLiteral("%1 at %2 edge").arg(what, side);
}
}

QStringList writeInstructions(const StitchGrid &grid, const ShapingOptions &options)
{
    QStringList lines;
    if (grid.isEmpty())
    {
        return lines;
    }
    lines << QStringLiteral("Cast on %1 stitches.").arg(grid.castOn());
    for (const Segment &s : planSegments(grid))
    {
        const int first = s.firstRow + 1;
        if (!s.shaped)
        {
            if (s.rows == 1)
            {
                lines << QStringLiteral("Row %1: work even.").arg(first);
            }
            else
            {
                lines << QStringLiteral("Rows %1-%2: work even.").arg(first).arg(first + s.rows - 1);
            }
            continue;
        }
        QStringList parts;
        const QString l = edge(QStringLiteral("left"), s.shaping.left, options);
        const QString r = edge(QStringLiteral("right"), s.shaping.right, options);
        if (!l.isEmpty()) parts << l;
        if (!r.isEmpty()) parts << r;
        lines << QStringLiteral("Row %1: keep %2 edge stitch(es), %3. (%4 stitches)")
                     .arg(first).arg(options.edgeStitches).arg(parts.join(QStringLiteral("; ")))
                     .arg(s.shaping.stitchesAfter);
    }
    lines << QStringLiteral("Bind off %1 stitches.").arg(grid.bindOff());
    return lines;
}

QString rowTableCsv(const StitchGrid &grid)
{
    QString csv = QStringLiteral("row;first stitch;count;change left;change right\n");
    const QVector<Row> &rows = grid.rows();
    for (int i = 0; i < rows.size(); ++i)
    {
        const Row &r = rows.at(i);
        int dl = 0, dr = 0;
        if (i > 0)
        {
            dl = rows.at(i - 1).firstStitch - r.firstStitch;
            dr = r.end() - rows.at(i - 1).end();
        }
        csv += QStringLiteral("%1;%2;%3;%4;%5\n").arg(i + 1).arg(r.firstStitch).arg(r.count).arg(dl).arg(dr);
    }
    return csv;
}

}
