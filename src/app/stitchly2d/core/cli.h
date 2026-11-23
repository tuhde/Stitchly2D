// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef CLI_H
#define CLI_H

#include <QString>

#include "../../../libs/stitchload/core/knit_job.h"   // userCatalogPath()

class QTextStream;

struct CliOptions
{
    QString input;                 //!< a Seamly2D pattern (.sm2d) or a Stitchly2D project (.stitchly)
    QString measurements;          //!< measurement file to use instead of the one named in the pattern / project
    bool    hasStitchWidth = false;    //!< set on the command line: overrides what the project says
    double  stitchWidthMm = 5.0;
    bool    hasRowHeight = false;
    double  rowHeightMm = 3.5;
    bool    seamAllowance = false;     //!< knit the outline with seam allowance (a project can ask for it, too)
    QString method;                    //!< knit all pieces this way: flat, round, circular_machine, double_bed_machine; empty = as the project says
    int     needles = -1;              //!< machine needles; -1 = as the project says
    QString shapeOn;                   //!< "outside" or "every"; empty = the project's (or the default: outside)
    bool    csv = false;               //!< print the row table instead of the instructions
    QString piece;                     //!< only this piece; empty = all
};

/**
 * Prints stitch counts and written instructions (or the row table) of the pieces of a pattern or project.
 * With a project, its stitch settings, bands and patterns are used, and its catalog entries replace the ones of the
 * user's catalog for this run, so the result is the one the project was made with. Needs an Application2D.
 * Returns the exit code: 0 ok, 1 could not load, 2 wrong arguments.
 */
int runCli(const CliOptions &options, QTextStream &out, QTextStream &err);

#endif
