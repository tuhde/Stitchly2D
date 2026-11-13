// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_PATTERNFILE_H
#define STITCHCORE_PATTERNFILE_H

#include <QString>
#include <QVector>

#include "catalog.h"

namespace stitchcore
{

/**
 * @brief File format for exchanging special and color patterns ("*.stpat").
 *
 * UTF-8 JSON, one file holds one or several patterns:
 * @code
 * {
 *   "format": "stitchly-patterns",
 *   "version": 1,
 *   "author": "...",            optional
 *   "description": "...",       optional
 *   "patterns": [
 *     { "name": "Seed stitch", "kind": "special",   // or "color"
 *       "width": 2, "height": 2,
 *       "legend": ["k", "p"],                      // symbols, or yarn colors like "MC", "#c03030"
 *       "rows": [[1, 0],                           // top row first, as a chart is read
 *                [0, 1]] }                         // numbers index the legend, -1 = leave the base stitch
 *   ]
 * }
 * @endcode
 * The cells are in the order of the chart as seen: the last row is the bottom row of the repeat.
 * Files with a newer version number are refused rather than half-read.
 */
struct PatternFileInfo
{
    QString author;
    QString description;
};

constexpr int kPatternFileVersion = 1;
extern const char *const kPatternFileExtension;      // "stpat"

bool exportPatterns(const QString &path, const QVector<PatternDef> &patterns,
                    const PatternFileInfo &info = PatternFileInfo(), QString *error = nullptr);

bool importPatterns(const QString &path, QVector<PatternDef> &patterns,
                    PatternFileInfo *info = nullptr, QString *error = nullptr);

/** What to do when an imported pattern has the name of one already in the catalog. */
enum class ConflictPolicy { Skip, Replace, Rename };

struct ImportResult
{
    int added    = 0;
    int replaced = 0;
    int skipped  = 0;
    int renamed  = 0;      //!< added under a new name ("name (2)"); also counted in added
};

ImportResult addToCatalog(StitchCatalog &catalog, const QVector<PatternDef> &patterns, ConflictPolicy policy);

}

#endif
