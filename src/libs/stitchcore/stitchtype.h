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

}

#endif
