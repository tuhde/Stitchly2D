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
    const bool multi = grid.bands().size() > 1;
    lines << QStringLiteral("Cast on %1 stitches%2.").arg(grid.castOn())
                 .arg(multi ? QStringLiteral(" and work in %1").arg(grid.bands().first().stitchType) : QString());
    for (const Segment &s : planSegments(grid))
    {
        const int first = s.firstRow + 1;
        if (!s.shaped)
        {
            const QString type = multi ? QStringLiteral(" (%1)").arg(grid.bands().at(grid.rows().at(s.firstRow).band).stitchType)
                                       : QString();
            if (s.rows == 1)
            {
                lines << QStringLiteral("Row %1%2: work even.").arg(first).arg(type);
            }
            else
            {
                lines << QStringLiteral("Rows %1-%2%3: work even.").arg(first).arg(first + s.rows - 1).arg(type);
            }
            continue;
        }
        if (s.shaping.bandChange)
        {
            const QString &name = grid.bands().at(s.shaping.band).stitchType;
            if (s.shaping.stitchesBefore == s.shaping.stitchesAfter)
            {
                lines << QStringLiteral("Row %1: change to %2 (%3 stitches).")
                             .arg(first).arg(name).arg(s.shaping.stitchesAfter);
            }
            else
            {
                const int d = s.shaping.stitchesAfter - s.shaping.stitchesBefore;
                lines << QStringLiteral("Row %1: change to %2 and %3 %4 stitches evenly across the row (%5 -> %6 stitches).")
                             .arg(first).arg(name)
                             .arg(d > 0 ? QStringLiteral("increase") : QStringLiteral("decrease"))
                             .arg(qAbs(d)).arg(s.shaping.stitchesBefore).arg(s.shaping.stitchesAfter);
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
    QString csv = QStringLiteral("row;band;stitch type;first stitch;count;change left;change right\n");
    const QVector<Row> &rows = grid.rows();
    for (int i = 0; i < rows.size(); ++i)
    {
        const Row &r = rows.at(i);
        int dl = 0, dr = 0;
        if (i > 0 && rows.at(i - 1).band == r.band)
        {
            dl = rows.at(i - 1).firstStitch - r.firstStitch;
            dr = r.end() - rows.at(i - 1).end();
        }
        csv += QStringLiteral("%1;%2;%3;%4;%5;%6;%7\n").arg(i + 1).arg(r.band + 1).arg(grid.bands().at(r.band).stitchType).arg(r.firstStitch).arg(r.count).arg(dl).arg(dr);
    }
    return csv;
}

}
