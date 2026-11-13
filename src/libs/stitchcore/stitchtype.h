// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_STITCHTYPE_H
#define STITCHCORE_STITCHTYPE_H

#include <QString>
#include <QVector>

#include "stitchsize.h"

namespace stitchcore
{

/** A named stitch type with the size one stitch takes in the fabric (e.g. stockinette, 2x2 rib). */
struct StitchType
{
    QString    name;
    StitchSize size;
};

/** Library of named stitch types, saved with the project. Names are unique. */
class StitchLibrary
{
public:
    /** Library with a few common starting entries. */
    static StitchLibrary withDefaults();

    const QVector<StitchType> &types() const { return m_types; }
    bool contains(const QString &name) const;
    StitchType find(const QString &name) const;      //!< empty name if not found
    void set(const StitchType &type);                //!< add or replace by name
    bool remove(const QString &name);

private:
    QVector<StitchType> m_types;
};

/**
 * One horizontal band of a piece: from the previous band (or the bottom) upwards by heightMm.
 * A heightMm <= 0 means "up to the top of the piece" and ends the list.
 */
struct BandSpec
{
    QString stitchType;
    qreal   heightMm = 0;
};

}

#endif
