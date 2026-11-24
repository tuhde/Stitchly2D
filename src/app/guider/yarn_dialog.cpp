#include "yarn_dialog.h"

#include <QBrush>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "../../libs/yarnui/yarn_picker.h"
#include "guide_scale.h"

using namespace stitchcore;

YarnDialog::YarnDialog(const QString &pieceName, const QStringList &slotNames, const YarnInventory &inventory,
                       const QVector<YarnUse> &uses, const QMap<QString, QString> &preselected, QWidget *parent)
    : QDialog(parent), m_slots(slotNames)
{
    setWindowTitle(tr("Yarn"));
    const int u = guideUnit(this);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Which yarn do you knit %1 with?").arg(pieceName), this));

    for (const QString &slot : slotNames)
    {
        if (slotNames.size() > 1 || slot != QLatin1String(kMainYarnSlot))
        {
            layout->addWidget(new QLabel(slot == QLatin1String(kMainYarnSlot) ? tr("Main yarn") : tr("Yarn %1").arg(slot), this));
        }
        auto *list = new QListWidget(this);
        list->setObjectName(QStringLiteral("yarnList_%1").arg(slot));
        list->setTextElideMode(Qt::ElideRight);
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        auto *none = new QListWidgetItem(tr("(no yarn)"), list);
        none->setData(Qt::UserRole, QString());
        int select = 0;
        for (const Yarn &y : inventory.yarns())
        {
            const qreal needed = estimateYarnMeters(uses, {{slot, y}}).value(slot);
            QString text = y.displayName();
            if (!y.dyeLot.isEmpty()) text += tr("  ·  lot %1").arg(y.dyeLot);
            text += tr("\n%1 m in stock").arg(y.stockMeters(), 0, 'f', 0);
            const bool lacking = needed > 0 && y.stockMeters() < needed;
            if (needed > 0) text += tr(", about %1 m needed").arg(needed, 0, 'f', 0);
            if (lacking) text += tr(" (not enough)");
            auto *item = new QListWidgetItem(yarnui::yarnColorIcon(y.colorHex), text, list);
            item->setData(Qt::UserRole, y.id);
            if (lacking) item->setForeground(QBrush(QColor(0xc0, 0x39, 0x2b)));
            if (y.id == preselected.value(slot)) select = list->count() - 1;
        }
        list->setIconSize(QSize(u * 2, u * 2));
        list->setCurrentRow(select);
        list->setMinimumHeight(u * 8);
        layout->addWidget(list, 1);
        m_lists << list;
        if (slotNames.size() == 1)
        {
            connect(list, &QListWidget::itemClicked, this, &QDialog::accept);
        }
    }

    auto *buttons = new QHBoxLayout;
    auto *without = new QPushButton(tr("Without yarn"), this);
    auto *start = new QPushButton(tr("Start"), this);
    for (QPushButton *b : {without, start}) b->setMinimumHeight(u * 4);
    start->setDefault(true);
    buttons->addWidget(without);
    buttons->addStretch(1);
    buttons->addWidget(start);
    layout->addLayout(buttons);
    connect(without, &QPushButton::clicked, this, &QDialog::reject);
    connect(start, &QPushButton::clicked, this, &QDialog::accept);
    resize(u * 40, u * (8 * qMax<int>(1, slotNames.size()) + 12));
}

QMap<QString, QString> YarnDialog::choice() const
{
    QMap<QString, QString> chosen;
    for (int i = 0; i < m_lists.size(); ++i)
    {
        const QListWidgetItem *item = m_lists.at(i)->currentItem();
        const QString id = item ? item->data(Qt::UserRole).toString() : QString();
        if (!id.isEmpty()) chosen.insert(m_slots.at(i), id);
    }
    return chosen;
}
