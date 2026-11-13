#include "mainwindow.h"

#include <QAction>
#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QGraphicsSimpleTextItem>
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QStandardPaths>
#include <QShowEvent>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "catalog_dialog.h"
#include "core/cli.h"
#include "mcp/mcp_integration.h"
#include "mcp/mcp_panel.h"
#ifdef STITCHLY_HAVE_QTERMWIDGET
#include "mcp/claude_shell_panel.h"
#endif
#include "core/colors.h"
#include "../../libs/stitchcore/instructions.h"
#include "../../libs/stitchcore/pieceoutline.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

using namespace stitchcore;

namespace
{
const qreal kScale = 4.0;           // scene units per millimetre
const QString kCustom = QStringLiteral("Custom");
// The stitch settings dock, usable as a band's type
const QString kDefaultStitch = QString::fromLatin1(stitchcore::kDefaultStitchName);

QString catalogPath()
{
    return userCatalogPath();
}
}

MainWindow::MainWindow()
{
    setWindowTitle(QStringLiteral("Stitchly2D"));
    resize(1280, 800);

    m_catalog = StitchCatalog::withDefaults();
    StitchCatalog user;
    if (QFile::exists(catalogPath()) && StitchCatalog::load(catalogPath(), user))
    {
        m_catalog = user;
    }

    m_scene = new VMainGraphicsScene(this);
    m_view = new VMainGraphicsView(this);
    m_view->setScene(m_scene);
    setCentralWidget(m_view);

    createDocks();
    createActions();
    statusBar()->showMessage(tr("Open a Seamly2D pattern (.sm2d) to start."));
    refresh();
    initializeMcp();
}

void MainWindow::initializeMcp()
{
    m_mcp = new McpIntegration(this);

    m_mcpStatus = new QLabel(this);
    m_mcpStatus->setVisible(false);
    statusBar()->addPermanentWidget(m_mcpStatus);

    // "MCP Config and Log" dock: a tab next to the stitch settings, listed in the View menu.
    McpPanel *mcpPanel = new McpPanel(m_mcp, this);
    addDockWidget(Qt::RightDockWidgetArea, mcpPanel);
    if (QDockWidget *settingsDock = findChild<QDockWidget *>(QStringLiteral("settingsDock")))
    {
        tabifyDockWidget(settingsDock, mcpPanel);
    }
    mcpPanel->hide();
    m_viewMenu->addAction(mcpPanel->toggleViewAction());

#ifdef STITCHLY_HAVE_QTERMWIDGET
    // Dockable terminal that runs the Claude Code CLI against this window's MCP server (starts when first shown).
    ClaudeShellPanel *shell = new ClaudeShellPanel(m_mcp, this);
    addDockWidget(Qt::RightDockWidgetArea, shell);
    if (QDockWidget *settingsDock = findChild<QDockWidget *>(QStringLiteral("settingsDock")))
    {
        tabifyDockWidget(settingsDock, shell);
    }
    resizeDocks({shell}, {520}, Qt::Horizontal);
    shell->hide();
    // Not toggleViewAction(): a restored layout can mark the shell as shown while its tab is behind another one.
    // This entry always brings the tab to the front and hides the panel only if it is already in front.
    QAction *shellAction = new QAction(tr("Claude shell"), this);
    shellAction->setMenuRole(QAction::NoRole);
    connect(shellAction, &QAction::triggered, shell, [shell]()
    {
        if (shell->isVisible())
        {
            shell->hide();
        }
        else
        {
            shell->show();
            shell->raise();
        }
    });
    m_viewMenu->addAction(shellAction);
#endif

    connect(m_mcp, &McpIntegration::stateChanged, this, [this]()
    {
        const QString text = m_mcp->statusText();
        m_mcpStatus->setText(text);
        m_mcpStatus->setVisible(!text.isEmpty());
    });

    m_mcp->startFromSettings();

    // Test hook: open the panels at once (tests and demos cannot click through the menus).
    if (qEnvironmentVariableIsSet("STITCHLY_MCP_SHOW_PANELS"))
    {
        mcpPanel->show();
#ifdef STITCHLY_HAVE_QTERMWIDGET
        shell->show();
        shell->raise();
#endif
    }
}

void MainWindow::createActions()
{
    QMenu *file = menuBar()->addMenu(tr("&File"));
    QAction *open = file->addAction(tr("&Open pattern..."), this, &MainWindow::openFile, QKeySequence::Open);
    file->addAction(tr("Open &project..."), this, &MainWindow::openProjectDialog);
    QAction *save = file->addAction(tr("&Save project"), this, &MainWindow::saveProjectAction, QKeySequence::Save);
    file->addAction(tr("Save project &as..."), this, &MainWindow::saveProjectAs, QKeySequence::SaveAs);
    file->addSeparator();
    file->addAction(tr("Export &instructions..."), this, &MainWindow::exportInstructions);
    file->addAction(tr("Export &row table (CSV)..."), this, &MainWindow::exportRowTable);
    file->addAction(tr("Export &chart image..."), this, &MainWindow::exportImage);
    file->addSeparator();
    file->addAction(tr("&Quit"), this, &QWidget::close, QKeySequence::Quit);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    m_viewMenu = viewMenu;
    QAction *fit = viewMenu->addAction(tr("Zoom to &fit"), this, &MainWindow::fitAll,
                                       QKeySequence(Qt::CTRL | Qt::Key_0));
    QAction *zin = viewMenu->addAction(tr("Zoom &in"), m_view, &VMainGraphicsView::zoomIn, QKeySequence::ZoomIn);
    QAction *zout = viewMenu->addAction(tr("Zoom &out"), m_view, &VMainGraphicsView::zoomOut, QKeySequence::ZoomOut);
    viewMenu->addSeparator();
    for (QDockWidget *dock : findChildren<QDockWidget *>())
    {
        viewMenu->addAction(dock->toggleViewAction());
    }

    QMenu *catalogMenu = menuBar()->addMenu(tr("&Catalog"));
    catalogMenu->addAction(tr("&Edit catalog (stitches and patterns)..."), this, &MainWindow::editCatalog);

    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About Stitchly2D"), this, &MainWindow::about);

    QToolBar *bar = addToolBar(tr("Tools"));
    bar->setObjectName(QStringLiteral("toolbar"));
    bar->addAction(open);
    bar->addAction(save);
    bar->addSeparator();
    bar->addAction(fit);
    bar->addAction(zin);
    bar->addAction(zout);
}

void MainWindow::createDocks()
{
    // Pieces
    m_pieces = new QListWidget;
    m_pieces->setObjectName(QStringLiteral("piecesList"));
    connect(m_pieces, &QListWidget::currentRowChanged, this, &MainWindow::pieceChanged);
    auto *piecesDock = new QDockWidget(tr("Pieces"), this);
    piecesDock->setObjectName(QStringLiteral("piecesDock"));
    piecesDock->setWidget(m_pieces);
    addDockWidget(Qt::LeftDockWidgetArea, piecesDock);

    // Stitch settings
    auto *panel = new QWidget;
    auto *form = new QFormLayout(panel);

    m_stitchType = new QComboBox;
    m_stitchType->setObjectName(QStringLiteral("stitchTypeCombo"));
    for (const StitchType &t : m_catalog.basics().types())
    {
        m_stitchType->addItem(t.name);
    }
    m_stitchType->addItem(kCustom);
    form->addRow(tr("Basic stitch"), m_stitchType);

    m_stitchWidth = new QDoubleSpinBox;
    m_stitchWidth->setObjectName(QStringLiteral("stitchWidthSpin"));
    m_stitchWidth->setRange(0.1, 100);
    m_stitchWidth->setDecimals(2);
    m_stitchWidth->setSuffix(tr(" mm"));
    form->addRow(tr("Stitch width"), m_stitchWidth);

    m_rowHeight = new QDoubleSpinBox;
    m_rowHeight->setObjectName(QStringLiteral("rowHeightSpin"));
    m_rowHeight->setRange(0.1, 100);
    m_rowHeight->setDecimals(2);
    m_rowHeight->setSuffix(tr(" mm"));
    form->addRow(tr("Row height"), m_rowHeight);

    m_gauge = new QLabel;
    m_gauge->setWordWrap(true);
    form->addRow(tr("Gauge"), m_gauge);

    m_allowance = new QCheckBox(tr("Include seam allowance"));
    m_allowance->setObjectName(QStringLiteral("allowanceCheck"));
    form->addRow(m_allowance);

    m_edgeStitches = new QSpinBox;
    m_edgeStitches->setObjectName(QStringLiteral("edgeStitchesSpin"));
    m_edgeStitches->setRange(0, 10);
    m_edgeStitches->setValue(1);
    form->addRow(tr("Edge stitches"), m_edgeStitches);

    m_increase = new QComboBox;
    m_increase->setObjectName(QStringLiteral("increaseCombo"));
    m_increase->addItem(tr("M1"), int(IncreaseMethod::M1));
    m_increase->addItem(tr("Yarn over"), int(IncreaseMethod::YarnOver));
    m_increase->addItem(tr("Knit front and back"), int(IncreaseMethod::KFB));
    form->addRow(tr("Increase"), m_increase);

    m_decrease = new QComboBox;
    m_decrease->setObjectName(QStringLiteral("decreaseCombo"));
    m_decrease->addItem(tr("ssk / k2tog"), int(DecreaseMethod::K2tog_Ssk));
    m_decrease->addItem(tr("k2tog"), int(DecreaseMethod::K2tog));
    form->addRow(tr("Decrease"), m_decrease);

    m_stats = new QLabel;
    m_stats->setWordWrap(true);
    form->addRow(tr("Piece"), m_stats);

    auto *settingsDock = new QDockWidget(tr("Stitch settings"), this);
    settingsDock->setObjectName(QStringLiteral("settingsDock"));
    settingsDock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea, settingsDock);

    // Bands and overlays of the selected piece
    auto *bandsPanel = new QWidget;
    auto *bandsLayout = new QVBoxLayout(bandsPanel);
    bandsLayout->addWidget(new QLabel(tr("Bands, from the bottom of the piece. The last band reaches the top.")));
    m_bandTable = new QTableWidget(0, 2);
    m_bandTable->setObjectName(QStringLiteral("bandTable"));
    m_bandTable->setHorizontalHeaderLabels({tr("Stitch"), tr("Height")});
    m_bandTable->horizontalHeader()->setStretchLastSection(true);
    m_bandTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_bandTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_bandTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bandsLayout->addWidget(m_bandTable);
    auto *bandButtons = new QHBoxLayout;
    auto *addBandButton = new QPushButton(tr("Add band"));
    addBandButton->setObjectName(QStringLiteral("addBandButton"));
    m_removeBand = new QPushButton(tr("Remove band"));
    m_removeBand->setObjectName(QStringLiteral("removeBandButton"));
    bandButtons->addWidget(addBandButton);
    bandButtons->addWidget(m_removeBand);
    bandsLayout->addLayout(bandButtons);
    bandsLayout->addWidget(new QLabel(tr("Patterns laid over the selected band (special stitch or color):")));
    m_overlayTable = new QTableWidget(0, 7);
    m_overlayTable->setObjectName(QStringLiteral("overlayTable"));
    m_overlayTable->setHorizontalHeaderLabels({tr("Pattern"), tr("First row"), tr("Rows"), tr("First stitch"),
                                               tr("Stitches"), tr("Shift x"), tr("Shift y")});
    m_overlayTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_overlayTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_overlayTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    bandsLayout->addWidget(m_overlayTable);
    auto *overlayButtons = new QHBoxLayout;
    m_addOverlay = new QPushButton(tr("Add pattern"));
    m_addOverlay->setObjectName(QStringLiteral("addOverlayButton"));
    m_removeOverlay = new QPushButton(tr("Remove pattern"));
    m_removeOverlay->setObjectName(QStringLiteral("removeOverlayButton"));
    overlayButtons->addWidget(m_addOverlay);
    overlayButtons->addWidget(m_removeOverlay);
    bandsLayout->addLayout(overlayButtons);
    auto *bandsDock = new QDockWidget(tr("Bands and patterns"), this);
    bandsDock->setObjectName(QStringLiteral("bandsDock"));
    bandsDock->setWidget(bandsPanel);
    addDockWidget(Qt::RightDockWidgetArea, bandsDock);
    tabifyDockWidget(settingsDock, bandsDock);
    settingsDock->raise();
    connect(addBandButton, &QPushButton::clicked, this, &MainWindow::addBand);
    connect(m_removeBand, &QPushButton::clicked, this, &MainWindow::removeBand);
    connect(m_addOverlay, &QPushButton::clicked, this, &MainWindow::addOverlay);
    connect(m_removeOverlay, &QPushButton::clicked, this, &MainWindow::removeOverlay);
    connect(m_bandTable, &QTableWidget::itemSelectionChanged, this, &MainWindow::bandSelected);

    // Instructions
    m_instructions = new QPlainTextEdit;
    m_instructions->setReadOnly(true);
    m_instructions->setLineWrapMode(QPlainTextEdit::NoWrap);
    auto *instrDock = new QDockWidget(tr("Instructions"), this);
    instrDock->setObjectName(QStringLiteral("instructionsDock"));
    instrDock->setWidget(m_instructions);
    addDockWidget(Qt::BottomDockWidgetArea, instrDock);

    connect(m_stitchType, &QComboBox::currentIndexChanged, this, &MainWindow::stitchTypeChanged);
    connect(m_stitchWidth, &QDoubleSpinBox::valueChanged, this, &MainWindow::sizeEdited);
    connect(m_rowHeight, &QDoubleSpinBox::valueChanged, this, &MainWindow::sizeEdited);
    connect(m_allowance, &QCheckBox::toggled, this, &MainWindow::settingsChanged);
    connect(m_edgeStitches, &QSpinBox::valueChanged, this, &MainWindow::settingsChanged);
    connect(m_increase, &QComboBox::currentIndexChanged, this, &MainWindow::settingsChanged);
    connect(m_decrease, &QComboBox::currentIndexChanged, this, &MainWindow::settingsChanged);

    m_stitchType->setCurrentIndex(0);
    stitchTypeChanged();
}

StitchSize MainWindow::currentSize() const
{
    StitchSize s;
    s.stitchWidthMm = m_stitchWidth->value();
    s.rowHeightMm = m_rowHeight->value();
    return s;
}

ShapingOptions MainWindow::currentShaping() const
{
    ShapingOptions o;
    o.edgeStitches = m_edgeStitches->value();
    o.increase = static_cast<IncreaseMethod>(m_increase->currentData().toInt());
    o.decrease = static_cast<DecreaseMethod>(m_decrease->currentData().toInt());
    return o;
}

bool MainWindow::openPattern(const QString &fileName, QString *error, const QString &measurementsOverride)
{
    LoadedPattern loaded;
    QString message;
    if (!loadPattern(fileName, loaded, &message, measurementsOverride))
    {
        if (error) *error = message;
        return false;
    }
    m_pattern = loaded;
    m_pattern.fileName = QFileInfo(fileName).absoluteFilePath();
    m_measurementsOverride = measurementsOverride;
    m_projectFile.clear();
    m_bands.clear();
    m_updating = true;
    m_pieces->clear();
    for (const LoadedPiece &p : m_pattern.pieces)
    {
        m_pieces->addItem(p.name);
    }
    m_updating = false;
    setWindowTitle(QStringLiteral("%1 - Stitchly2D").arg(QFileInfo(fileName).fileName()));
    statusBar()->showMessage(tr("%1 pieces loaded from %2").arg(m_pattern.pieces.size()).arg(fileName));
    if (m_pieces->count() > 0)
    {
        m_pieces->setCurrentRow(0);
    }
    populateBandTables();
    refresh();
    return true;
}

void MainWindow::openFile()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Open pattern"), QString(),
                                                      tr("Seamly2D patterns (*.sm2d *.val);;All files (*)"));
    if (file.isEmpty())
    {
        return;
    }
    QString error;
    if (!openPattern(file, &error))
    {
        QMessageBox::warning(this, tr("Could not load the pattern"), error);
    }
}

void MainWindow::pieceChanged()
{
    if (!m_updating)
    {
        populateBandTables();
        refresh();
    }
}

QVector<BandSpec> MainWindow::bandsFor(const LoadedPiece &piece) const
{
    const auto it = m_bands.constFind(piece.name);
    if (it != m_bands.constEnd() && !it.value().isEmpty())
    {
        return it.value();
    }
    BandSpec def;
    def.stitchType = kDefaultStitch;
    return {def};
}

StitchCatalog MainWindow::workingCatalog() const
{
    StitchCatalog c = m_catalog;
    c.basics().set({kDefaultStitch, currentSize()});
    return c;
}

StitchGrid MainWindow::gridFor(const LoadedPiece &piece, QPolygonF *outline) const
{
    const QPolygonF &source = (m_allowance->isChecked() && !piece.cutLine.isEmpty()) ? piece.cutLine : piece.seamLine;
    const QPolygonF shape = knittingOutline(source, piece.grainline);
    if (outline) *outline = shape;
    return StitchGrid::build(shape, bandsFor(piece), workingCatalog());
}

void MainWindow::populateBandTables()
{
    const bool was = m_updating;
    m_updating = true;
    const int selected = qMax(0, m_bandTable->currentRow());
    m_bandTable->setRowCount(0);
    const int index = m_pieces->currentRow();
    if (index >= 0 && index < m_pattern.pieces.size())
    {
        QStringList types{kDefaultStitch};
        for (const StitchType &t : m_catalog.basics().types()) types << t.name;
        const QVector<BandSpec> bands = bandsFor(m_pattern.pieces.at(index));
        m_bandTable->setRowCount(bands.size());
        for (int r = 0; r < bands.size(); ++r)
        {
            auto *combo = new QComboBox;
            combo->addItems(types);
            combo->setCurrentText(bands.at(r).stitchType);
            connect(combo, &QComboBox::currentIndexChanged, this, &MainWindow::commitBands);
            m_bandTable->setCellWidget(r, 0, combo);
            auto *height = new QDoubleSpinBox;
            height->setRange(0, 5000);
            height->setDecimals(1);
            height->setSuffix(tr(" mm"));
            height->setValue(r == bands.size() - 1 ? 0 : bands.at(r).heightMm);
            height->setEnabled(r != bands.size() - 1);
            height->setSpecialValueText(tr("to the top"));
            connect(height, &QDoubleSpinBox::valueChanged, this, &MainWindow::commitBands);
            m_bandTable->setCellWidget(r, 1, height);
        }
        if (bands.size() > 0)
        {
            m_bandTable->selectRow(qMin(selected, bands.size() - 1));
        }
    }
    m_updating = was;
    populateOverlayTable();
}

void MainWindow::populateOverlayTable()
{
    const bool was = m_updating;
    m_updating = true;
    m_overlayTable->setRowCount(0);
    const int index = m_pieces->currentRow();
    const int band = m_bandTable->currentRow();
    const bool have = index >= 0 && index < m_pattern.pieces.size() && band >= 0;
    m_addOverlay->setEnabled(have);
    m_removeBand->setEnabled(have && m_bandTable->rowCount() > 1);
    m_removeOverlay->setEnabled(false);
    if (have)
    {
        const QVector<BandSpec> bands = bandsFor(m_pattern.pieces.at(index));
        if (band < bands.size())
        {
            const QVector<Overlay> overlays = bands.at(band).overlays;
            m_overlayTable->setRowCount(overlays.size());
            m_removeOverlay->setEnabled(!overlays.isEmpty());
            for (int r = 0; r < overlays.size(); ++r)
            {
                const Overlay &o = overlays.at(r);
                auto *combo = new QComboBox;
                for (const PatternDef &p : m_catalog.patterns())
                {
                    combo->addItem(QStringLiteral("%1 (%2)").arg(p.name, p.kind == PatternDef::Color ? tr("color")
                                                                                                      : tr("special")),
                                   p.name);
                }
                combo->setCurrentIndex(qMax(0, combo->findData(o.pattern)));
                connect(combo, &QComboBox::currentIndexChanged, this, &MainWindow::commitBands);
                m_overlayTable->setCellWidget(r, 0, combo);
                const int values[6] = {o.firstRow, o.rowCount, o.firstStitch, o.stitchCount, o.offsetX, o.offsetY};
                for (int c = 0; c < 6; ++c)
                {
                    auto *spin = new QSpinBox;
                    spin->setRange(c >= 4 ? -1000 : 0, 10000);
                    if (c == 1 || c == 3) spin->setSpecialValueText(tr("all"));
                    spin->setValue(values[c]);
                    connect(spin, &QSpinBox::valueChanged, this, &MainWindow::commitBands);
                    m_overlayTable->setCellWidget(r, c + 1, spin);
                }
            }
            m_overlayTable->resizeColumnsToContents();
        }
    }
    m_updating = was;
}

void MainWindow::bandSelected()
{
    if (!m_updating)
    {
        populateOverlayTable();
    }
}

void MainWindow::commitBands()
{
    const int index = m_pieces->currentRow();
    if (m_updating || index < 0 || index >= m_pattern.pieces.size())
    {
        return;
    }
    const QString name = m_pattern.pieces.at(index).name;
    QVector<BandSpec> bands = bandsFor(m_pattern.pieces.at(index));
    for (int r = 0; r < m_bandTable->rowCount() && r < bands.size(); ++r)
    {
        bands[r].stitchType = qobject_cast<QComboBox *>(m_bandTable->cellWidget(r, 0))->currentText();
        bands[r].heightMm = qobject_cast<QDoubleSpinBox *>(m_bandTable->cellWidget(r, 1))->value();
    }
    const int band = m_bandTable->currentRow();
    if (band >= 0 && band < bands.size())
    {
        QVector<Overlay> overlays;
        for (int r = 0; r < m_overlayTable->rowCount(); ++r)
        {
            Overlay o;
            o.pattern = qobject_cast<QComboBox *>(m_overlayTable->cellWidget(r, 0))->currentData().toString();
            int *fields[6] = {&o.firstRow, &o.rowCount, &o.firstStitch, &o.stitchCount, &o.offsetX, &o.offsetY};
            for (int c = 0; c < 6; ++c)
            {
                *fields[c] = qobject_cast<QSpinBox *>(m_overlayTable->cellWidget(r, c + 1))->value();
            }
            overlays.append(o);
        }
        bands[band].overlays = overlays;
    }
    m_bands.insert(name, bands);
    refresh();
}

void MainWindow::addBand()
{
    const int index = m_pieces->currentRow();
    if (index < 0 || index >= m_pattern.pieces.size())
    {
        return;
    }
    QVector<BandSpec> bands = bandsFor(m_pattern.pieces.at(index));
    if (bands.last().heightMm <= 0)
    {
        bands.last().heightMm = 50;     // the old last band now ends somewhere
    }
    BandSpec band;
    band.stitchType = kDefaultStitch;
    bands.append(band);
    m_bands.insert(m_pattern.pieces.at(index).name, bands);
    populateBandTables();
    m_bandTable->selectRow(bands.size() - 1);
    refresh();
}

void MainWindow::removeBand()
{
    const int index = m_pieces->currentRow();
    const int band = m_bandTable->currentRow();
    if (index < 0 || index >= m_pattern.pieces.size() || band < 0)
    {
        return;
    }
    QVector<BandSpec> bands = bandsFor(m_pattern.pieces.at(index));
    if (bands.size() <= 1)
    {
        return;
    }
    bands.removeAt(band);
    bands.last().heightMm = 0;
    m_bands.insert(m_pattern.pieces.at(index).name, bands);
    populateBandTables();
    refresh();
}

void MainWindow::addOverlay()
{
    const int index = m_pieces->currentRow();
    const int band = m_bandTable->currentRow();
    if (index < 0 || index >= m_pattern.pieces.size() || band < 0 || m_catalog.patterns().isEmpty())
    {
        return;
    }
    QVector<BandSpec> bands = bandsFor(m_pattern.pieces.at(index));
    Overlay o;
    o.pattern = m_catalog.patterns().first().name;
    bands[band].overlays.append(o);
    m_bands.insert(m_pattern.pieces.at(index).name, bands);
    populateOverlayTable();
    refresh();
}

void MainWindow::removeOverlay()
{
    const int index = m_pieces->currentRow();
    const int band = m_bandTable->currentRow();
    const int row = m_overlayTable->currentRow() >= 0 ? m_overlayTable->currentRow() : m_overlayTable->rowCount() - 1;
    if (index < 0 || index >= m_pattern.pieces.size() || band < 0 || row < 0)
    {
        return;
    }
    QVector<BandSpec> bands = bandsFor(m_pattern.pieces.at(index));
    if (row < bands[band].overlays.size())
    {
        bands[band].overlays.removeAt(row);
    }
    m_bands.insert(m_pattern.pieces.at(index).name, bands);
    populateOverlayTable();
    refresh();
}

void MainWindow::stitchTypeChanged()
{
    const QString name = m_stitchType->currentText();
    if (name != kCustom)
    {
        const StitchType t = m_catalog.basics().find(name);
        m_updating = true;
        m_stitchWidth->setValue(t.size.stitchWidthMm);
        m_rowHeight->setValue(t.size.rowHeightMm);
        m_updating = false;
    }
    refresh();
}

void MainWindow::sizeEdited()
{
    if (m_updating)
    {
        return;
    }
    // Typing a size that is not the catalog's one makes it a custom stitch.
    const StitchType t = m_catalog.basics().find(m_stitchType->currentText());
    if (m_stitchType->currentText() != kCustom
        && (!qFuzzyCompare(t.size.stitchWidthMm, m_stitchWidth->value())
            || !qFuzzyCompare(t.size.rowHeightMm, m_rowHeight->value())))
    {
        m_updating = true;
        m_stitchType->setCurrentText(kCustom);
        m_updating = false;
    }
    refresh();
}

void MainWindow::settingsChanged()
{
    if (!m_updating)
    {
        refresh();
    }
}

void MainWindow::refresh()
{
    if (m_updating || m_stitchWidth == nullptr || m_instructions == nullptr)
    {
        return;
    }
    const StitchSize size = currentSize();
    m_gauge->setText(tr("%1 sts x %2 rows per 10 cm\n%3 sts x %4 rows per inch")
                         .arg(size.stitchesPer10cm(), 0, 'f', 1).arg(size.rowsPer10cm(), 0, 'f', 1)
                         .arg(size.stitchesPerInch(), 0, 'f', 1).arg(size.rowsPerInch(), 0, 'f', 1));

    m_scene->clear();
    m_grid = StitchGrid();
    m_instructions->clear();
    m_stats->clear();

    const int index = m_pieces->currentRow();
    if (index < 0 || index >= m_pattern.pieces.size())
    {
        return;
    }
    const LoadedPiece &piece = m_pattern.pieces.at(index);
    QPolygonF outline;
    m_grid = gridFor(piece, &outline);

    if (m_grid.isEmpty())
    {
        m_stats->setText(tr("No stitches (check the stitch types and patterns of the bands)"));
        return;
    }
    const QRectF box = outline.boundingRect();
    m_stats->setText(tr("%1 x %2 mm\n%3 rows, cast on %4, widest %5, bind off %6%7")
                         .arg(box.width(), 0, 'f', 0).arg(box.height(), 0, 'f', 0)
                         .arg(m_grid.rowCount()).arg(m_grid.castOn()).arg(m_grid.width()).arg(m_grid.bindOff())
                         .arg(piece.grainline.isNull() ? tr("\nNo grainline: drawn orientation") : QString()));
    m_instructions->setPlainText(writeInstructions(m_grid, currentShaping()).join(QLatin1Char('\n')));
    showGrid(outline, m_grid);
}

void MainWindow::showGrid(const QPolygonF &outline, const StitchGrid &grid)
{
    const qreal height = outline.boundingRect().height();
    auto sx = [](qreal x) { return x * kScale; };
    auto sy = [height](qreal y) { return (height - y) * kScale; };   // y up in the grid, y down on the canvas

    QPainterPath cells;
    for (const Row &row : grid.rows())
    {
        const GridBand &band = grid.bands().at(row.band);
        const qreal sw = band.size.stitchWidthMm;
        const qreal x0 = band.xOriginMm + row.firstStitch * sw;
        cells.addRect(QRectF(QPointF(sx(x0), sy(row.y1Mm)), QPointF(sx(x0 + row.count * sw), sy(row.y0Mm))));
        for (int s = 1; s < row.count; ++s)
        {
            cells.moveTo(sx(x0 + s * sw), sy(row.y1Mm));
            cells.lineTo(sx(x0 + s * sw), sy(row.y0Mm));
        }
    }
    // Colors and stitch symbols from overlays
    QHash<QString, QPainterPath> colored;
    QPainterPath purls;
    bool overlays = false;
    for (const GridBand &b : grid.bands()) overlays = overlays || !b.overlays.isEmpty();
    if (overlays)
    {
        for (const Row &row : grid.rows())
        {
            const GridBand &band = grid.bands().at(row.band);
            const qreal sw = band.size.stitchWidthMm;
            const qreal x0 = band.xOriginMm + row.firstStitch * sw;
            for (int s = 0; s < row.count; ++s)
            {
                const Cell c = grid.cell(row.index, row.firstStitch + s);
                const QRectF r(QPointF(sx(x0 + s * sw), sy(row.y1Mm)), QPointF(sx(x0 + (s + 1) * sw), sy(row.y0Mm)));
                if (!c.color.isEmpty())
                {
                    colored[c.color].addRect(r);
                }
                if (c.special == QLatin1String("p"))
                {
                    purls.addEllipse(r.center(), r.width() * 0.18, r.width() * 0.18);
                }
            }
        }
    }
    for (auto it = colored.cbegin(); it != colored.cend(); ++it)
    {
        m_scene->addPath(it.value(), Qt::NoPen, QBrush(yarnColor(it.key())));
    }
    QPen cellPen(QColor(120, 140, 170));
    cellPen.setCosmetic(true);
    cellPen.setWidthF(0.6);
    // The grid is drawn first and the colors on top of it, so the cell lines stay visible.
    auto *gridItem = m_scene->addPath(cells, cellPen, overlays ? QBrush(Qt::white) : QBrush(QColor(235, 240, 248)));
    gridItem->setZValue(-2);
    for (QGraphicsItem *item : m_scene->items())
    {
        if (item != gridItem && item->type() == QGraphicsPathItem::Type) item->setZValue(-1);
    }
    if (overlays)
    {
        QPen linePen = cellPen;
        linePen.setColor(QColor(90, 90, 90, 140));
        auto *lines = m_scene->addPath(cells, linePen, Qt::NoBrush);
        lines->setZValue(-0.5);
    }
    if (!purls.isEmpty())
    {
        auto *dots = m_scene->addPath(purls, Qt::NoPen, QBrush(QColor(30, 30, 30)));
        dots->setZValue(-0.4);
    }

    QPolygonF shape;
    for (const QPointF &p : outline)
    {
        shape << QPointF(sx(p.x()), sy(p.y()));
    }
    QPen outlinePen(QColor(200, 30, 30));
    outlinePen.setCosmetic(true);
    outlinePen.setWidthF(2.0);
    m_scene->addPolygon(shape, outlinePen, Qt::NoBrush);

    // Row numbers every 10 rows, on the left of the widest row.
    QFont font;
    font.setPointSizeF(8);
    for (const Row &row : grid.rows())
    {
        if ((row.index + 1) % 10 != 0 && row.index != 0)
        {
            continue;
        }
        auto *label = m_scene->addSimpleText(QString::number(row.index + 1), font);
        label->setBrush(QColor(60, 60, 60));
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations);
        label->setPos(sx(-3.0) - label->boundingRect().width(), sy((row.y0Mm + row.y1Mm) / 2) - label->boundingRect().height() / 2);
    }

    m_scene->setSceneRect(m_scene->itemsBoundingRect().adjusted(-40, -40, 40, 40));
    fitAll();
}

bool MainWindow::saveProject(const QString &fileName, QString *error)
{
    if (m_pattern.fileName.isEmpty())
    {
        if (error) *error = tr("Open a pattern first.");
        return false;
    }
    Project p;
    p.patternFile = m_pattern.fileName;
    p.measurementsFile = m_measurementsOverride.isEmpty() ? QString() : QFileInfo(m_measurementsOverride).absoluteFilePath();
    p.settings = settings();
    for (auto it = m_bands.cbegin(); it != m_bands.cend(); ++it)
    {
        p.bands.insert(it.key(), it.value());
    }
    p.catalog = usedCatalog(m_catalog, p.bands);
    if (!stitchcore::saveProject(fileName, p, error))
    {
        return false;
    }
    m_projectFile = fileName;
    setWindowTitle(QStringLiteral("%1 - Stitchly2D").arg(QFileInfo(fileName).fileName()));
    statusBar()->showMessage(tr("Project saved to %1").arg(fileName), 5000);
    return true;
}

bool MainWindow::openProject(const QString &fileName, QString *error, CatalogConflict conflict)
{
    Project p;
    if (!stitchcore::loadProject(fileName, p, error))
    {
        return false;
    }
    // The pattern first: if it cannot be loaded, nothing else changes.
    QString message;
    if (!openPattern(p.patternFile, &message, p.measurementsFile))
    {
        if (error) *error = tr("The pattern of the project could not be loaded (%1): %2").arg(p.patternFile, message);
        return false;
    }

    // The catalog entries the project was made with
    const CatalogDiff diff = diffCatalog(m_catalog, p.catalog);
    bool replace = conflict == CatalogConflict::UseProject;
    if (!diff.differing.isEmpty() && conflict == CatalogConflict::Ask)
    {
        QMessageBox box(QMessageBox::Question, tr("Open project"),
                        tr("These stitches or patterns in your catalog differ from the ones the project was made with:\n\n%1\n\n"
                           "Use the project's versions? (This changes your catalog for this session; the catalog "
                           "file is only rewritten when you edit the catalog.)").arg(diff.differing.join(QStringLiteral(", "))),
                        QMessageBox::NoButton, this);
        QPushButton *useProject = box.addButton(tr("Use the project's"), QMessageBox::AcceptRole);
        box.addButton(tr("Keep mine"), QMessageBox::RejectRole);
        box.exec();
        replace = box.clickedButton() == useProject;
    }
    StitchCatalog merged = m_catalog;
    mergeCatalog(merged, p.catalog, replace);

    // The catalog first (it refreshes the stitch list), then the settings, then the bands
    setCatalog(merged);
    setSettings(p.settings);

    m_bands.clear();
    for (auto it = p.bands.cbegin(); it != p.bands.cend(); ++it)
    {
        m_bands.insert(it.key(), it.value());
    }
    m_projectFile = fileName;
    setWindowTitle(QStringLiteral("%1 - Stitchly2D").arg(QFileInfo(fileName).fileName()));
    populateBandTables();
    refresh();
    return true;
}

void MainWindow::openProjectDialog()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Open project"), QString(),
                                                      tr("Stitchly2D projects (*.stitchly);;All files (*)"));
    if (file.isEmpty())
    {
        return;
    }
    QString error;
    if (!openProject(file, &error))
    {
        QMessageBox::warning(this, tr("Could not open the project"), error);
    }
}

void MainWindow::saveProjectAction()
{
    if (m_projectFile.isEmpty())
    {
        saveProjectAs();
        return;
    }
    QString error;
    if (!saveProject(m_projectFile, &error))
    {
        QMessageBox::warning(this, tr("Could not save the project"), error);
    }
}

void MainWindow::saveProjectAs()
{
    if (m_pattern.fileName.isEmpty())
    {
        QMessageBox::information(this, tr("Save project"), tr("Open a pattern first."));
        return;
    }
    QString suggested = m_projectFile.isEmpty()
                            ? QFileInfo(m_pattern.fileName).absolutePath() + QLatin1Char('/')
                                  + QFileInfo(m_pattern.fileName).completeBaseName() + QStringLiteral(".stitchly")
                            : m_projectFile;
    QString file = QFileDialog::getSaveFileName(this, tr("Save project"), suggested,
                                                tr("Stitchly2D projects (*.stitchly)"));
    if (file.isEmpty())
    {
        return;
    }
    if (QFileInfo(file).suffix().isEmpty())
    {
        file += QStringLiteral(".stitchly");
    }
    QString error;
    if (!saveProject(file, &error))
    {
        QMessageBox::warning(this, tr("Could not save the project"), error);
    }
}

void MainWindow::editCatalog()
{
    CatalogDialog dialog(m_catalog, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }
    setCatalog(dialog.catalog());
    QDir().mkpath(QFileInfo(catalogPath()).absolutePath());
    if (m_catalog.save(catalogPath()))
    {
        statusBar()->showMessage(tr("Catalog saved"), 5000);
    }
    else
    {
        QMessageBox::warning(this, tr("Catalog"), tr("The catalog could not be saved to %1.").arg(catalogPath()));
    }
}

void MainWindow::setCatalog(const StitchCatalog &catalog)
{
    m_catalog = catalog;

    // Basic stitch list of the settings dock
    const QString previous = m_stitchType->currentText();
    m_updating = true;
    m_stitchType->clear();
    for (const StitchType &t : m_catalog.basics().types())
    {
        m_stitchType->addItem(t.name);
    }
    m_stitchType->addItem(kCustom);
    const int keep = m_stitchType->findText(previous);
    m_stitchType->setCurrentIndex(keep >= 0 ? keep : 0);
    m_updating = false;
    if (m_stitchType->currentText() != kCustom)
    {
        stitchTypeChanged();        // takes over the sizes, which may have been edited
    }

    // Bands and patterns that point at things which are gone
    for (auto it = m_bands.begin(); it != m_bands.end(); ++it)
    {
        for (BandSpec &band : it.value())
        {
            if (band.stitchType != kDefaultStitch && !m_catalog.basics().contains(band.stitchType))
            {
                band.stitchType = kDefaultStitch;
            }
            QVector<Overlay> kept;
            for (const Overlay &o : band.overlays)
            {
                if (m_catalog.contains(o.pattern)) kept.append(o);
            }
            band.overlays = kept;
        }
    }
    populateBandTables();
    refresh();
}

void MainWindow::fitAll()
{
    if (m_scene->items().isEmpty())
    {
        return;
    }
    m_view->fitInView(m_scene->itemsBoundingRect().adjusted(-30, -30, 30, 30), Qt::KeepAspectRatio);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // The canvas only has its real size once the window is shown.
    QTimer::singleShot(0, this, &MainWindow::fitAll);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (isVisible() && !m_grid.isEmpty())
    {
        QTimer::singleShot(0, this, &MainWindow::fitAll);
    }
}

QString MainWindow::instructionsText() const
{
    return m_instructions->toPlainText();
}

bool MainWindow::saveText(const QString &title, const QString &filter, const QString &suggested, const QString &text)
{
    const QString path = QFileDialog::getSaveFileName(this, title, suggested, filter);
    if (path.isEmpty())
    {
        return false;
    }
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(text.toUtf8()) < 0 || !f.commit())
    {
        QMessageBox::warning(this, title, f.errorString());
        return false;
    }
    statusBar()->showMessage(tr("Saved %1").arg(path), 5000);
    return true;
}

void MainWindow::exportInstructions()
{
    if (m_grid.isEmpty())
    {
        return;
    }
    // All pieces, so the file is a complete knitting pattern.
    saveText(tr("Export instructions"), tr("Text files (*.txt)"), QStringLiteral("instructions.txt"),
             allInstructionsText());
}

QString MainWindow::allInstructionsText() const
{
    QString text;
    for (const LoadedPiece &piece : m_pattern.pieces)
    {
        const StitchGrid grid = gridFor(piece);
        text += QStringLiteral("== %1 ==\n").arg(piece.name);
        text += writeInstructions(grid, currentShaping()).join(QLatin1Char('\n')) + QStringLiteral("\n\n");
    }
    return text;
}

// ---------------------------------------------------------------------------------------------------------------------
// The window's state, for the Claude (MCP) tools. They act on exactly what the user sees.

const LoadedPiece *MainWindow::findPiece(const QString &name) const
{
    for (const LoadedPiece &p : m_pattern.pieces)
    {
        if (p.name == name) return &p;
    }
    return nullptr;
}

QStringList MainWindow::pieceNames() const
{
    QStringList names;
    for (const LoadedPiece &p : m_pattern.pieces) names << p.name;
    return names;
}

QString MainWindow::currentPiece() const
{
    const int row = m_pieces->currentRow();
    return row >= 0 && row < m_pattern.pieces.size() ? m_pattern.pieces.at(row).name : QString();
}

bool MainWindow::selectPiece(const QString &name)
{
    for (int i = 0; i < m_pattern.pieces.size(); ++i)
    {
        if (m_pattern.pieces.at(i).name == name)
        {
            m_pieces->setCurrentRow(i);
            return true;
        }
    }
    return false;
}

ProjectSettings MainWindow::settings() const
{
    ProjectSettings s;
    s.stitchType = m_stitchType->currentText() == kCustom ? QString() : m_stitchType->currentText();
    s.size = currentSize();
    s.includeSeamAllowance = m_allowance->isChecked();
    s.shaping = currentShaping();
    return s;
}

void MainWindow::setSettings(const ProjectSettings &s)
{
    m_updating = true;
    m_allowance->setChecked(s.includeSeamAllowance);
    m_edgeStitches->setValue(s.shaping.edgeStitches);
    m_increase->setCurrentIndex(m_increase->findData(int(s.shaping.increase)));
    m_decrease->setCurrentIndex(m_decrease->findData(int(s.shaping.decrease)));
    // A catalog stitch is only shown by name when its size is the one wanted; otherwise it is a custom stitch.
    const StitchType named = m_catalog.basics().find(s.stitchType);
    const bool sameAsCatalog = !s.stitchType.isEmpty() && !named.name.isEmpty()
                               && qFuzzyCompare(named.size.stitchWidthMm, s.size.stitchWidthMm)
                               && qFuzzyCompare(named.size.rowHeightMm, s.size.rowHeightMm);
    m_stitchType->setCurrentIndex(sameAsCatalog ? m_stitchType->findText(s.stitchType)
                                                : m_stitchType->findText(kCustom));
    m_stitchWidth->setValue(s.size.stitchWidthMm);
    m_rowHeight->setValue(s.size.rowHeightMm);
    m_updating = false;
    refresh();
}

QVector<BandSpec> MainWindow::bands(const QString &piece) const
{
    const LoadedPiece *p = findPiece(piece);
    return p ? bandsFor(*p) : QVector<BandSpec>();
}

bool MainWindow::setBands(const QString &piece, const QVector<BandSpec> &bands, QString *error)
{
    auto fail = [error](const QString &msg) {
        if (error) *error = msg;
        return false;
    };
    if (!findPiece(piece))
    {
        return fail(tr("No piece named \"%1\".").arg(piece));
    }
    if (bands.isEmpty())
    {
        return fail(tr("A piece needs at least one band."));
    }
    for (int i = 0; i < bands.size(); ++i)
    {
        const BandSpec &b = bands.at(i);
        if (b.stitchType != kDefaultStitch && !m_catalog.basics().contains(b.stitchType))
        {
            return fail(tr("Band %1: unknown basic stitch \"%2\".").arg(i + 1).arg(b.stitchType));
        }
        if (b.heightMm < 0 || (i < bands.size() - 1 && b.heightMm <= 0))
        {
            return fail(tr("Band %1: only the last band may have no height.").arg(i + 1));
        }
        for (const Overlay &o : b.overlays)
        {
            if (!m_catalog.contains(o.pattern))
            {
                return fail(tr("Band %1: unknown pattern \"%2\".").arg(i + 1).arg(o.pattern));
            }
            if (o.firstRow < 0 || o.rowCount < 0 || o.firstStitch < 0 || o.stitchCount < 0)
            {
                return fail(tr("Band %1: rows and stitches of a pattern must not be negative.").arg(i + 1));
            }
        }
    }
    QVector<BandSpec> fixed = bands;
    fixed.last().heightMm = 0;                   // the last band always reaches the top
    m_bands.insert(piece, fixed);
    if (piece == currentPiece())
    {
        populateBandTables();
    }
    refresh();
    return true;
}

QJsonObject MainWindow::pieceSummary(const QString &name) const
{
    const LoadedPiece *piece = findPiece(name);
    if (!piece)
    {
        return QJsonObject();
    }
    QPolygonF outline;
    const StitchGrid grid = gridFor(*piece, &outline);
    const QRectF box = outline.boundingRect();
    QJsonArray bandList;
    for (const GridBand &b : grid.bands())
    {
        bandList.append(QJsonObject{{QStringLiteral("stitchType"), b.stitchType},
                                    {QStringLiteral("firstRow"), b.firstRow + 1},
                                    {QStringLiteral("rows"), b.rowCount},
                                    {QStringLiteral("stitchWidthMm"), b.size.stitchWidthMm},
                                    {QStringLiteral("rowHeightMm"), b.size.rowHeightMm},
                                    {QStringLiteral("patterns"), b.overlays.size()}});
    }
    return QJsonObject{{QStringLiteral("name"), piece->name},
                       {QStringLiteral("widthMm"), qRound(box.width() * 10) / 10.0},
                       {QStringLiteral("heightMm"), qRound(box.height() * 10) / 10.0},
                       {QStringLiteral("hasGrainline"), !piece->grainline.isNull()},
                       {QStringLiteral("hasStitches"), !grid.isEmpty()},
                       {QStringLiteral("rows"), grid.rowCount()},
                       {QStringLiteral("castOn"), grid.castOn()},
                       {QStringLiteral("widestRow"), grid.width()},
                       {QStringLiteral("bindOff"), grid.bindOff()},
                       {QStringLiteral("bands"), bandList}};
}

QStringList MainWindow::instructionsFor(const QString &piece) const
{
    const LoadedPiece *p = findPiece(piece);
    return p ? writeInstructions(gridFor(*p), currentShaping()) : QStringList();
}

QString MainWindow::rowTableFor(const QString &piece) const
{
    const LoadedPiece *p = findPiece(piece);
    return p ? rowTableCsv(gridFor(*p)) : QString();
}

QByteArray MainWindow::screenshotPng()
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    grab().save(&buffer, "PNG");
    return bytes;
}

QJsonObject MainWindow::mcpProjectInfo() const
{
    return QJsonObject{{QStringLiteral("file"), m_pattern.fileName},
                       {QStringLiteral("project"), m_projectFile},
                       {QStringLiteral("modified"), false},     // there is no unsaved-changes tracking yet
                       {QStringLiteral("selectedPiece"), currentPiece()},
                       {QStringLiteral("pieceCount"), m_pattern.pieces.size()}};
}

void MainWindow::exportRowTable()
{
    if (!m_grid.isEmpty())
    {
        saveText(tr("Export row table"), tr("CSV files (*.csv)"), QStringLiteral("rows.csv"), rowTableCsv(m_grid));
    }
}

void MainWindow::exportImage()
{
    if (m_grid.isEmpty())
    {
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("Export chart image"), QStringLiteral("chart.png"),
                                                      tr("PNG images (*.png)"));
    if (path.isEmpty())
    {
        return;
    }
    const QRectF rect = m_scene->sceneRect();
    QImage image(rect.size().toSize() * 2, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&painter, QRectF(), rect);
    painter.end();
    if (!image.save(path))
    {
        QMessageBox::warning(this, tr("Export chart image"), tr("Could not write %1").arg(path));
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About Stitchly2D"),
                       tr("<b>Stitchly2D</b><br>Prepares Seamly2D patterns for knitting.<br><br>"
                          "Built on the Seamly2D libraries (GPLv3)."));
}
