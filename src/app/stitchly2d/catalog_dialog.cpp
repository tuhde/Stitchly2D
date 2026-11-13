#include "catalog_dialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QSpinBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include "core/colors.h"

using namespace stitchcore;

CatalogDialog::CatalogDialog(const StitchCatalog &catalog, QWidget *parent)
    : QDialog(parent)
    , m_catalog(catalog)
{
    setWindowTitle(tr("Catalog of stitches and patterns"));
    resize(860, 560);

    auto *tabs = new QTabWidget;

    // ---- Basic stitches
    auto *basicsPage = new QWidget;
    auto *basicsLayout = new QVBoxLayout(basicsPage);
    basicsLayout->addWidget(new QLabel(tr("Basic stitches. The width of one stitch and the height of one row depend "
                                          "on the stitch; measure them on a swatch.")));
    m_basics = new QTableWidget(0, 3);
    m_basics->setObjectName(QStringLiteral("basicsTable"));
    m_basics->setHorizontalHeaderLabels({tr("Name"), tr("Stitch width"), tr("Row height")});
    m_basics->horizontalHeader()->setStretchLastSection(true);
    m_basics->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    basicsLayout->addWidget(m_basics);
    auto *basicsButtons = new QHBoxLayout;
    auto *addBasicButton = new QPushButton(tr("Add stitch"));
    addBasicButton->setObjectName(QStringLiteral("addBasicButton"));
    auto *removeBasicButton = new QPushButton(tr("Remove stitch"));
    removeBasicButton->setObjectName(QStringLiteral("removeBasicButton"));
    basicsButtons->addWidget(addBasicButton);
    basicsButtons->addWidget(removeBasicButton);
    basicsButtons->addStretch();
    basicsLayout->addLayout(basicsButtons);
    tabs->addTab(basicsPage, tr("Basic stitches"));

    // ---- Patterns
    auto *patternsPage = new QWidget;
    auto *patternsLayout = new QHBoxLayout(patternsPage);

    auto *left = new QVBoxLayout;
    m_list = new QListWidget;
    m_list->setObjectName(QStringLiteral("patternList"));
    m_list->setMinimumWidth(230);
    left->addWidget(m_list);
    auto *newButton = new QPushButton(tr("New pattern"));
    newButton->setObjectName(QStringLiteral("newPatternButton"));
    auto *deleteButton = new QPushButton(tr("Delete pattern"));
    deleteButton->setObjectName(QStringLiteral("deletePatternButton"));
    auto *importButton = new QPushButton(tr("Import (.stpat)..."));
    importButton->setObjectName(QStringLiteral("importButton"));
    auto *exportButton = new QPushButton(tr("Export selected..."));
    exportButton->setObjectName(QStringLiteral("exportButton"));
    auto *exportAllButton = new QPushButton(tr("Export all..."));
    exportAllButton->setObjectName(QStringLiteral("exportAllButton"));
    left->addWidget(newButton);
    left->addWidget(deleteButton);
    left->addWidget(importButton);
    left->addWidget(exportButton);
    left->addWidget(exportAllButton);
    patternsLayout->addLayout(left, 1);

    auto *right = new QVBoxLayout;
    auto *form = new QFormLayout;
    m_name = new QLineEdit;
    m_name->setObjectName(QStringLiteral("patternName"));
    form->addRow(tr("Name"), m_name);
    m_kind = new QComboBox;
    m_kind->setObjectName(QStringLiteral("patternKind"));
    m_kind->addItem(tr("Special stitch (symbols such as k, p, C4F)"), int(PatternDef::Special));
    m_kind->addItem(tr("Color (yarn names such as A, MC or #c03030)"), int(PatternDef::Color));
    form->addRow(tr("Kind"), m_kind);
    m_width = new QSpinBox;
    m_width->setObjectName(QStringLiteral("patternWidth"));
    m_width->setRange(1, 200);
    m_width->setSuffix(tr(" stitches"));
    form->addRow(tr("Repeat width"), m_width);
    m_height = new QSpinBox;
    m_height->setObjectName(QStringLiteral("patternHeight"));
    m_height->setRange(1, 200);
    m_height->setSuffix(tr(" rows"));
    form->addRow(tr("Repeat height"), m_height);
    right->addLayout(form);
    right->addWidget(new QLabel(tr("Chart of the repeat: the top row is the last row of the repeat. "
                                   "Type a symbol or yarn name in each cell; an empty cell leaves the base stitch.")));
    m_grid = new QTableWidget(0, 0);
    m_grid->setObjectName(QStringLiteral("patternGrid"));
    m_grid->horizontalHeader()->setDefaultSectionSize(48);
    m_grid->verticalHeader()->setDefaultSectionSize(28);
    right->addWidget(m_grid, 1);
    patternsLayout->addLayout(right, 3);
    tabs->addTab(patternsPage, tr("Patterns"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &CatalogDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs);
    layout->addWidget(buttons);

    connect(addBasicButton, &QPushButton::clicked, this, &CatalogDialog::addBasic);
    connect(removeBasicButton, &QPushButton::clicked, this, &CatalogDialog::removeBasic);
    connect(newButton, &QPushButton::clicked, this, &CatalogDialog::newPattern);
    connect(deleteButton, &QPushButton::clicked, this, &CatalogDialog::deletePattern);
    connect(importButton, &QPushButton::clicked, this, &CatalogDialog::importClicked);
    connect(exportButton, &QPushButton::clicked, this, &CatalogDialog::exportSelected);
    connect(exportAllButton, &QPushButton::clicked, this, &CatalogDialog::exportAll);
    connect(m_list, &QListWidget::currentRowChanged, this, &CatalogDialog::patternSelected);
    connect(m_width, &QSpinBox::valueChanged, this, &CatalogDialog::gridSizeChanged);
    connect(m_height, &QSpinBox::valueChanged, this, &CatalogDialog::gridSizeChanged);
    connect(m_kind, &QComboBox::currentIndexChanged, this, &CatalogDialog::cellChanged);
    // Keep the list entry in step with what is typed.
    auto updateListText = [this]() {
        if (m_loading || m_current < 0 || m_current >= m_list->count()) return;
        const bool color = m_kind->currentData().toInt() == int(PatternDef::Color);
        m_list->item(m_current)->setText(QStringLiteral("%1 (%2)").arg(m_name->text().trimmed(),
                                                                       color ? tr("color") : tr("special")));
    };
    connect(m_name, &QLineEdit::textChanged, this, updateListText);
    connect(m_kind, &QComboBox::currentIndexChanged, this, updateListText);
    connect(m_grid, &QTableWidget::itemChanged, this, &CatalogDialog::cellChanged);

    fillBasics();
    fillPatternList(m_catalog.patterns().isEmpty() ? -1 : 0);
}

// ---------------------------------------------------------------------------------------------------------------------
// Basic stitches

void CatalogDialog::fillBasics()
{
    m_basics->setRowCount(0);
    const QVector<StitchType> types = m_catalog.basics().types();
    m_basics->setRowCount(types.size());
    for (int r = 0; r < types.size(); ++r)
    {
        m_basics->setItem(r, 0, new QTableWidgetItem(types.at(r).name));
        for (int c = 0; c < 2; ++c)
        {
            auto *spin = new QDoubleSpinBox;
            spin->setRange(0.1, 100);
            spin->setDecimals(2);
            spin->setSuffix(tr(" mm"));
            spin->setValue(c == 0 ? types.at(r).size.stitchWidthMm : types.at(r).size.rowHeightMm);
            m_basics->setCellWidget(r, c + 1, spin);
        }
    }
}

void CatalogDialog::addBasic()
{
    const int r = m_basics->rowCount();
    m_basics->insertRow(r);
    m_basics->setItem(r, 0, new QTableWidgetItem(tr("New stitch %1").arg(r + 1)));
    for (int c = 0; c < 2; ++c)
    {
        auto *spin = new QDoubleSpinBox;
        spin->setRange(0.1, 100);
        spin->setDecimals(2);
        spin->setSuffix(tr(" mm"));
        spin->setValue(c == 0 ? 5.0 : 3.5);
        m_basics->setCellWidget(r, c + 1, spin);
    }
    m_basics->setCurrentCell(r, 0);
}

void CatalogDialog::removeBasic()
{
    const int r = m_basics->currentRow();
    if (r >= 0)
    {
        m_basics->removeRow(r);
    }
}

bool CatalogDialog::commitBasics(QString *error)
{
    StitchLibrary lib;
    QSet<QString> names;
    for (int r = 0; r < m_basics->rowCount(); ++r)
    {
        StitchType t;
        t.name = m_basics->item(r, 0)->text().trimmed();
        t.size.stitchWidthMm = qobject_cast<QDoubleSpinBox *>(m_basics->cellWidget(r, 1))->value();
        t.size.rowHeightMm = qobject_cast<QDoubleSpinBox *>(m_basics->cellWidget(r, 2))->value();
        if (t.name.isEmpty())
        {
            if (error) *error = tr("A basic stitch has no name.");
            return false;
        }
        if (names.contains(t.name))
        {
            if (error) *error = tr("Two basic stitches are named \"%1\".").arg(t.name);
            return false;
        }
        names.insert(t.name);
        lib.set(t);
    }
    m_catalog.basics() = lib;
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------
// Patterns

void CatalogDialog::fillPatternList(int select)
{
    m_loading = true;
    m_list->clear();
    for (const PatternDef &p : m_catalog.patterns())
    {
        m_list->addItem(QStringLiteral("%1 (%2)").arg(p.name, p.kind == PatternDef::Color ? tr("color") : tr("special")));
    }
    m_current = -1;
    if (select >= 0 && select < m_list->count())
    {
        m_list->setCurrentRow(select);      // still loading: no selection signal
    }
    m_loading = false;
    if (select >= 0 && select < m_list->count())
    {
        showPattern(select);
    }
    else
    {
        showPattern(-1);
    }
}

void CatalogDialog::showPattern(int index)
{
    m_loading = true;
    m_current = index;
    const bool have = index >= 0 && index < m_catalog.patterns().size();
    m_name->setEnabled(have);
    m_kind->setEnabled(have);
    m_width->setEnabled(have);
    m_height->setEnabled(have);
    m_grid->setEnabled(have);
    if (!have)
    {
        m_name->clear();
        m_grid->setRowCount(0);
        m_grid->setColumnCount(0);
        m_loading = false;
        return;
    }
    const PatternDef p = m_catalog.patterns().at(index);
    m_name->setText(p.name);
    m_kind->setCurrentIndex(m_kind->findData(int(p.kind)));
    m_width->setValue(p.width);
    m_height->setValue(p.height);
    m_grid->setColumnCount(p.width);
    m_grid->setRowCount(p.height);
    QStringList rowLabels;
    for (int i = 0; i < p.height; ++i)
    {
        rowLabels << tr("row %1").arg(p.height - i);
    }
    m_grid->setVerticalHeaderLabels(rowLabels);
    for (int i = 0; i < p.height; ++i)
    {
        const int y = p.height - 1 - i;             // the top row of the chart is the last row of the repeat
        for (int x = 0; x < p.width; ++x)
        {
            const int idx = p.cells.at(y * p.width + x);
            auto *item = new QTableWidgetItem(idx < 0 ? QString() : p.legend.at(idx));
            item->setTextAlignment(Qt::AlignCenter);
            m_grid->setItem(i, x, item);
        }
    }
    m_loading = false;
    recolorCells();
}

PatternDef CatalogDialog::patternFromEditor() const
{
    PatternDef p;
    p.name = m_name->text().trimmed();
    p.kind = static_cast<PatternDef::Kind>(m_kind->currentData().toInt());
    p.width = m_grid->columnCount();
    p.height = m_grid->rowCount();
    p.cells.fill(-1, p.width * p.height);
    for (int i = 0; i < p.height; ++i)
    {
        const int y = p.height - 1 - i;
        for (int x = 0; x < p.width; ++x)
        {
            const QTableWidgetItem *item = m_grid->item(i, x);
            const QString text = item ? item->text().trimmed() : QString();
            if (text.isEmpty())
            {
                continue;
            }
            int idx = p.legend.indexOf(text);
            if (idx < 0)
            {
                p.legend << text;
                idx = p.legend.size() - 1;
            }
            p.cells[y * p.width + x] = idx;
        }
    }
    return p;
}

bool CatalogDialog::commitPattern(QString *error)
{
    if (m_current < 0 || m_current >= m_catalog.patterns().size())
    {
        return true;
    }
    const PatternDef def = patternFromEditor();
    if (def.name.isEmpty())
    {
        if (error) *error = tr("The pattern has no name.");
        return false;
    }
    if (def.legend.isEmpty())
    {
        if (error) *error = tr("The pattern \"%1\" has no stitches or colors: fill in at least one cell.").arg(def.name);
        return false;
    }
    QVector<PatternDef> all = m_catalog.patterns();
    for (int i = 0; i < all.size(); ++i)
    {
        if (i != m_current && all.at(i).name == def.name)
        {
            if (error) *error = tr("Two patterns are named \"%1\".").arg(def.name);
            return false;
        }
    }
    if (!def.isValid())
    {
        if (error) *error = tr("The pattern \"%1\" is not valid.").arg(def.name);
        return false;
    }
    all[m_current] = def;
    for (const PatternDef &p : m_catalog.patterns())
    {
        m_catalog.remove(p.name);
    }
    for (const PatternDef &p : all)
    {
        m_catalog.set(p);          // same order as before
    }
    return true;
}

void CatalogDialog::newPattern()
{
    QString error;
    if (!commitPattern(&error))
    {
        QMessageBox::warning(this, windowTitle(), error);
        return;
    }
    PatternDef p;
    int n = m_catalog.patterns().size() + 1;
    do
    {
        p.name = tr("New pattern %1").arg(n++);
    } while (m_catalog.contains(p.name));
    p.kind = PatternDef::Special;
    p.width = 2;
    p.height = 2;
    p.legend = {QStringLiteral("k")};
    p.cells = {0, 0, 0, 0};
    m_catalog.set(p);
    fillPatternList(m_catalog.patterns().size() - 1);
}

void CatalogDialog::deletePattern()
{
    if (m_current < 0 || m_current >= m_catalog.patterns().size())
    {
        return;
    }
    m_catalog.remove(m_catalog.patterns().at(m_current).name);
    fillPatternList(qMin(m_current, m_catalog.patterns().size() - 1));
}

void CatalogDialog::patternSelected()
{
    if (m_loading)
    {
        return;
    }
    const int next = m_list->currentRow();
    QString error;
    if (!commitPattern(&error))
    {
        QMessageBox::warning(this, windowTitle(), error);
        m_loading = true;
        m_list->setCurrentRow(m_current);      // stay on the pattern that needs fixing
        m_loading = false;
        return;
    }
    // The commit keeps the order, so the index is still right; the names in the list may have changed.
    fillPatternList(next);
}

void CatalogDialog::gridSizeChanged()
{
    if (!m_loading)
    {
        resizeGrid(m_width->value(), m_height->value());
    }
}

void CatalogDialog::resizeGrid(int width, int height)
{
    // Keep what was typed; the bottom row and the left column stay where they are.
    const int oldW = m_grid->columnCount();
    const int oldH = m_grid->rowCount();
    QVector<QString> old(oldW * oldH);
    for (int i = 0; i < oldH; ++i)
    {
        for (int x = 0; x < oldW; ++x)
        {
            const QTableWidgetItem *item = m_grid->item(i, x);
            old[(oldH - 1 - i) * oldW + x] = item ? item->text() : QString();
        }
    }
    m_loading = true;
    m_grid->setColumnCount(width);
    m_grid->setRowCount(height);
    QStringList rowLabels;
    for (int i = 0; i < height; ++i)
    {
        rowLabels << tr("row %1").arg(height - i);
        const int y = height - 1 - i;
        for (int x = 0; x < width; ++x)
        {
            const QString text = (x < oldW && y < oldH) ? old.at(y * oldW + x) : QString();
            auto *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            m_grid->setItem(i, x, item);
        }
    }
    m_grid->setVerticalHeaderLabels(rowLabels);
    m_loading = false;
    recolorCells();
}

void CatalogDialog::cellChanged()
{
    if (!m_loading)
    {
        recolorCells();
    }
}

void CatalogDialog::recolorCells()
{
    const bool color = m_kind->currentData().toInt() == int(PatternDef::Color);
    const bool was = m_loading;
    m_loading = true;
    for (int i = 0; i < m_grid->rowCount(); ++i)
    {
        for (int x = 0; x < m_grid->columnCount(); ++x)
        {
            QTableWidgetItem *item = m_grid->item(i, x);
            if (!item) continue;
            const QString text = item->text().trimmed();
            if (color && !text.isEmpty())
            {
                const QColor c = yarnColor(text);
                item->setBackground(c);
                item->setForeground(c.lightness() < 128 ? Qt::white : Qt::black);
            }
            else
            {
                item->setData(Qt::BackgroundRole, QVariant());
                item->setData(Qt::ForegroundRole, QVariant());
            }
        }
    }
    m_loading = was;
}

// ---------------------------------------------------------------------------------------------------------------------
// Import and export

bool CatalogDialog::importFile(const QString &path, ConflictPolicy policy, ImportResult *result, QString *error)
{
    if (!commitPattern(error))
    {
        return false;
    }
    QVector<PatternDef> patterns;
    if (!importPatterns(path, patterns, nullptr, error))
    {
        return false;
    }
    const ImportResult res = addToCatalog(m_catalog, patterns, policy);
    if (result) *result = res;
    fillPatternList(m_catalog.patterns().isEmpty() ? -1 : qMax(0, qMin(m_current, m_catalog.patterns().size() - 1)));
    return true;
}

bool CatalogDialog::exportPatterns(const QString &path, const QStringList &names, QString *error)
{
    if (!commitPattern(error))
    {
        return false;
    }
    QVector<PatternDef> out;
    for (const QString &name : names)
    {
        if (!m_catalog.contains(name))
        {
            if (error) *error = tr("No pattern named \"%1\".").arg(name);
            return false;
        }
        out.append(m_catalog.find(name));
    }
    return stitchcore::exportPatterns(path, out, PatternFileInfo(), error);
}

void CatalogDialog::importClicked()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Import patterns"), QString(),
                                                      tr("Stitchly2D patterns (*.stpat);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }
    // Look first, so the question about name clashes is only asked when there are clashes.
    QVector<PatternDef> patterns;
    QString error;
    if (!importPatterns(path, patterns, nullptr, &error))
    {
        QMessageBox::warning(this, tr("Import patterns"), error);
        return;
    }
    ConflictPolicy policy = ConflictPolicy::Rename;
    bool clash = false;
    for (const PatternDef &p : patterns) clash = clash || m_catalog.contains(p.name);
    if (clash)
    {
        QMessageBox box(QMessageBox::Question, tr("Import patterns"),
                        tr("Some patterns have the same name as patterns in the catalog. What should happen to them?"),
                        QMessageBox::NoButton, this);
        QPushButton *rename = box.addButton(tr("Keep both (rename)"), QMessageBox::AcceptRole);
        QPushButton *replace = box.addButton(tr("Replace"), QMessageBox::DestructiveRole);
        QPushButton *skip = box.addButton(tr("Skip them"), QMessageBox::RejectRole);
        box.addButton(QMessageBox::Cancel);
        box.exec();
        if (box.clickedButton() == replace) policy = ConflictPolicy::Replace;
        else if (box.clickedButton() == skip) policy = ConflictPolicy::Skip;
        else if (box.clickedButton() != rename) return;
    }
    ImportResult res;
    if (!importFile(path, policy, &res, &error))
    {
        QMessageBox::warning(this, tr("Import patterns"), error);
        return;
    }
    QMessageBox::information(this, tr("Import patterns"),
                             tr("%1 added (%2 renamed), %3 replaced, %4 skipped.")
                                 .arg(res.added).arg(res.renamed).arg(res.replaced).arg(res.skipped));
}

void CatalogDialog::exportSelected()
{
    if (m_current < 0 || m_current >= m_catalog.patterns().size())
    {
        return;
    }
    QString error;
    if (!commitPattern(&error))
    {
        QMessageBox::warning(this, windowTitle(), error);
        return;
    }
    const QString name = m_catalog.patterns().at(m_current).name;
    const QString path = QFileDialog::getSaveFileName(this, tr("Export pattern"), name + QStringLiteral(".stpat"),
                                                      tr("Stitchly2D patterns (*.stpat)"));
    if (!path.isEmpty() && !exportPatterns(path, {name}, &error))
    {
        QMessageBox::warning(this, tr("Export pattern"), error);
    }
}

void CatalogDialog::exportAll()
{
    QStringList names;
    QString error;
    if (!commitPattern(&error))
    {
        QMessageBox::warning(this, windowTitle(), error);
        return;
    }
    for (const PatternDef &p : m_catalog.patterns()) names << p.name;
    if (names.isEmpty())
    {
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("Export all patterns"),
                                                      QStringLiteral("patterns.stpat"),
                                                      tr("Stitchly2D patterns (*.stpat)"));
    if (!path.isEmpty() && !exportPatterns(path, names, &error))
    {
        QMessageBox::warning(this, tr("Export patterns"), error);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool CatalogDialog::apply(QString *error)
{
    return commitBasics(error) && commitPattern(error);
}

void CatalogDialog::accept()
{
    QString error;
    if (!apply(&error))
    {
        QMessageBox::warning(this, windowTitle(), error);
        return;
    }
    QDialog::accept();
}
