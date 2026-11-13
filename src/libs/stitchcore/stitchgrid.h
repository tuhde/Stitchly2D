// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_STITCHGRID_H
#define STITCHCORE_STITCHGRID_H

#include <QPolygonF>
#include <QString>
#include <QVector>

#include "catalog.h"

namespace stitchcore
{

/** One knitted row: stitches [firstStitch, firstStitch + count) counted in the grid of its band. */
struct Row
{
    int index       = 0;  //!< 0 = cast-on row (bottom)
    int band        = 0;
    int firstStitch = 0;
    int count       = 0;

    int end() const { return firstStitch + count; }
};

/** An overlay together with its pattern, as used by the grid. */
struct ResolvedOverlay
{
    Overlay    overlay;
    PatternDef pattern;
};

/** A band as it ended up in the grid. */
struct GridBand
{
    QString                 stitchType;
    StitchSize              size;
    int                     firstRow = 0;
    int                     rowCount = 0;
    QVector<ResolvedOverlay> overlays;
};

/** What one stitch is made of: the basic type plus whatever the overlays put on it. */
struct Cell
{
    QString basic;
    QString special;   //!< stitch symbol from a special pattern, empty if none
    QString color;     //!< yarn color from a color pattern, empty if none
};

/** Consecutive stitches of a row with the same special symbol and color. */
struct Run
{
    QString special;
    QString color;
    int     count = 0;
};

/**
 * @brief Stitch grid of one piece, built band by band from the bottom.
 *
 * The outline is given in millimetres with the knitting direction along +y (row 0 is at the smallest y).
 * The caller rotates the piece so the grainline is vertical and flips y if the source uses y-down.
 * Stitch width and row height can differ from band to band, so stitch positions are only comparable
 * inside one band; across a band change only the stitch count carries over.
 */
class StitchGrid
{
public:
    /** Whole piece in one stitch type. */
    static StitchGrid build(const QPolygonF &outlineMm, const StitchSize &size);

    /**
     * Piece in bands. Bands with an unknown stitch type or an invalid size make the result empty.
     * The last band always reaches the top, whatever its height says.
     */
    static StitchGrid build(const QPolygonF &outlineMm, const QVector<BandSpec> &bands,
                            const StitchLibrary &library);

    /** Piece in bands with overlays. An overlay naming a pattern that is not in the catalog makes the result empty. */
    static StitchGrid build(const QPolygonF &outlineMm, const QVector<BandSpec> &bands,
                            const StitchCatalog &catalog);

    /** Stitch s (counted in the band grid, see Row) of row r; basic type only if r or s is outside the piece. */
    Cell cell(int row, int stitch) const;

    /** The stitches of a row as runs. Empty if no overlay touches the row. */
    QVector<Run> overlayRuns(int row) const;

    const QVector<Row> &rows() const { return m_rows; }
    const QVector<GridBand> &bands() const { return m_bands; }
    int rowCount() const { return m_rows.size(); }
    int width() const { return m_width; }          //!< widest row in stitches (of its own band)
    int castOn() const { return m_rows.isEmpty() ? 0 : m_rows.first().count; }
    int bindOff() const { return m_rows.isEmpty() ? 0 : m_rows.last().count; }
    bool isEmpty() const { return m_rows.isEmpty(); }

private:
    QVector<Row>      m_rows;
    QVector<GridBand> m_bands;
    int               m_width = 0;
};

}

#endif
