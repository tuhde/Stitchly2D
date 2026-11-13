// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_STITCHSIZE_H
#define STITCHCORE_STITCHSIZE_H

#include <QtGlobal>

namespace stitchcore
{

/**
 * @brief Size of one stitch: its width and the height of one row, both in millimetres.
 *
 * This is the stored form. Gauges (stitches and rows per 10 cm or per inch) are calculated from it on the fly.
 */
struct StitchSize
{
    qreal stitchWidthMm = 5.0;
    qreal rowHeightMm   = 3.5;

    bool  isValid() const;

    // Derived gauges
    qreal stitchesPer10cm() const;
    qreal rowsPer10cm() const;
    qreal stitchesPerInch() const;
    qreal rowsPerInch() const;

    // Construct from a gauge
    static StitchSize fromPer10cm(qreal stitchesPer10cm, qreal rowsPer10cm);
    static StitchSize fromPerInch(qreal stitchesPerInch, qreal rowsPerInch);
};

}

#endif
