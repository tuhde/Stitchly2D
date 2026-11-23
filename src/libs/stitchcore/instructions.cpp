#include "instructions.h"

#include <QCoreApplication>
#include <QHash>
#include <QPair>

namespace stitchcore
{

namespace
{
/**
 * Translations of the written instructions. The library does not depend on a GUI, so the strings are looked up with
 * QCoreApplication::translate (context "Instructions"): without a translator the English text is returned.
 */
struct Instructions
{
    Q_DECLARE_TR_FUNCTIONS(Instructions)
};

/**
 * "row" or "round": flat knitting works in rows, a tube is knitted in rounds. The words are filled into the sentences,
 * so the sentences exist only once (in both languages "row" and "round" have the same grammatical gender).
 */
struct Words
{
    QString row, Row, rows, Rows;
    explicit Words(bool round)
    {
        if (round)
        {
            row = Instructions::tr("round", "word for row in a tube");
            Row = Instructions::tr("Round", "word for row in a tube");
            rows = Instructions::tr("rounds", "word for row in a tube");
            Rows = Instructions::tr("Rounds", "word for row in a tube");
        }
        else
        {
            row = Instructions::tr("row", "word for row in flat knitting");
            Row = Instructions::tr("Row", "word for row in flat knitting");
            rows = Instructions::tr("rows", "word for row in flat knitting");
            Rows = Instructions::tr("Rows", "word for row in flat knitting");
        }
    }
};

QString incText(IncreaseMethod m)
{
    switch (m)
    {
        case IncreaseMethod::M1:       return Instructions::tr("M1", "increase");
        case IncreaseMethod::YarnOver: return Instructions::tr("yo", "increase");
        case IncreaseMethod::KFB:      return Instructions::tr("kfb", "increase");
    }
    return QString();
}

QString decText(DecreaseMethod m, bool leftEdge)
{
    if (m == DecreaseMethod::K2tog)
    {
        return Instructions::tr("k2tog", "decrease");
    }
    return leftEdge ? Instructions::tr("ssk", "decrease") : Instructions::tr("k2tog", "decrease");
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

/** Where an edge change happens: at the edge of a flat piece, at the beginning or the end of a round. */
QString whereText(const QString &side, KnitMethod m)
{
    const bool left = side == QLatin1String("left");
    if (m == KnitMethod::Flat)
    {
        return left ? Instructions::tr("left edge") : Instructions::tr("right edge");
    }
    return left ? Instructions::tr("the beginning of the round") : Instructions::tr("the end of the round");
}

QString edge(const QString &side, int change, const ShapingOptions &o)
{
    if (change == 0)
    {
        return QString();
    }
    const bool left = side == QLatin1String("left");
    const int n = qAbs(change);
    const QString where = whereText(side, o.method);
    const QString inc = Instructions::tr("increase %1 (%2)").arg(n).arg(incText(o.increase));
    const QString dec = Instructions::tr("decrease %1 (%2)").arg(n).arg(decText(o.decrease, left));
    switch (o.method)
    {
        case KnitMethod::Flat:
            if (isBulk(change, o))
            {
                return change > 0 ? Instructions::tr("cast on %1 stitches at %2").arg(n).arg(where)
                                  : Instructions::tr("bind off %1 stitches at %2").arg(n).arg(where);
            }
            return Instructions::tr("%1 at %2").arg(change > 0 ? inc : dec, where);

        case KnitMethod::Round:
            // A tube has no edge to cast on or bind off at: a large change is spread over the round.
            if (isBulk(change, o))
            {
                return Instructions::tr("%1 evenly across the round").arg(change > 0 ? inc : dec);
            }
            return Instructions::tr("%1 at %2").arg(change > 0 ? inc : dec, where);

        case KnitMethod::CircularMachine:
            if (change > 0)
            {
                return Instructions::tr("bring %1 needle(s) into work at %2").arg(n).arg(where);
            }
            return Instructions::tr("transfer %1 stitch(es) to the neighbouring needle(s) and take the empty needle(s) "
                                    "out of work at %2").arg(n).arg(where);

        case KnitMethod::DoubleBedMachine:
        {
            const QString beds = Instructions::tr("front bed %1, back bed %2").arg((n + 1) / 2).arg(n / 2);
            if (change > 0)
            {
                return Instructions::tr("bring %1 needle(s) into work at %2 (%3)").arg(n).arg(where, beds);
            }
            return Instructions::tr("transfer %1 stitch(es) inward at %2 (%3) and take the empty needle(s) out of work")
                .arg(n).arg(where, beds);
        }
    }
    return QString();
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
    // On a machine the needles at the edge are simply the ones that are moved.
    const bool plain = !isMachine(o.method)
                       && ((s.left != 0 && !isBulk(s.left, o)) || (s.right != 0 && !isBulk(s.right, o)));
    return plain ? Instructions::tr("keep %1 edge stitch(es), %2").arg(o.edgeStitches).arg(body) : body;
}

/** "4th": the English suffixes are a translation too (German writes "4."). */
QString ordinal(int n)
{
    const int r100 = n % 100;
    QString text;
    if (r100 >= 11 && r100 <= 13)  text = Instructions::tr("%1th", "ordinal number");
    else if (n % 10 == 1)          text = Instructions::tr("%1st", "ordinal number");
    else if (n % 10 == 2)          text = Instructions::tr("%1nd", "ordinal number");
    else if (n % 10 == 3)          text = Instructions::tr("%1rd", "ordinal number");
    else                           text = Instructions::tr("%1th", "ordinal number");
    return text.arg(n);
}

/** " (stitch type)" after a row number when the piece has more than one band. */
QString typeSuffix(const StitchGrid &grid, int row, bool multi)
{
    return multi ? QStringLiteral(" (%1)").arg(grid.bands().at(grid.rows().at(row).band).stitchType) : QString();
}
}

namespace
{
/** Collects the lines; the rows to attach to the next lines are set with rows(). */
struct StepSink
{
    QVector<InstructionStep> steps;
    int first = 0;
    int last  = 0;
    void rows(int f, int l) { first = f; last = l; }
    StepSink &operator<<(const QString &text) { steps.append({first, last, text}); return *this; }
};
}

QVector<InstructionStep> writeInstructionSteps(const StitchGrid &grid, const ShapingOptions &options)
{
    StepSink lines;
    if (grid.isEmpty())
    {
        return lines.steps;
    }
    const Words w(isRound(options.method));   // a tube is knitted in rounds, not in rows
    const bool multi = grid.bands().size() > 1;
    const QString startType = multi ? grid.bands().first().stitchType : QString();
    const int castOn = grid.castOn();
    switch (options.method)
    {
        case KnitMethod::Flat:
            lines << Instructions::tr("Cast on %1 stitches%2.").arg(castOn)
                         .arg(multi ? Instructions::tr(" and work in %1").arg(startType) : QString());
            break;
        case KnitMethod::Round:
            lines << Instructions::tr("Cast on %1 stitches%2. Join in the round without twisting the stitches and place a "
                                      "marker at the beginning of the round.").arg(castOn)
                         .arg(multi ? Instructions::tr(" and work in %1").arg(startType) : QString());
            break;
        case KnitMethod::CircularMachine:
            lines << Instructions::tr("Put %1 needles in work and cast on%2. Knit in rounds.").arg(castOn)
                         .arg(multi ? Instructions::tr(" (work in %1)").arg(startType) : QString());
            break;
        case KnitMethod::DoubleBedMachine:
            lines << Instructions::tr("Cast on %1 stitches tubular: %2 needles on the front bed and %3 on the back bed%4. "
                                      "One round is two carriage passes: front bed, then back bed.")
                         .arg(castOn).arg((castOn + 1) / 2).arg(castOn / 2)
                         .arg(multi ? Instructions::tr(" (work in %1)").arg(startType) : QString());
            break;
    }
    // Does the piece fit the machine?
    if (isMachine(options.method) && options.machineNeedles > 0)
    {
        int widest = 0;
        for (int c : plannedCounts(grid, options.effectiveShapeOn())) widest = qMax(widest, c);
        const int needed = options.method == KnitMethod::DoubleBedMachine ? (widest + 1) / 2 : widest;
        if (needed > options.machineNeedles)
        {
            lines << Instructions::tr("Warning: the piece needs %1 needles%2, the machine has %3.")
                         .arg(needed).arg(options.method == KnitMethod::DoubleBedMachine ? Instructions::tr(" per bed") : QString())
                         .arg(options.machineNeedles);
        }
    }
    const QVector<Segment> segments = planSegments(grid, options.effectiveShapeOn());
    bool anyShaping = false;
    for (const Segment &t : segments) anyShaping = anyShaping || (t.shaped && !t.shaping.bandChange);
    if (anyShaping && options.method == KnitMethod::Flat && options.shapeOn == ShapingRows::OutsideRows)
    {
        lines << Instructions::tr("Row 1 is an outside row. Increases and decreases are worked on outside rows only.");
    }
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
                lines.rows(s.shaping.row + 1, lastRow + 1);
                const QString cadence = gap == 1 ? Instructions::tr("every %1").arg(w.row)
                                       : gap == 2 ? Instructions::tr("every other %1").arg(w.row)
                                                  : Instructions::tr("every %1 %2").arg(ordinal(gap), w.row);
                lines << Instructions::tr("%1 %2-%3%4: %5 (%6 times) starting with %7 %2: %8%9. (%10 -> %11 stitches)")
                             .arg(w.Rows).arg(s.shaping.row + 1).arg(lastRow + 1).arg(typeSuffix(grid, s.shaping.row, multi))
                             .arg(cadence).arg(run.size()).arg(w.row)
                             .arg(shapingBody(s.shaping, options))
                             .arg(gap == 1 ? QString() : Instructions::tr("; work the other %1 even").arg(w.rows))
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
                lines.rows(blockStart + 1, endRow);
                if (n == 1)
                    lines << Instructions::tr("%1 %2%3: work even.").arg(w.Row).arg(blockStart + 1)
                                 .arg(typeSuffix(grid, blockStart, multi));
                else
                    lines << Instructions::tr("%1 %2-%3%4: work even.").arg(w.Rows).arg(blockStart + 1).arg(endRow)
                                 .arg(typeSuffix(grid, blockStart, multi));
            };
            for (int r = s.firstRow; r < s.firstRow + s.rows; ++r)
            {
                const QVector<Run> runs = grid.overlayRuns(r);
                if (runs.isEmpty()) continue;
                flush(r);
                lines.rows(r + 1, r + 1);
                lines << Instructions::tr("%1 %2%3: %4.").arg(w.Row).arg(r + 1).arg(typeSuffix(grid, r, multi))
                             .arg(runsText(runs));
                blockStart = r + 1;
            }
            flush(s.firstRow + s.rows);
            continue;
        }
        if (s.shaping.bandChange)
        {
            const QString &name = grid.bands().at(s.shaping.band).stitchType;
            lines.rows(first, first);
            if (s.shaping.stitchesBefore == s.shaping.stitchesAfter)
            {
                lines << Instructions::tr("%1 %2: change to %3 (%4 stitches).")
                             .arg(w.Row).arg(first).arg(name).arg(s.shaping.stitchesAfter);
            }
            else
            {
                const int d = s.shaping.stitchesAfter - s.shaping.stitchesBefore;
                lines << (d > 0 ? Instructions::tr("%1 %2: change to %3 and increase %4 stitches evenly across the %5 "
                                                   "(%6 -> %7 stitches).")
                                : Instructions::tr("%1 %2: change to %3 and decrease %4 stitches evenly across the %5 "
                                                   "(%6 -> %7 stitches)."))
                             .arg(w.Row).arg(first).arg(name).arg(qAbs(d)).arg(w.row)
                             .arg(s.shaping.stitchesBefore).arg(s.shaping.stitchesAfter);
            }
            const QVector<Run> bruns = grid.overlayRuns(s.firstRow);
            if (!bruns.isEmpty())
            {
                lines << Instructions::tr("    pattern: %1.").arg(runsText(bruns));
            }
            continue;
        }
        lines.rows(first, first);
        lines << Instructions::tr("%1 %2: %3. (%4 stitches)").arg(w.Row).arg(first).arg(shapingBody(s.shaping, options))
                     .arg(s.shaping.stitchesAfter);
        const QVector<Run> eruns = grid.overlayRuns(s.firstRow);
        if (!eruns.isEmpty())
        {
            lines << Instructions::tr("    pattern: %1.").arg(runsText(eruns));
        }
    }
    lines.rows(grid.rowCount() + 1, grid.rowCount() + 1);
    switch (options.method)
    {
        case KnitMethod::CircularMachine:
            lines << Instructions::tr("Take the work off the machine: bind off %1 stitches.").arg(grid.bindOff());
            break;
        case KnitMethod::DoubleBedMachine:
            lines << Instructions::tr("Take the work off both beds: bind off %1 stitches.").arg(grid.bindOff());
            break;
        default:
            lines << Instructions::tr("Bind off %1 stitches.").arg(grid.bindOff());
            break;
    }
    return lines.steps;
}

QStringList writeInstructions(const StitchGrid &grid, const ShapingOptions &options)
{
    QStringList lines;
    for (const InstructionStep &s : writeInstructionSteps(grid, options))
    {
        lines << s.text;
    }
    return lines;
}

QVector<int> stepsForRow(const QVector<InstructionStep> &steps, int row)
{
    QVector<int> found;
    for (int i = 0; i < steps.size(); ++i)
    {
        if (steps.at(i).firstRow <= row && row <= steps.at(i).lastRow)
        {
            found << i;
        }
    }
    return found;
}

QString rowTableCsv(const StitchGrid &grid, const ShapingOptions &options)
{
    // The CSV is meant for other programs: its header stays English in every language.
    QString csv = QStringLiteral("row;band;stitch type;first stitch;count;change left;change right\n");
    const QVector<Row> &rows = grid.rows();
    const QVector<int> counts = plannedCounts(grid, options.effectiveShapeOn());
    QHash<int, Shaping> plan;
    for (const Shaping &s : planShaping(grid, options.effectiveShapeOn())) plan.insert(s.row, s);
    for (int i = 0; i < rows.size(); ++i)
    {
        const Row &r = rows.at(i);
        const Shaping s = plan.value(i);              // no entry = no change
        csv += QStringLiteral("%1;%2;%3;%4;%5;%6;%7\n").arg(i + 1).arg(r.band + 1).arg(grid.bands().at(r.band).stitchType)
                   .arg(r.firstStitch).arg(counts.at(i)).arg(s.bandChange ? 0 : s.left).arg(s.bandChange ? 0 : s.right);
    }
    return csv;
}

int nextStepRow(const QVector<InstructionStep> &steps, int row)
{
    int best = row;
    for (const InstructionStep &s : steps)
    {
        if (s.firstRow > row && (best == row || s.firstRow < best))
        {
            best = s.firstRow;
        }
    }
    return best;
}

int previousStepRow(const QVector<InstructionStep> &steps, int row)
{
    int best = row;
    for (const InstructionStep &s : steps)
    {
        if (s.firstRow < row && (best == row || s.firstRow > best))
        {
            best = s.firstRow;
        }
    }
    return best;
}

}
