// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef PATTERN_LOADER_H
#define PATTERN_LOADER_H

#include <QLineF>
#include <QPolygonF>
#include <QString>
#include <QVector>

/** One pattern piece as Seamly2D drew it. Coordinates are millimetres in the scene (y points down). */
struct LoadedPiece
{
    QString   name;
    QPolygonF seamLine;       //!< closed outline without seam allowance
    QPolygonF cutLine;        //!< outline with seam allowance; empty if the piece has none
    QLineF    grainline;      //!< null if the piece has no grainline
};

struct LoadedPattern
{
    QString               fileName;
    QString               measurementsFile;
    QVector<LoadedPiece>  pieces;
};

/**
 * Loads a Seamly2D pattern (.sm2d, or an old .val) with its measurements and returns the pieces.
 * Runs Seamly2D's own parser, so the geometry is the same as in Seamly2D.
 * Needs an Application2D instance. Returns false and fills error on failure; the pattern file is never modified.
 *
 * @param measurementsOverride  use this measurement file instead of the one named in the pattern
 */
bool loadPattern(const QString &fileName, LoadedPattern &out, QString *error,
                 const QString &measurementsOverride = QString());

#endif
