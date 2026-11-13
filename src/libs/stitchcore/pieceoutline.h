// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_PIECEOUTLINE_H
#define STITCHCORE_PIECEOUTLINE_H

#include <QLineF>
#include <QPolygonF>

namespace stitchcore
{

/**
 * @brief Turn a piece outline from Seamly2D into the orientation StitchGrid expects.
 *
 * Input: millimetres, y pointing down (scene coordinates), plus the grainline (null if the piece has none).
 * The piece is rotated so the grainline is vertical, and the smallest turn is used, so a piece that is
 * already upright stays upright. Output: y points up (row 0 = bottom of the piece as drawn), knitting goes along +y.
 * Without a grainline the piece is only converted to y-up and its drawn orientation is kept.
 * The result is moved so its bounding box starts at (0, 0).
 */
QPolygonF knittingOutline(const QPolygonF &sceneMm, const QLineF &grainline);

}

#endif
