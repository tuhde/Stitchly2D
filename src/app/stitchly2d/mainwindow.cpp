#include "mainwindow.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsSimpleTextItem>
#include <QImage>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QSpinBox>
#include <QStandardPaths>
#include <QShowEvent>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

#include "../../libs/stitchcore/instructions.h"
#include "../../libs/stitchcore/pieceoutline.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

using namespace stitchcore;

namespace
{
const qreal kScale = 4.0;           // scene units per millimetre
const QString kCustom = QStringLiteral("Custom");

QString catalogPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/catalog.json");
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
}

void MainWindow::createActions()
{
    QMenu *file = menuBar()->addMenu(tr("&File"));
    QAction *open = file->addAction(tr("&Open pattern..."), this, &MainWindow::openFile, QKeySequence::Open);
    file->addSeparator();
    file->addAction(tr("Export &instructions..."), this, &MainWindow::exportInstructions);
    file->addAction(tr("Export &row table (CSV)..."), this, &MainWindow::exportRowTable);
    file->addAction(tr("Export &chart image..."), this, &MainWindow::exportImage);
    file->addSeparator();
    file->addAction(tr("&Quit"), this, &QWidget::close, QKeySequence::Quit);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QAction *fit = viewMenu->addAction(tr("Zoom to &fit"), this, &MainWindow::fitAll,
                                       QKeySequence(Qt::CTRL | Qt::Key_0));
    QAction *zin = viewMenu->addAction(tr("Zoom &in"), m_view, &VMainGraphicsView::zoomIn, QKeySequence::ZoomIn);
    QAction *zout = viewMenu->addAction(tr("Zoom &out"), m_view, &VMainGraphicsView::zoomOut, QKeySequence::ZoomOut);
    viewMenu->addSeparator();
    for (QDockWidget *dock : findChildren<QDockWidget *>())
    {
        viewMenu->addAction(dock->toggleViewAction());
    }

    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(tr("&About Stitchly2D"), this, &MainWindow::about);

    QToolBar *bar = addToolBar(tr("Tools"));
    bar->setObjectName(QStringLiteral("toolbar"));
    bar->addAction(open);
    bar->addSeparator();
    bar->addAction(fit);
    bar->addAction(zin);
    bar->addAction(zout);
}

void MainWindow::createDocks()
{
    // Pieces
    m_pieces = new QListWidget;
    connect(m_pieces, &QListWidget::currentRowChanged, this, &MainWindow::pieceChanged);
    auto *piecesDock = new QDockWidget(tr("Pieces"), this);
    piecesDock->setObjectName(QStringLiteral("piecesDock"));
    piecesDock->setWidget(m_pieces);
    addDockWidget(Qt::LeftDockWidgetArea, piecesDock);

    // Stitch settings
    auto *panel = new QWidget;
    auto *form = new QFormLayout(panel);

    m_stitchType = new QComboBox;
    for (const StitchType &t : m_catalog.basics().types())
    {
        m_stitchType->addItem(t.name);
    }
    m_stitchType->addItem(kCustom);
    form->addRow(tr("Basic stitch"), m_stitchType);

    m_stitchWidth = new QDoubleSpinBox;
    m_stitchWidth->setRange(0.1, 100);
    m_stitchWidth->setDecimals(2);
    m_stitchWidth->setSuffix(tr(" mm"));
    form->addRow(tr("Stitch width"), m_stitchWidth);

    m_rowHeight = new QDoubleSpinBox;
    m_rowHeight->setRange(0.1, 100);
    m_rowHeight->setDecimals(2);
    m_rowHeight->setSuffix(tr(" mm"));
    form->addRow(tr("Row height"), m_rowHeight);

    m_gauge = new QLabel;
    m_gauge->setWordWrap(true);
    form->addRow(tr("Gauge"), m_gauge);

    m_allowance = new QCheckBox(tr("Include seam allowance"));
    form->addRow(m_allowance);

    m_edgeStitches = new QSpinBox;
    m_edgeStitches->setRange(0, 10);
    m_edgeStitches->setValue(1);
    form->addRow(tr("Edge stitches"), m_edgeStitches);

    m_increase = new QComboBox;
    m_increase->addItem(tr("M1"), int(IncreaseMethod::M1));
    m_increase->addItem(tr("Yarn over"), int(IncreaseMethod::YarnOver));
    m_increase->addItem(tr("Knit front and back"), int(IncreaseMethod::KFB));
    form->addRow(tr("Increase"), m_increase);

    m_decrease = new QComboBox;
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

bool MainWindow::openPattern(const QString &fileName, QString *error)
{
    LoadedPattern loaded;
    QString message;
    if (!loadPattern(fileName, loaded, &message))
    {
        if (error) *error = message;
        return false;
    }
    m_pattern = loaded;
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
        refresh();
    }
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
    const QPolygonF &source = (m_allowance->isChecked() && !piece.cutLine.isEmpty()) ? piece.cutLine : piece.seamLine;
    const QPolygonF outline = knittingOutline(source, piece.grainline);
    m_grid = StitchGrid::build(outline, size);

    if (m_grid.isEmpty())
    {
        m_stats->setText(tr("No stitches"));
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
    QPen cellPen(QColor(120, 140, 170));
    cellPen.setCosmetic(true);
    cellPen.setWidthF(0.6);
    QPainterPath cellPath = cells;
    m_scene->addPath(cellPath, cellPen, QBrush(QColor(235, 240, 248)));

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
    QString text;
    const StitchSize size = currentSize();
    for (const LoadedPiece &piece : m_pattern.pieces)
    {
        const QPolygonF &source = (m_allowance->isChecked() && !piece.cutLine.isEmpty()) ? piece.cutLine
                                                                                         : piece.seamLine;
        const StitchGrid grid = StitchGrid::build(knittingOutline(source, piece.grainline), size);
        text += QStringLiteral("== %1 ==\n").arg(piece.name);
        text += writeInstructions(grid, currentShaping()).join(QLatin1Char('\n')) + QStringLiteral("\n\n");
    }
    saveText(tr("Export instructions"), tr("Text files (*.txt)"), QStringLiteral("instructions.txt"), text);
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
