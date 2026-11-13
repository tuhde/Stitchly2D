#include "gauge.h"

namespace stitchcore
{

bool Gauge::isValid() const
{
    return stitchesPer10cm > 0 && rowsPer10cm > 0;
}

qreal Gauge::stitchWidthMm() const
{
    return isValid() ? 100.0 / stitchesPer10cm : 0.0;
}

qreal Gauge::rowHeightMm() const
{
    return isValid() ? 100.0 / rowsPer10cm : 0.0;
}

Gauge Gauge::fromPerInch(qreal stitchesPerInch, qreal rowsPerInch)
{
    Gauge g;
    g.stitchesPer10cm = stitchesPerInch * 100.0 / 25.4;
    g.rowsPer10cm     = rowsPerInch * 100.0 / 25.4;
    return g;
}

}
