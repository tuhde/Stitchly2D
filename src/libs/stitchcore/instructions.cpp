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

QString runsText(const QVector<Run> &runs)
{
    QStringList parts;
    for (const Run &r : runs)
    {
        QString label = r.color;
        if (!r.special.isEmpty())
        {
            label += (label.isEmpty() ? QString() : QStringLiteral("/")) + r.special;
        }
        if (label.isEmpty())
        {
            label = QStringLiteral("-");   // base stitch, nothing laid over it
        }
        parts << QStringLiteral("%1 x%2").arg(label).arg(r.count);
    }
    return parts.join(QStringLiteral(", "));
}

bool isBulk(int change, const ShapingOptions &o)
{
    return o.bulkFrom > 0 && qAbs(change) >= o.bulkFrom;
}

QString edge(const QString &side, int change, const ShapingOptions &o)
{
    if (change == 0)
    {
        return QString();
    }
    const bool left = side == QLatin1String("left");
    const int n = qAbs(change);
    if (isBulk(change, o))
    {
        return QStringLiteral("%1 %2 stitches at %3 edge")
            .arg(change > 0 ? QStringLiteral("cast on") : QStringLiteral("bind off")).arg(n).arg(side);
    }
    const QString what = change > 0 ? QStringLiteral("increase %1 (%2)").arg(n).arg(incText(o.increase))
                                    : QStringLiteral("decrease %1 (%2)").arg(n).arg(decText(o.decrease, left));
    return QStringLiteral("%1 at %2 edge").arg(what, side);
}

/** What is done at the edges in a shaping row, e.g. "keep 1 edge stitch(es), increase 2 (M1) at left edge". */
QString shapingBody(const Shaping &s, const ShapingOptions &o)
{
    QStringList parts;
    const QString l = edge(QStringLiteral("left"), s.left, o);
    const QString r = edge(QStringLiteral("right"), s.right, o);
    if (!l.isEmpty()) parts << l;
    if (!r.isEmpty()) parts << r;
    const QString body = parts.join(QStringLiteral("; "));
    // Edge stitches belong to increases and decreases; casting on and binding off happen at the very edge.
    const bool plain = (s.left != 0 && !isBulk(s.left, o)) || (s.right != 0 && !isBulk(s.right, o));
    return plain ? QStringLiteral("keep %1 edge stitch(es), %2").arg(o.edgeStitches).arg(body) : body;
}

QString ordinal(int n)
{
    const int r100 = n % 100;
    const char *suffix = (r100 >= 11 && r100 <= 13) ? "th" : n % 10 == 1 ? "st" : n % 10 == 2 ? "nd" : n % 10 == 3 ? "rd" : "th";
    return QStringLiteral("%1%2").arg(n).arg(QLatin1String(suffix));
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
    const QVector<Segment> segments = planSegments(grid);
    for (int si = 0; si < segments.size(); ++si)
    {
        const Segment &s = segments.at(si);
        const int first = s.firstRow + 1;

        // The same edge change again and again at a regular distance is written once ("every 4th row, 6 times").
        // Not where a pattern is laid over the rows: those rows are written one by one.
        if (s.shaped && !s.shaping.bandChange)
        {
            QVector<int> run{si};
            int gap = 0;
            for (int j = si + 1; j < segments.size(); ++j)
            {
                const Segment &t = segments.at(j);
                if (!t.shaped)
                {
                    continue;
                }
                const int g = t.shaping.row - segments.at(run.last()).shaping.row;
                if (t.shaping.bandChange || t.shaping.left != s.shaping.left || t.shaping.right != s.shaping.right
                    || (gap != 0 && g != gap))
                {
                    break;
                }
                gap = g;
                run << j;
            }
            const int lastRow = segments.at(run.last()).shaping.row;
            bool plainRows = true;
            for (int r = s.shaping.row; r <= lastRow && plainRows; ++r)
            {
                plainRows = grid.overlayRuns(r).isEmpty();
            }
            if (run.size() >= 3 && plainRows)
            {
                const Shaping &z = segments.at(run.last()).shaping;
                const QString cadence = gap == 1 ? QStringLiteral("every row")
                                       : gap == 2 ? QStringLiteral("every other row")
                                                  : QStringLiteral("every %1 row").arg(ordinal(gap));
                const QString type = multi ? QStringLiteral(" (%1)").arg(grid.bands().at(grid.rows().at(s.shaping.row).band).stitchType)
                                           : QString();
                lines << QStringLiteral("Rows %1-%2%3: %4 (%5 times) starting with row %1: %6%7. (%8 -> %9 stitches)")
                             .arg(s.shaping.row + 1).arg(lastRow + 1).arg(type).arg(cadence).arg(run.size())
                             .arg(shapingBody(s.shaping, options))
                             .arg(gap == 1 ? QString() : QStringLiteral("; work the other rows even"))
                             .arg(s.shaping.stitchesAfter - s.shaping.left - s.shaping.right).arg(z.stitchesAfter);
                si = run.last();
                continue;
            }
        }
        if (!s.shaped)
        {
            // Rows with an overlay are written one by one; plain rows are grouped.
            int blockStart = s.firstRow;
            auto flush = [&](int endRow) {
                const int n = endRow - blockStart;
                if (n <= 0) return;
                const QString type = multi ? QStringLiteral(" (%1)").arg(grid.bands().at(grid.rows().at(blockStart).band).stitchType)
                                           : QString();
                if (n == 1)
                    lines << QStringLiteral("Row %1%2: work even.").arg(blockStart + 1).arg(type);
                else
                    lines << QStringLiteral("Rows %1-%2%3: work even.").arg(blockStart + 1).arg(endRow).arg(type);
            };
            for (int r = s.firstRow; r < s.firstRow + s.rows; ++r)
            {
                const QVector<Run> runs = grid.overlayRuns(r);
                if (runs.isEmpty()) continue;
                flush(r);
                const QString type = multi ? QStringLiteral(" (%1)").arg(grid.bands().at(grid.rows().at(r).band).stitchType)
                                           : QString();
                lines << QStringLiteral("Row %1%2: %3.").arg(r + 1).arg(type).arg(runsText(runs));
                blockStart = r + 1;
            }
            flush(s.firstRow + s.rows);
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
            const QVector<Run> bruns = grid.overlayRuns(s.firstRow);
            if (!bruns.isEmpty())
            {
                lines << QStringLiteral("    pattern: %1.").arg(runsText(bruns));
            }
            continue;
        }
        lines << QStringLiteral("Row %1: %2. (%3 stitches)").arg(first).arg(shapingBody(s.shaping, options))
                     .arg(s.shaping.stitchesAfter);
        const QVector<Run> eruns = grid.overlayRuns(s.firstRow);
        if (!eruns.isEmpty())
        {
            lines << QStringLiteral("    pattern: %1.").arg(runsText(eruns));
        }
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
