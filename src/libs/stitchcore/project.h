// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_PROJECT_H
#define STITCHCORE_PROJECT_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include "catalog.h"
#include "shaping.h"

namespace stitchcore
{

/** Settings shared by all pieces of a project. */
struct ProjectSettings
{
    QString        stitchType;                 //!< catalog name shown in the settings, empty = custom size
    StitchSize     size;                       //!< the "default stitch" of the bands
    bool           includeSeamAllowance = false;
    ShapingOptions shaping;
};

/**
 * @brief A knitting project: which Seamly2D pattern, the stitch settings, and the bands and patterns of each piece.
 *
 * Saved as JSON ("*.stitchly"). The catalog entries the bands use are stored inside the project, so it gives the
 * same result on a machine whose catalog differs; see diffCatalog() and mergeCatalog().
 * File names are stored relative to the project file and are absolute in memory.
 */
struct Project
{
    QString                          patternFile;        //!< absolute
    QString                          measurementsFile;   //!< absolute; empty = the one named in the pattern
    ProjectSettings                  settings;
    QMap<QString, QVector<BandSpec>> bands;              //!< by piece name; pieces without an entry use one default band
    QMap<QString, KnitMethod>        methods;            //!< pieces that are knitted differently from settings.shaping.method
    StitchCatalog                    catalog;            //!< only the entries the bands use
};

// Equality, used to tell whether an edit changed anything (undo history, "modified" marker).
bool operator==(const Overlay &a, const Overlay &b);
bool operator==(const BandSpec &a, const BandSpec &b);
bool operator==(const ProjectSettings &a, const ProjectSettings &b);
/** Same stitches and patterns with the same definitions (the order of the entries does not matter). */
bool sameCatalog(const StitchCatalog &a, const StitchCatalog &b);

constexpr int kProjectFileVersion = 1;
extern const char *const kProjectFileExtension;          // "stitchly"

/** Name of the stitch a band uses when it should take the stitch from the settings. */
extern const char *const kDefaultStitchName;             // "Default stitch"

bool saveProject(const QString &path, const Project &project, QString *error = nullptr);

/** On failure the project is left alone and error says why. Files with a newer version are refused. */
bool loadProject(const QString &path, Project &project, QString *error = nullptr);

/** The entries of full that the bands use (basic stitches by name, patterns by name); unknown names are skipped. */
StitchCatalog usedCatalog(const StitchCatalog &full, const QMap<QString, QVector<BandSpec>> &bands);

struct CatalogDiff
{
    QStringList added;        //!< in incoming, not in target
    QStringList differing;    //!< in both, with a different definition
};

/** What mergeCatalog() would do, without changing anything. Names of basic stitches and patterns are not mixed up. */
CatalogDiff diffCatalog(const StitchCatalog &target, const StitchCatalog &incoming);

/** Add what is missing; replace entries with a different definition only if replaceDifferent. */
void mergeCatalog(StitchCatalog &target, const StitchCatalog &incoming, bool replaceDifferent);

}

#endif
