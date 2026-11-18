// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <functional>

#include <QHash>
#include <QJsonObject>
#include <QMainWindow>
#include <QStringList>

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
class QUndoStack;
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
    ~MainWindow() override;

    /** Open a Seamly2D pattern. A new pattern starts a new project: bands and patterns are cleared. */
    bool openPattern(const QString &fileName, QString *error = nullptr,
                     const QString &measurementsOverride = QString());

    /** Answer of the "unsaved changes" question. */
    enum class SaveChoice { Save, Discard, Cancel };
    using SavePrompt = std::function<SaveChoice()>;
    /** Replace the question that is asked before unsaved changes would be lost (the tests use this). */
    void setSavePrompt(const SavePrompt &prompt) { m_savePrompt = prompt; }

    /** True when the settings, bands or catalog changed since the pattern or project was opened or saved. */
    bool isModified() const;
    /** Asks what to do with unsaved changes. False means "cancel": do not go on. */
    bool maybeSave();

    /** Undo and redo of the settings, the catalog and the bands. Return the text of the step, empty if none. */
    QString undo();
    QString redo();
    QStringList undoTexts() const;      //!< the steps that can be undone, latest first
    /** Put in front of the text of the steps made while it is set (the Claude tools use "Claude: "). */
    void setUndoPrefix(const QString &prefix) { m_undoPrefix = prefix; }

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
    /** How a piece is knitted: its own method if it has one, otherwise the default of the settings. */
    stitchcore::KnitMethod pieceMethod(const QString &piece) const;
    bool hasPieceMethod(const QString &piece) const;
    /** Give a piece its own knitting method; -1 (or a value that is a KnitMethod) = use the default again. */
    bool setPieceMethod(const QString &piece, int method, QString *error = nullptr, const QString &undoText = QString());
    QStringList pieceNames() const;
    QString currentPiece() const;
    bool selectPiece(const QString &name);
    stitchcore::ProjectSettings settings() const;
    void setSettings(const stitchcore::ProjectSettings &settings, const QString &undoText = QString());
    QVector<stitchcore::BandSpec> bands(const QString &piece) const;
    /** Replace the bands of a piece. Checked against the catalog; the last band always reaches the top. */
    bool setBands(const QString &piece, const QVector<stitchcore::BandSpec> &bands, QString *error = nullptr,
                  const QString &undoText = QString());
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
    void setCatalog(const stitchcore::StitchCatalog &catalog, const QString &undoText = QString());

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void fitAll();
    void setLanguage(const QString &locale);
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
    friend class StateCommand;

    /** Everything an undo step changes. */
    struct State
    {
        stitchcore::ProjectSettings                            settings;
        QHash<QString, QVector<stitchcore::BandSpec>>          bands;
        QHash<QString, int>                                    methods;     //!< pieces with their own knitting method
        stitchcore::StitchCatalog                              catalog;
    };
    State captureState() const;
    void applyState(const State &state);
    static bool sameState(const State &a, const State &b);
    /** Called after an edit has been made: pushes an undo step if something changed. Same mergeId = one step. */
    void recordChange(const QString &text, int mergeId = -1);
    void resetHistory();
    void updateTitle();

    void createActions();
    void createDocks();
    void refresh();
    stitchcore::StitchSize currentSize() const;
    stitchcore::ShapingOptions currentShaping() const;
    const LoadedPiece *findPiece(const QString &name) const;
    /** The shaping options of a piece: the settings, with the knitting method of that piece. */
    stitchcore::ShapingOptions shapingFor(const LoadedPiece &piece) const;
    void populatePieceMethod();
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
    QHash<QString, int> m_methods;                           //!< KnitMethod of the pieces that differ from the default

    VMainGraphicsScene *m_scene = nullptr;
    VMainGraphicsView  *m_view = nullptr;
    QListWidget        *m_pieces = nullptr;
    QComboBox          *m_stitchType = nullptr;
    QDoubleSpinBox     *m_stitchWidth = nullptr;
    QDoubleSpinBox     *m_rowHeight = nullptr;
    QLabel             *m_gauge = nullptr;
    QCheckBox          *m_allowance = nullptr;
    QSpinBox           *m_edgeStitches = nullptr;
    QSpinBox           *m_bulkFrom = nullptr;
    QComboBox          *m_shapeOn = nullptr;
    QComboBox          *m_method = nullptr;
    QSpinBox           *m_needles = nullptr;
    QComboBox          *m_pieceMethod = nullptr;
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
    QUndoStack         *m_undo = nullptr;
    State               m_lastState;       //!< the state the undo history is at
    int                 m_suppress = 0;    //!< >0 while changes are made that are recorded as one step later
    QString             m_undoPrefix;
    SavePrompt          m_savePrompt;
    McpIntegration     *m_mcp = nullptr;
    QLabel             *m_mcpStatus = nullptr;
    QString             m_projectFile;
    QString             m_measurementsOverride;
};

#endif
