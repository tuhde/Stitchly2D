// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_PASSES_H
#define STITCHCORE_PASSES_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "shaping.h"
#include "stitchgrid.h"

namespace stitchcore
{

enum class Bed { None, Front, Rear };

/** One carriage pass of the machine: with which yarn (empty = no colour pattern) and on which bed (tubular double-bed). */
struct RowPass
{
    QString yarn;
    Bed     bed = Bed::None;
};

/**
 * What the machine does for one row. The row is finished when all its passes are done, so a row counter that
 * counts carriage passes needs passes.size() of them.
 *
 * - Tubular double-bed machine: every yarn of the row is knitted on the front and then on the rear bed
 *   (a plain row: front, rear).
 * - Flat on a double-bed machine: one pass per yarn of the row (a plain row: one pass).
 * - All other methods: one pass; the colours of the row are worked within it.
 */
struct RowPasses
{
    QStringList      yarns;       //!< the yarns of the row in order of appearance; empty if no colour pattern touches the row
    QVector<RowPass> passes = {RowPass()};      //!< never empty

    int count() const { return passes.size(); }
};

/** Passes of a row of the grid (zero-based: grid row 0 is instruction row 1). */
RowPasses passesForRow(const StitchGrid &grid, KnitMethod method, int gridRow);

/**
 * The passes of a row as short text for the row counter: yarns separated by commas, beds by slashes, e.g.
 * "red,green", "front/rear", "red front/rear,green front/rear". Empty if the row is a single plain pass.
 * current = index of the pass the machine is at (-1: none); with rich it is marked bold and underlined (HTML).
 */
QString passesText(const RowPasses &passes, int current = -1, bool rich = false);

}

#endif
