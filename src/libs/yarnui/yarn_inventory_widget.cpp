#include "yarn_inventory_widget.h"

#include <QColorDialog>
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
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSpinBox>
#include <QSplitter>
#include <QTableWidget>
#include <QVBoxLayout>

#include "yarn_picker.h"

using namespace stitchcore;

namespace yarnui
{

namespace
{
QDoubleSpinBox *spin(const QString &objectName, qreal max, int decimals, const QString &suffix = QString())
{
    auto *s = new QDoubleSpinBox;
    s->setObjectName(objectName);
    s->setRange(0, max);
    s->setDecimals(decimals);
    s->setSuffix(suffix);
    return s;
}
}

YarnInventoryWidget::YarnInventoryWidget(const YarnInventory &inventory, const QStringList &stitchTypes, QWidget *parent)
    : QWidget(parent), m_inventory(inventory), m_stitchTypes(stitchTypes)
{
    auto *outer = new QHBoxLayout(this);
    auto *split = new QSplitter;
    outer->addWidget(split);

    // The list
    auto *left = new QWidget;
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    m_list = new QListWidget;
    m_list->setObjectName(QStringLiteral("yarnList"));
    leftLayout->addWidget(m_list, 1);
    auto *buttons = new QHBoxLayout;
    auto *add = new QPushButton(tr("Add"));
    add->setObjectName(QStringLiteral("addYarnButton"));
    m_duplicate = new QPushButton(tr("Duplicate"));
    m_duplicate->setObjectName(QStringLiteral("duplicateYarnButton"));
    m_remove = new QPushButton(tr("Remove"));
    m_remove->setObjectName(QStringLiteral("removeYarnButton"));
    buttons->addWidget(add);
    buttons->addWidget(m_duplicate);
    buttons->addWidget(m_remove);
    leftLayout->addLayout(buttons);
    split->addWidget(left);

    // The selected yarn
    m_form = new QWidget;
    auto *rightLayout = new QVBoxLayout(m_form);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    auto *form = new QFormLayout;
    rightLayout->addLayout(form);

    auto line = [&](const QString &objectName) {
        auto *e = new QLineEdit;
        e->setObjectName(objectName);
        return e;
    };
    m_brand = line(QStringLiteral("yarnBrand"));
    m_name = line(QStringLiteral("yarnName"));
    m_fiber = line(QStringLiteral("yarnFiber"));
    m_fiber->setPlaceholderText(tr("e.g. merino 80%, nylon 20%"));
    m_weight = line(QStringLiteral("yarnWeight"));
    m_weight->setPlaceholderText(tr("e.g. fingering, DK, aran"));
    m_colorName = line(QStringLiteral("yarnColorName"));
    m_colorHex = line(QStringLiteral("yarnColorHex"));
    m_colorHex->setPlaceholderText(QStringLiteral("#rrggbb"));
    m_colorHex->setMaxLength(7);
    m_colorHex->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^(#[0-9a-fA-F]{0,6})?$")), m_colorHex));
    m_colorButton = new QPushButton(tr("Pick..."));
    m_colorButton->setObjectName(QStringLiteral("yarnPickColor"));
    m_image = line(QStringLiteral("yarnImage"));
    auto *browse = new QPushButton(tr("Browse..."));
    browse->setObjectName(QStringLiteral("yarnBrowseImage"));
    m_preview = new QLabel;
    m_preview->setFixedHeight(64);
    m_dyeLot = line(QStringLiteral("yarnDyeLot"));
    m_notes = new QPlainTextEdit;
    m_notes->setObjectName(QStringLiteral("yarnNotes"));
    m_notes->setFixedHeight(60);
    m_meters = spin(QStringLiteral("yarnMeters"), 100000, 0, tr(" m"));
    m_grams = spin(QStringLiteral("yarnGrams"), 100000, 0, tr(" g"));
    m_skeins = new QSpinBox;
    m_skeins->setObjectName(QStringLiteral("yarnSkeins"));
    m_skeins->setRange(0, 10000);
    m_partial = spin(QStringLiteral("yarnPartial"), 100000, 0, tr(" m"));
    m_stock = new QLabel;

    form->addRow(tr("Brand"), m_brand);
    form->addRow(tr("Name"), m_name);
    form->addRow(tr("Fiber"), m_fiber);
    form->addRow(tr("Weight"), m_weight);
    form->addRow(tr("Color"), m_colorName);
    auto *hexRow = new QHBoxLayout;
    hexRow->addWidget(m_colorHex, 1);
    hexRow->addWidget(m_colorButton);
    form->addRow(tr("Color value"), hexRow);
    auto *imageRow = new QHBoxLayout;
    imageRow->addWidget(m_image, 1);
    imageRow->addWidget(browse);
    form->addRow(tr("Photo"), imageRow);
    form->addRow(QString(), m_preview);
    form->addRow(tr("Dye lot"), m_dyeLot);
    form->addRow(tr("Length per skein"), m_meters);
    form->addRow(tr("Weight per skein"), m_grams);
    form->addRow(tr("Skeins in stock"), m_skeins);
    form->addRow(tr("Left on an opened skein"), m_partial);
    form->addRow(tr("In stock"), m_stock);
    form->addRow(tr("Notes"), m_notes);

    rightLayout->addWidget(new QLabel(tr("What this yarn knits to with a basic stitch (swatch, per 10 cm). "
                                         "Yarn per stitch is optional: measure it for a better estimate.")));
    m_gauges = new QTableWidget(0, 4);
    m_gauges->setObjectName(QStringLiteral("yarnGauges"));
    m_gauges->setHorizontalHeaderLabels({tr("Stitch"), tr("Stitches"), tr("Rows"), tr("Yarn per stitch")});
    m_gauges->horizontalHeader()->setStretchLastSection(true);
    m_gauges->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_gauges->setSelectionMode(QAbstractItemView::SingleSelection);
    rightLayout->addWidget(m_gauges, 1);
    auto *gaugeButtons = new QHBoxLayout;
    auto *addGaugeButton = new QPushButton(tr("Add gauge"));
    addGaugeButton->setObjectName(QStringLiteral("addGaugeButton"));
    auto *removeGaugeButton = new QPushButton(tr("Remove gauge"));
    removeGaugeButton->setObjectName(QStringLiteral("removeGaugeButton"));
    gaugeButtons->addWidget(addGaugeButton);
    gaugeButtons->addWidget(removeGaugeButton);
    rightLayout->addLayout(gaugeButtons);
    split->addWidget(m_form);
    split->setStretchFactor(1, 1);

    connect(add, &QPushButton::clicked, this, &YarnInventoryWidget::addYarn);
    connect(m_duplicate, &QPushButton::clicked, this, &YarnInventoryWidget::duplicateYarn);
    connect(m_remove, &QPushButton::clicked, this, &YarnInventoryWidget::removeYarn);
    connect(m_list, &QListWidget::currentRowChanged, this, &YarnInventoryWidget::yarnSelected);
    connect(m_colorButton, &QPushButton::clicked, this, &YarnInventoryWidget::pickColor);
    connect(browse, &QPushButton::clicked, this, &YarnInventoryWidget::browseImage);
    connect(addGaugeButton, &QPushButton::clicked, this, &YarnInventoryWidget::addGauge);
    connect(removeGaugeButton, &QPushButton::clicked, this, &YarnInventoryWidget::removeGauge);
    for (QLineEdit *e : {m_brand, m_name, m_fiber, m_weight, m_colorName, m_colorHex, m_image, m_dyeLot})
    {
        connect(e, &QLineEdit::textEdited, this, &YarnInventoryWidget::commit);
    }
    connect(m_notes, &QPlainTextEdit::textChanged, this, &YarnInventoryWidget::commit);
    for (QDoubleSpinBox *s : {m_meters, m_grams, m_partial})
    {
        connect(s, &QDoubleSpinBox::valueChanged, this, &YarnInventoryWidget::commit);
    }
    connect(m_skeins, &QSpinBox::valueChanged, this, &YarnInventoryWidget::commit);

    fillList(QString());
}

QString YarnInventoryWidget::currentId() const
{
    const QListWidgetItem *item = m_list->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void YarnInventoryWidget::fillList(const QString &select)
{
    const bool was = m_loading;
    m_loading = true;
    m_list->clear();
    for (const Yarn &y : m_inventory.yarns())
    {
        QString text = y.displayName();
        if (text.isEmpty()) text = tr("(unnamed)");
        auto *item = new QListWidgetItem(yarnColorIcon(y.colorHex), text);
        item->setData(Qt::UserRole, y.id);
        m_list->addItem(item);
    }
    m_loading = was;
    int row = -1;
    for (int i = 0; i < m_list->count(); ++i)
    {
        if (m_list->item(i)->data(Qt::UserRole).toString() == select) row = i;
    }
    m_list->setCurrentRow(row >= 0 ? row : (m_list->count() > 0 ? 0 : -1));
    yarnSelected();          // also when the row did not change
}

void YarnInventoryWidget::selectYarn(const QString &id)
{
    for (int i = 0; i < m_list->count(); ++i)
    {
        if (m_list->item(i)->data(Qt::UserRole).toString() == id)
        {
            m_list->setCurrentRow(i);
            return;
        }
    }
}

void YarnInventoryWidget::yarnSelected()
{
    const Yarn *y = m_inventory.find(currentId());
    showYarn(y);
}

void YarnInventoryWidget::showYarn(const Yarn *yarn)
{
    m_loading = true;
    m_form->setEnabled(yarn != nullptr);
    m_duplicate->setEnabled(yarn != nullptr);
    m_remove->setEnabled(yarn != nullptr);
    const Yarn y = yarn ? *yarn : Yarn();
    m_brand->setText(y.brand);
    m_name->setText(y.name);
    m_fiber->setText(y.fiber);
    m_weight->setText(y.weightClass);
    m_colorName->setText(y.colorName);
    m_colorHex->setText(y.colorHex);
    m_image->setText(y.imagePath);
    const QPixmap pix(y.imagePath);
    m_preview->setPixmap(pix.isNull() ? QPixmap() : pix.scaledToHeight(64, Qt::SmoothTransformation));
    m_dyeLot->setText(y.dyeLot);
    m_notes->setPlainText(y.notes);
    m_meters->setValue(y.metersPerSkein);
    m_grams->setValue(y.gramsPerSkein);
    m_skeins->setValue(y.skeinsInStock);
    m_partial->setValue(y.partialMeters);
    m_stock->setText(tr("%1 m").arg(y.stockMeters(), 0, 'f', 0));
    m_gauges->setRowCount(0);
    m_gauges->setRowCount(y.gauges.size());
    for (int r = 0; r < y.gauges.size(); ++r)
    {
        fillGaugeRow(r, y.gauges.at(r));
    }
    m_loading = false;
}

void YarnInventoryWidget::fillGaugeRow(int row, const YarnGauge &g)
{
    auto *type = new QComboBox;
    type->addItems(m_stitchTypes);
    if (type->findText(g.stitchType) < 0) type->addItem(g.stitchType);
    type->setCurrentText(g.stitchType);
    auto *stitches = spin(QStringLiteral("gaugeStitches"), 200, 1);
    stitches->setMinimum(0.1);
    stitches->setValue(g.size.stitchesPer10cm());
    auto *rows = spin(QStringLiteral("gaugeRows"), 400, 1);
    rows->setMinimum(0.1);
    rows->setValue(g.size.rowsPer10cm());
    auto *yarnPer = spin(QStringLiteral("gaugeYarn"), 500, 1, tr(" mm"));
    yarnPer->setSpecialValueText(tr("estimate"));
    yarnPer->setValue(g.yarnPerStitchMm);
    m_gauges->setCellWidget(row, 0, type);
    m_gauges->setCellWidget(row, 1, stitches);
    m_gauges->setCellWidget(row, 2, rows);
    m_gauges->setCellWidget(row, 3, yarnPer);
    connect(type, &QComboBox::currentIndexChanged, this, &YarnInventoryWidget::commit);
    connect(stitches, &QDoubleSpinBox::valueChanged, this, &YarnInventoryWidget::commit);
    connect(rows, &QDoubleSpinBox::valueChanged, this, &YarnInventoryWidget::commit);
    connect(yarnPer, &QDoubleSpinBox::valueChanged, this, &YarnInventoryWidget::commit);
}

void YarnInventoryWidget::commit()
{
    if (m_loading)
    {
        return;
    }
    Yarn *y = m_inventory.find(currentId());
    if (!y)
    {
        return;
    }
    y->brand = m_brand->text().trimmed();
    y->name = m_name->text().trimmed();
    y->fiber = m_fiber->text();
    y->weightClass = m_weight->text();
    y->colorName = m_colorName->text();
    const QString hex = m_colorHex->text();
    if (hex.isEmpty() || hex.size() == 7) y->colorHex = hex;       // while typing, the old value stays
    y->imagePath = m_image->text();
    y->dyeLot = m_dyeLot->text();
    y->notes = m_notes->toPlainText();
    y->metersPerSkein = m_meters->value();
    y->gramsPerSkein = m_grams->value();
    y->skeinsInStock = m_skeins->value();
    y->partialMeters = m_partial->value();
    y->gauges.clear();
    for (int r = 0; r < m_gauges->rowCount(); ++r)
    {
        const auto *type = qobject_cast<QComboBox *>(m_gauges->cellWidget(r, 0));
        const auto *st = qobject_cast<QDoubleSpinBox *>(m_gauges->cellWidget(r, 1));
        const auto *ro = qobject_cast<QDoubleSpinBox *>(m_gauges->cellWidget(r, 2));
        const auto *yp = qobject_cast<QDoubleSpinBox *>(m_gauges->cellWidget(r, 3));
        if (!type || !st || !ro || !yp || y->gaugeFor(type->currentText())) continue;   // one gauge per stitch
        y->gauges.append(YarnGauge{type->currentText(), StitchSize::fromPer10cm(st->value(), ro->value()), yp->value()});
    }
    m_stock->setText(tr("%1 m").arg(y->stockMeters(), 0, 'f', 0));
    const QPixmap pix(y->imagePath);
    m_preview->setPixmap(pix.isNull() ? QPixmap() : pix.scaledToHeight(64, Qt::SmoothTransformation));
    if (QListWidgetItem *item = m_list->currentItem())
    {
        item->setText(y->displayName().isEmpty() ? tr("(unnamed)") : y->displayName());
        item->setIcon(yarnColorIcon(y->colorHex));
    }
    emit changed();
}

void YarnInventoryWidget::addYarn()
{
    Yarn y;
    y.name = tr("New yarn");
    const QString id = m_inventory.set(y);
    fillList(id);
    m_name->setFocus();
    m_name->selectAll();
    emit changed();
}

void YarnInventoryWidget::duplicateYarn()
{
    const Yarn *src = m_inventory.find(currentId());
    if (!src)
    {
        return;
    }
    Yarn copy = *src;
    copy.id.clear();
    copy.name = tr("%1 (copy)").arg(copy.name);
    const QString id = m_inventory.set(copy);
    fillList(id);
    emit changed();
}

void YarnInventoryWidget::removeYarn()
{
    const Yarn *y = m_inventory.find(currentId());
    if (!y)
    {
        return;
    }
    if (QMessageBox::question(this, tr("Remove yarn"), tr("Remove %1 from the inventory?").arg(y->displayName()))
        != QMessageBox::Yes)
    {
        return;
    }
    const int row = m_list->currentRow();
    m_inventory.remove(y->id);
    fillList(QString());
    m_list->setCurrentRow(qMin(row, m_list->count() - 1));
    emit changed();
}

void YarnInventoryWidget::pickColor()
{
    const QColor c = QColorDialog::getColor(QColor(m_colorHex->text()), this, tr("Yarn color"));
    if (c.isValid())
    {
        m_colorHex->setText(c.name());
        commit();
    }
}

void YarnInventoryWidget::browseImage()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Photo of the yarn"), m_image->text(),
                                                      tr("Images (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (!file.isEmpty())
    {
        m_image->setText(file);
        commit();
    }
}

void YarnInventoryWidget::addGauge()
{
    Yarn *y = m_inventory.find(currentId());
    if (!y)
    {
        return;
    }
    QString type;
    for (const QString &t : m_stitchTypes)
    {
        if (!y->gaugeFor(t)) { type = t; break; }
    }
    if (type.isEmpty())
    {
        return;                         // every stitch has its gauge
    }
    StitchSize size = StitchSize::fromPer10cm(22, 30);
    if (!y->gauges.isEmpty()) size = y->gauges.last().size;
    y->gauges.append(YarnGauge{type, size, 0});
    showYarn(y);
    emit changed();
}

void YarnInventoryWidget::removeGauge()
{
    Yarn *y = m_inventory.find(currentId());
    const int row = m_gauges->currentRow();
    if (!y || row < 0 || row >= y->gauges.size())
    {
        return;
    }
    y->gauges.removeAt(row);
    showYarn(y);
    emit changed();
}

bool YarnInventoryWidget::validate(QString *error) const
{
    for (const Yarn &y : m_inventory.yarns())
    {
        if (y.brand.isEmpty() && y.name.isEmpty())
        {
            if (error) *error = tr("Every yarn needs a brand or a name.");
            return false;
        }
    }
    return true;
}

YarnInventoryDialog::YarnInventoryDialog(const YarnInventory &inventory, const QStringList &stitchTypes, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Yarns"));
    resize(900, 640);
    auto *layout = new QVBoxLayout(this);
    m_widget = new YarnInventoryWidget(inventory, stitchTypes);
    layout->addWidget(m_widget, 1);
    auto *box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(box, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(box);
}

void YarnInventoryDialog::accept()
{
    QString error;
    if (!m_widget->validate(&error))
    {
        QMessageBox::warning(this, tr("Yarns"), error);
        return;
    }
    QDialog::accept();
}

}
