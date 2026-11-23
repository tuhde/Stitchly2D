// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef HEADLESS_WINDOW_H
#define HEADLESS_WINDOW_H

#include "../vwidgets/vabstractmainwindow.h"

/**
 * Never shown. Seamly2D's spline tools connect to the main window while a pattern is parsed, so the
 * loader needs a real VAbstractMainWindow to hand them.
 */
class HeadlessWindow : public VAbstractMainWindow
{
    Q_OBJECT
public:
    HeadlessWindow() = default;

public slots:
    void setStatusMessage(const QString &message) override { Q_UNUSED(message) }
    void updateGroups() override {}
    void zoomToSelected() override {}

protected:
    void exportToCSVData(const QString &fileName, const DialogExportToCSV &dialog) override
    {
        Q_UNUSED(fileName) Q_UNUSED(dialog)
    }
};

#endif
