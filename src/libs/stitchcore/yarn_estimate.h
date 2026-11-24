// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_YARN_ESTIMATE_H
#define STITCHCORE_YARN_ESTIMATE_H

#include <QMap>
#include <QString>

#include "stitchgrid.h"
#include "yarn.h"

namespace stitchcore
{

/** Slot of the yarn that knits the stitches no color pattern colours. */
extern const char *const kMainYarnSlot;     // "main"

/** Without a measured value, one stitch takes this many stitch widths of yarn. */
constexpr qreal kDefaultYarnPerStitchWidths = 4.0;
/** Added on top of the estimate (tension, ends, swatch). */
constexpr qreal kYarnMargin = 0.10;

/** How many stitches of the grid each yarn slot knits (cast-on row included). Slots: kMainYarnSlot or the color name of a pattern. */
QMap<QString, int> stitchesPerSlot(const StitchGrid &grid);

/** Yarn length of a number of stitches of a basic stitch, margin not included. */
qreal yarnForStitches(const Yarn &yarn, const QString &stitchType, const StitchSize &size, int stitches);

/**
 * Meters (with margin) every slot needs. Slots without a yarn in the map are left out.
 * Uses the gauge the yarn has for the band's basic stitch. A band that uses the "Default stitch" counts as
 * defaultStitchType (the stitch named in the project settings; empty = custom size, no gauge applies).
 */
QMap<QString, qreal> estimateYarnMeters(const StitchGrid &grid, const QMap<QString, Yarn> &yarnBySlot,
                                        const QString &defaultStitchType = QString());

/** The stitch size a yarn gives with the basic stitch; false if the yarn has no gauge for it. */
bool suggestStitchSize(const Yarn &yarn, const QString &stitchType, StitchSize &size);

}

#endif
