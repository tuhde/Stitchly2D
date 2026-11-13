// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_GAUGE_H
#define STITCHCORE_GAUGE_H

#include <QtGlobal>

namespace stitchcore
{

/**
 * @brief Knitting gauge: how many stitches and rows make up 10 cm.
 */
struct Gauge
{
    qreal stitchesPer10cm = 20.0;
    qreal rowsPer10cm     = 28.0;

    bool   isValid() const;
    qreal  stitchWidthMm() const;  //!< width of one stitch
    qreal  rowHeightMm() const;    //!< height of one row

    /** Convert a gauge given per inch (e.g. swatch measured in inches). */
    static Gauge fromPerInch(qreal stitchesPerInch, qreal rowsPerInch);
};

}

#endif
