#include "yarn_estimate.h"

#include "project.h"        // kDefaultStitchName

namespace stitchcore
{

const char *const kMainYarnSlot = "main";

QMap<QString, int> stitchesPerSlot(const StitchGrid &grid)
{
    QMap<QString, int> total;
    for (const Row &row : grid.rows())
    {
        int coloured = 0;
        for (const Run &run : grid.overlayRuns(row.index))
        {
            if (!run.color.isEmpty())
            {
                total[run.color] += run.count;
                coloured += run.count;
            }
        }
        total[QString::fromLatin1(kMainYarnSlot)] += qMax(0, row.count - coloured);
    }
    return total;
}

qreal yarnForStitches(const Yarn &yarn, const QString &stitchType, const StitchSize &size, int stitches)
{
    const YarnGauge *g = yarn.gaugeFor(stitchType);
    const qreal perStitch = g && g->yarnPerStitchMm > 0 ? g->yarnPerStitchMm : kDefaultYarnPerStitchWidths * size.stitchWidthMm;
    return perStitch * stitches / 1000.0;
}

QVector<YarnUse> yarnUses(const StitchGrid &grid, const QString &defaultStitchType)
{
    QVector<YarnUse> uses;
    const QString mainSlot = QString::fromLatin1(kMainYarnSlot);
    auto add = [&uses](const QString &slot, const QString &type, const StitchSize &size, int count) {
        if (count <= 0) return;
        for (YarnUse &u : uses)
        {
            if (u.slot == slot && u.stitchType == type && qFuzzyCompare(u.size.stitchWidthMm, size.stitchWidthMm))
            {
                u.stitches += count;
                return;
            }
        }
        uses.append(YarnUse{slot, type, size, count});
    };
    for (const Row &row : grid.rows())
    {
        const GridBand &band = grid.bands().at(row.band);
        const QString type = band.stitchType == QLatin1String(kDefaultStitchName) ? defaultStitchType : band.stitchType;
        int coloured = 0;
        for (const Run &run : grid.overlayRuns(row.index))
        {
            if (!run.color.isEmpty())
            {
                add(run.color, type, band.size, run.count);
                coloured += run.count;
            }
        }
        add(mainSlot, type, band.size, row.count - coloured);
    }
    return uses;
}

QMap<QString, qreal> estimateYarnMeters(const QVector<YarnUse> &uses, const QMap<QString, Yarn> &yarnBySlot)
{
    QMap<QString, qreal> meters;
    for (const YarnUse &u : uses)
    {
        auto it = yarnBySlot.constFind(u.slot);
        if (it != yarnBySlot.constEnd())
        {
            meters[u.slot] += yarnForStitches(it.value(), u.stitchType, u.size, u.stitches) * (1.0 + kYarnMargin);
        }
    }
    return meters;
}

QMap<QString, qreal> estimateYarnMeters(const StitchGrid &grid, const QMap<QString, Yarn> &yarnBySlot,
                                        const QString &defaultStitchType)
{
    return estimateYarnMeters(yarnUses(grid, defaultStitchType), yarnBySlot);
}

bool suggestStitchSize(const Yarn &yarn, const QString &stitchType, StitchSize &size)
{
    const YarnGauge *g = yarn.gaugeFor(stitchType);
    if (!g) return false;
    size = g->size;
    return true;
}

}
