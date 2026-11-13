#include "shaping.h"

namespace stitchcore
{

QString knitMethodName(KnitMethod method)
{
    switch (method)
    {
        case KnitMethod::Flat:             return QStringLiteral("flat");
        case KnitMethod::Round:            return QStringLiteral("round");
        case KnitMethod::CircularMachine:  return QStringLiteral("circular_machine");
        case KnitMethod::DoubleBedMachine: return QStringLiteral("double_bed_machine");
    }
    return QStringLiteral("flat");
}

bool knitMethodFromName(const QString &name, KnitMethod &method)
{
    for (KnitMethod m : {KnitMethod::Flat, KnitMethod::Round, KnitMethod::CircularMachine, KnitMethod::DoubleBedMachine})
    {
        if (name == knitMethodName(m))
        {
            method = m;
            return true;
        }
    }
    return false;
}

QVector<Shaping> planShaping(const StitchGrid &grid, ShapingRows shapeOn)
{
    QVector<Shaping> out;
    const QVector<Row> &rows = grid.rows();
    const int n = rows.size();
    if (n < 2)
    {
        return out;
    }
    // What the outline asks for on each row; band changes are kept apart.
    QVector<int> left(n, 0), right(n, 0);
    QVector<bool> bandChange(n, false);
    for (int i = 1; i < n; ++i)
    {
        const Row &p = rows.at(i - 1);
        const Row &c = rows.at(i);
        if (c.band != p.band)
        {
            bandChange[i] = true;
            continue;
        }
        left[i] = p.firstStitch - c.firstStitch;      // edge moved left = stitches added
        right[i] = c.end() - p.end();
    }
    if (shapeOn == ShapingRows::OutsideRows)
    {
        for (int i = 1; i < n; i += 2)                // the inside rows: rows 2, 4, ... (indices 1, 3, ...)
        {
            if (left[i] == 0 && right[i] == 0)
            {
                continue;
            }
            int target = -1;
            if (i - 1 >= 1 && !bandChange[i - 1])
                target = i - 1;                       // the outside row before
            else if (i + 1 < n && !bandChange[i + 1])
                target = i + 1;                       // the first row has no shaping row before it
            if (target >= 0)
            {
                left[target] += left[i];
                right[target] += right[i];
                left[i] = right[i] = 0;
            }
        }
    }
    int running = rows.first().count;
    for (int i = 1; i < n; ++i)
    {
        Shaping s;
        s.row = i;
        s.stitchesBefore = running;
        if (bandChange[i])
        {
            s.bandChange = true;
            s.band = rows.at(i).band;
            running = rows.at(i).count;
            s.stitchesAfter = running;
            out.append(s);
        }
        else if (left[i] != 0 || right[i] != 0)
        {
            s.left = left[i];
            s.right = right[i];
            running += left[i] + right[i];
            s.stitchesAfter = running;
            out.append(s);
        }
    }
    return out;
}

QVector<int> plannedCounts(const StitchGrid &grid, ShapingRows shapeOn)
{
    QVector<int> counts(grid.rowCount(), 0);
    if (grid.isEmpty())
    {
        return counts;
    }
    int running = grid.rows().first().count;
    int next = 0;
    const QVector<Shaping> plan = planShaping(grid, shapeOn);
    for (int i = 0; i < grid.rowCount(); ++i)
    {
        if (next < plan.size() && plan.at(next).row == i)
        {
            running = plan.at(next).stitchesAfter;
            ++next;
        }
        counts[i] = running;
    }
    return counts;
}

QVector<Segment> planSegments(const StitchGrid &grid, ShapingRows shapeOn)
{
    QVector<Segment> segs;
    const QVector<Shaping> shaping = planShaping(grid, shapeOn);
    int next = 0;
    for (const Shaping &s : shaping)
    {
        if (s.row > next)
        {
            Segment even;
            even.firstRow = next;
            even.rows = s.row - next;
            segs.append(even);
        }
        Segment sh;
        sh.firstRow = s.row;
        sh.rows = 1;
        sh.shaped = true;
        sh.shaping = s;
        segs.append(sh);
        next = s.row + 1;
    }
    if (next < grid.rowCount())
    {
        Segment even;
        even.firstRow = next;
        even.rows = grid.rowCount() - next;
        segs.append(even);
    }
    return segs;
}

}
