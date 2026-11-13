// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_CATALOG_H
#define STITCHCORE_CATALOG_H

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "stitchtype.h"

namespace stitchcore
{

/**
 * @brief A repeat chart laid over a basic stitch type.
 *
 * Special patterns (kind Special) hold stitch symbols (e.g. "k", "p", "C4F") in the legend;
 * color patterns (kind Color) hold yarn colors (e.g. "MC", "CC1" or "#c03030").
 * Cells are row-major, row 0 at the bottom, and index into the legend; -1 leaves the base stitch alone.
 */
struct PatternDef
{
    enum Kind { Special, Color };

    QString      name;
    Kind         kind = Special;
    int          width = 1;       //!< repeat width in stitches
    int          height = 1;      //!< repeat height in rows
    QStringList  legend;
    QVector<int> cells;

    bool isValid() const;         //!< size matches cells, indices inside the legend
    int  cellAt(int x, int y) const;   //!< repeat cell (x, y), both wrapped around the repeat
};

/** Where and how a pattern is laid over a band. */
struct Overlay
{
    QString pattern;              //!< name in the catalog
    int firstRow   = 0;           //!< band-local row where it starts
    int rowCount   = 0;           //!< 0 = up to the end of the band
    int firstStitch = 0;          //!< counted from the first stitch of each row
    int stitchCount = 0;          //!< 0 = up to the end of the row
    int offsetX = 0;              //!< shifts the repeat
    int offsetY = 0;
};

/**
 * One horizontal band of a piece: from the previous band (or the bottom) upwards by heightMm,
 * worked in a basic stitch type with optional special and color overlays on top.
 * A heightMm <= 0 means "up to the top of the piece" and ends the list.
 */
struct BandSpec
{
    QString          stitchType;
    qreal            heightMm = 0;
    QVector<Overlay> overlays;
};

/** The catalog: basic stitch types plus special and color patterns, saved as JSON. */
class StitchCatalog
{
public:
    /** Catalog with a few common starting entries. Sizes are starting guesses; measure your own swatch. */
    static StitchCatalog withDefaults();

    StitchLibrary &basics() { return m_basics; }
    const StitchLibrary &basics() const { return m_basics; }

    const QVector<PatternDef> &patterns() const { return m_patterns; }
    QVector<PatternDef> patterns(PatternDef::Kind kind) const;
    bool contains(const QString &name) const;
    PatternDef find(const QString &name) const;       //!< empty name if not found
    bool set(const PatternDef &pattern);              //!< add or replace by name; false if invalid
    bool remove(const QString &name);

    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &json, StitchCatalog &out);   //!< false on malformed input
    bool save(const QString &path) const;
    static bool load(const QString &path, StitchCatalog &out);

private:
    StitchLibrary       m_basics;
    QVector<PatternDef> m_patterns;
};

}

#endif
