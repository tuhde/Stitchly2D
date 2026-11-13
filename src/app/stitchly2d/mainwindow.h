// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHash>
#include <QJsonObject>
#include <QMainWindow>

#include "core/pattern_loader.h"
#include "../../libs/stitchcore/catalog.h"
#include "../../libs/stitchcore/project.h"
#include "../../libs/stitchcore/shaping.h"
#include "../../libs/stitchcore/stitchgrid.h"

class McpIntegration;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QListWidget;
class QMenu;
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

    /** Open a Seamly2D pattern. A new pattern starts a new project: bands and patterns are cleared. */
    bool openPattern(const QString &fileName, QString *error = nullptr,
                     const QString &measurementsOverride = QString());

    /** What to do when a project's catalog entries differ from the ones in the catalog. */
    enum class CatalogConflict { Ask, UseProject, KeepMine };

    /**
     * Open a project (.stitchly): the pattern, the stitch settings, and the bands and patterns of every piece.
     * Nothing changes if the pattern cannot be loaded.
     */
    bool openProject(const QString &fileName, QString *error = nullptr, CatalogConflict conflict = CatalogConflict::Ask);

    /** Save the pattern's stitch settings, bands and patterns, with the catalog entries they use. */
    bool saveProject(const QString &fileName, QString *error = nullptr);

    QString projectFile() const { return m_projectFile; }

    // ---- The window's state for the Claude (MCP) tools. They act on exactly what the user sees.
    QStringList pieceNames() const;
    QString currentPiece() const;
    bool selectPiece(const QString &name);
    stitchcore::ProjectSettings settings() const;
    void setSettings(const stitchcore::ProjectSettings &settings);
    QVector<stitchcore::BandSpec> bands(const QString &piece) const;
    /** Replace the bands of a piece. Checked against the catalog; the last band always reaches the top. */
    bool setBands(const QString &piece, const QVector<stitchcore::BandSpec> &bands, QString *error = nullptr);
    /** Size, stitch counts and bands of a piece with the current settings (empty object for an unknown piece). */
    QJsonObject pieceSummary(const QString &piece) const;
    QStringList instructionsFor(const QString &piece) const;
    QString rowTableFor(const QString &piece) const;
    QString allInstructionsText() const;
    QByteArray screenshotPng();
    /** Small description of this window for the window list of the Claude connection. */
    QJsonObject mcpProjectInfo() const;
    McpIntegration *mcp() const { return m_mcp; }

    /** Instructions of the selected piece, for tests and screenshots. */
    QString instructionsText() const;

    const stitchcore::StitchCatalog &catalog() const { return m_catalog; }

    /**
     * Use a new catalog. Bands that use a basic stitch which is gone fall back to the default stitch, and patterns
     * that are gone are taken off the bands.
     */
    void setCatalog(const stitchcore::StitchCatalog &catalog);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void fitAll();
    void editCatalog();
    void openProjectDialog();
    void saveProjectAction();
    void saveProjectAs();
    void openFile();
    void exportInstructions();
    void exportRowTable();
    void exportImage();
    void about();
    void pieceChanged();
    void stitchTypeChanged();
    void sizeEdited();
    void settingsChanged();
    void commitBands();
    void bandSelected();
    void addBand();
    void removeBand();
    void addOverlay();
    void removeOverlay();

private:
    void createActions();
    void createDocks();
    void refresh();
    stitchcore::StitchSize currentSize() const;
    stitchcore::ShapingOptions currentShaping() const;
    const LoadedPiece *findPiece(const QString &name) const;
    void initializeMcp();
    QVector<stitchcore::BandSpec> bandsFor(const LoadedPiece &piece) const;
    stitchcore::StitchCatalog workingCatalog() const;
    stitchcore::StitchGrid gridFor(const LoadedPiece &piece, QPolygonF *outline = nullptr) const;
    void populateBandTables();
    void populateOverlayTable();
    void showGrid(const QPolygonF &outline, const stitchcore::StitchGrid &grid);
    bool saveText(const QString &title, const QString &filter, const QString &suggested, const QString &text);

    LoadedPattern              m_pattern;
    stitchcore::StitchCatalog  m_catalog;
    stitchcore::StitchGrid     m_grid;
    QHash<QString, QVector<stitchcore::BandSpec>> m_bands;   //!< edited bands per piece name

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
    QTableWidget       *m_bandTable = nullptr;
    QTableWidget       *m_overlayTable = nullptr;
    QPushButton        *m_removeBand = nullptr;
    QPushButton        *m_addOverlay = nullptr;
    QPushButton        *m_removeOverlay = nullptr;
    QPlainTextEdit     *m_instructions = nullptr;
    bool                m_updating = false;
    QMenu              *m_viewMenu = nullptr;
    McpIntegration     *m_mcp = nullptr;
    QLabel             *m_mcpStatus = nullptr;
    QString             m_projectFile;
    QString             m_measurementsOverride;
};

#endif
