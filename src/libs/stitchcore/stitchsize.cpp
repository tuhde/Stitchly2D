#include "stitchsize.h"

namespace stitchcore
{

namespace
{
const qreal mmPerInch = 25.4;
}

bool StitchSize::isValid() const
{
    return stitchWidthMm > 0 && rowHeightMm > 0;
}

qreal StitchSize::stitchesPer10cm() const { return isValid() ? 100.0 / stitchWidthMm : 0.0; }
qreal StitchSize::rowsPer10cm() const { return isValid() ? 100.0 / rowHeightMm : 0.0; }
qreal StitchSize::stitchesPerInch() const { return isValid() ? mmPerInch / stitchWidthMm : 0.0; }
qreal StitchSize::rowsPerInch() const { return isValid() ? mmPerInch / rowHeightMm : 0.0; }

StitchSize StitchSize::fromPer10cm(qreal stitches, qreal rows)
{
    StitchSize s;
    s.stitchWidthMm = stitches > 0 ? 100.0 / stitches : 0.0;
    s.rowHeightMm   = rows > 0 ? 100.0 / rows : 0.0;
    return s;
}

StitchSize StitchSize::fromPerInch(qreal stitches, qreal rows)
{
    StitchSize s;
    s.stitchWidthMm = stitches > 0 ? mmPerInch / stitches : 0.0;
    s.rowHeightMm   = rows > 0 ? mmPerInch / rows : 0.0;
    return s;
}

}
