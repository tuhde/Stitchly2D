#include "passes.h"

#include <QCoreApplication>

namespace stitchcore
{

namespace
{
struct PassWords
{
    Q_DECLARE_TR_FUNCTIONS(Passes)
};
}

RowPasses passesForRow(const StitchGrid &grid, KnitMethod method, int gridRow)
{
    RowPasses out;
    if (gridRow < 0 || gridRow >= grid.rowCount())
    {
        return out;
    }
    // The yarns of the row: the colours of the pattern laid over it. Stitches without a colour use the base yarn.
    const QVector<Run> runs = grid.overlayRuns(gridRow);
    bool anyColor = false;
    for (const Run &r : runs)
    {
        anyColor = anyColor || !r.color.isEmpty();
    }
    if (anyColor)
    {
        // A fixed order from row to row (the base yarn first, then the colours in the order of the pattern's legend),
        // not the order in which they happen to appear in the row.
        QStringList found;
        bool base = false;
        for (const Run &r : runs)
        {
            if (r.color.isEmpty()) base = true;
            else if (!found.contains(r.color)) found << r.color;
        }
        if (base)
        {
            out.yarns << PassWords::tr("main", "yarn of the stitches that have no colour");
        }
        const Row &row = grid.rows().at(gridRow);
        if (row.band >= 0 && row.band < grid.bands().size())
        {
            for (const ResolvedOverlay &o : grid.bands().at(row.band).overlays)
            {
                if (o.pattern.kind != PatternDef::Color) continue;
                for (const QString &color : o.pattern.legend)
                {
                    if (found.contains(color) && !out.yarns.contains(color)) out.yarns << color;
                }
            }
        }
        for (const QString &color : qAsConst(found))
        {
            if (!out.yarns.contains(color)) out.yarns << color;
        }
    }

    const QStringList yarns = out.yarns.isEmpty() ? QStringList{QString()} : out.yarns;
    out.passes.clear();
    switch (method)
    {
        case KnitMethod::DoubleBedMachine:
            for (const QString &y : yarns)
            {
                out.passes.append({y, Bed::Front});
                out.passes.append({y, Bed::Rear});
            }
            break;
        case KnitMethod::DoubleBedFlat:
            for (const QString &y : yarns)
            {
                out.passes.append({y, Bed::None});
            }
            break;
        default:
            out.passes.append(RowPass());
            break;
    }
    return out;
}

QString passesText(const RowPasses &passes, int current, bool rich)
{
    auto mark = [&](const QString &text, bool isCurrent) {
        const QString t = rich ? text.toHtmlEscaped() : text;
        return rich && isCurrent ? QStringLiteral("<b><u>%1</u></b>").arg(t) : t;
    };
    bool anyBed = false;
    for (const RowPass &p : passes.passes)
    {
        anyBed = anyBed || p.bed != Bed::None;
    }
    if (!anyBed && passes.passes.size() == 1)
    {
        // One pass: just name the yarns of the row (if any)
        QStringList names;
        for (const QString &y : passes.yarns) names << mark(y, false);
        return names.join(QLatin1Char(','));
    }

    // Group the passes by yarn, keeping their order
    QStringList groups;
    QVector<QString> order;
    for (const RowPass &p : passes.passes)
    {
        if (!order.contains(p.yarn)) order << p.yarn;
    }
    for (const QString &yarn : qAsConst(order))
    {
        QStringList beds;
        bool yarnIsCurrent = false;
        for (int i = 0; i < passes.passes.size(); ++i)
        {
            const RowPass &p = passes.passes.at(i);
            if (p.yarn != yarn) continue;
            if (p.bed == Bed::None)
            {
                yarnIsCurrent = yarnIsCurrent || i == current;
            }
            else
            {
                beds << mark(p.bed == Bed::Front ? PassWords::tr("front", "bed of the machine")
                                                 : PassWords::tr("rear", "bed of the machine"), i == current);
            }
        }
        QString group;
        if (!yarn.isEmpty()) group = mark(yarn, yarnIsCurrent);
        if (!beds.isEmpty()) group += (group.isEmpty() ? QString() : QStringLiteral(" ")) + beds.join(QLatin1Char('/'));
        groups << group;
    }
    return groups.join(QLatin1Char(','));
}

}
