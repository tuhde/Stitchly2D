// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCH_TOOLS_H
#define STITCH_TOOLS_H

class McpToolRegistry;
class MainWindow;

/**
 * Registers the tools Claude gets for a Stitchly2D window: look at the project, change the stitch settings, the
 * catalog and the bands of a piece, read the instructions, open and save files, take a screenshot.
 *
 * Every tool acts on the window the user sees, so the user watches the changes happen. Tools that read or write
 * files ask the user first (once per file and session if the box is ticked); the answer defaults to No.
 */
void registerStitchTools(McpToolRegistry &registry, MainWindow *window);

#endif
