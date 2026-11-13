#include "shaping.h"

namespace stitchcore
{

QVector<Shaping> planShaping(const StitchGrid &grid)
{
    QVector<Shaping> out;
    const QVector<Row> &rows = grid.rows();
    for (int i = 1; i < rows.size(); ++i)
    {
        const Row &p = rows.at(i - 1);
        const Row &c = rows.at(i);
        Shaping s;
        s.row = i;
        s.left = p.firstStitch - c.firstStitch;   // edge moved left = stitches added
        s.right = c.end() - p.end();
        s.stitchesAfter = c.count;
        if (s.left != 0 || s.right != 0)
        {
            out.append(s);
        }
    }
    return out;
}

QVector<Segment> planSegments(const StitchGrid &grid)
{
    QVector<Segment> segs;
    const QVector<Shaping> shaping = planShaping(grid);
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
