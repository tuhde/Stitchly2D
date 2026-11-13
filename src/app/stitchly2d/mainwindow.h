// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "core/pattern_loader.h"
#include "../../libs/stitchcore/catalog.h"
#include "../../libs/stitchcore/shaping.h"
#include "../../libs/stitchcore/stitchgrid.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QListWidget;
class QPlainTextEdit;
class QSpinBox;
class VMainGraphicsScene;
class VMainGraphicsView;

/**
 * Main window: pieces on the left, the selected piece with its stitch grid in the middle, stitch settings on the
 * right and the written instructions below. Same dock layout and canvas as Seamly2D.
 *
 * First version: one stitch size and one set of shaping options for the whole pattern.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();

    bool openPattern(const QString &fileName, QString *error = nullptr);

    /** Instructions of the selected piece, for tests and screenshots. */
    QString instructionsText() const;

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void fitAll();
    void openFile();
    void exportInstructions();
    void exportRowTable();
    void exportImage();
    void about();
    void pieceChanged();
    void stitchTypeChanged();
    void sizeEdited();
    void settingsChanged();

private:
    void createActions();
    void createDocks();
    void refresh();
    stitchcore::StitchSize currentSize() const;
    stitchcore::ShapingOptions currentShaping() const;
    void showGrid(const QPolygonF &outline, const stitchcore::StitchGrid &grid);
    bool saveText(const QString &title, const QString &filter, const QString &suggested, const QString &text);

    LoadedPattern              m_pattern;
    stitchcore::StitchCatalog  m_catalog;
    stitchcore::StitchGrid     m_grid;

    VMainGraphicsScene *m_scene = nullptr;
    VMainGraphicsView  *m_view = nullptr;
    QListWidget        *m_pieces = nullptr;
    QComboBox          *m_stitchType = nullptr;
    QDoubleSpinBox     *m_stitchWidth = nullptr;
    QDoubleSpinBox     *m_rowHeight = nullptr;
    QLabel             *m_gauge = nullptr;
    QCheckBox          *m_allowance = nullptr;
    QSpinBox           *m_edgeStitches = nullptr;
    QComboBox          *m_increase = nullptr;
    QComboBox          *m_decrease = nullptr;
    QLabel             *m_stats = nullptr;
    QPlainTextEdit     *m_instructions = nullptr;
    bool                m_updating = false;
};

#endif
