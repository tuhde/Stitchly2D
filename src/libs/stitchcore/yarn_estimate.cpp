#include "yarn_estimate.h"

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

QMap<QString, qreal> estimateYarnMeters(const StitchGrid &grid, const QMap<QString, Yarn> &yarnBySlot)
{
    QMap<QString, qreal> meters;
    const QString mainSlot = QString::fromLatin1(kMainYarnSlot);
    for (const Row &row : grid.rows())
    {
        const GridBand &band = grid.bands().at(row.band);
        int coloured = 0;
        auto add = [&](const QString &slot, int count) {
            auto it = yarnBySlot.constFind(slot);
            if (it != yarnBySlot.constEnd())
            {
                meters[slot] += yarnForStitches(it.value(), band.stitchType, band.size, count);
            }
        };
        for (const Run &run : grid.overlayRuns(row.index))
        {
            if (!run.color.isEmpty())
            {
                add(run.color, run.count);
                coloured += run.count;
            }
        }
        add(mainSlot, qMax(0, row.count - coloured));
    }
    for (auto it = meters.begin(); it != meters.end(); ++it)
    {
        it.value() *= 1.0 + kYarnMargin;
    }
    return meters;
}

bool suggestStitchSize(const Yarn &yarn, const QString &stitchType, StitchSize &size)
{
    const YarnGauge *g = yarn.gaugeFor(stitchType);
    if (!g) return false;
    size = g->size;
    return true;
}

}
