#include "yarn_picker.h"

#include <QColor>
#include <QCoreApplication>
#include <QComboBox>
#include <QPainter>
#include <QPixmap>

namespace yarnui
{

QIcon yarnColorIcon(const QString &colorHex)
{
    QPixmap pix(16, 16);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    const QColor color(colorHex);
    p.setBrush(color.isValid() ? color : QColor(Qt::lightGray));
    p.setPen(QColor(0, 0, 0, 120));
    p.drawEllipse(QRectF(1.5, 1.5, 13, 13));
    return QIcon(pix);
}

void fillYarnCombo(QComboBox *combo, const stitchcore::YarnInventory &inventory, const QString &currentId,
                   const QString &currentName, const QString &currentColor)
{
    const bool blocked = combo->blockSignals(true);
    combo->clear();
    combo->addItem(QCoreApplication::translate("yarnui", "(no yarn)"), QString());
    for (const stitchcore::Yarn &y : inventory.yarns())
    {
        combo->addItem(yarnColorIcon(y.colorHex), y.displayName(), y.id);
    }
    int index = 0;
    if (!currentId.isEmpty())
    {
        index = combo->findData(currentId);
        if (index < 0)
        {
            combo->addItem(yarnColorIcon(currentColor), QCoreApplication::translate("yarnui", "%1 (not in your inventory)").arg(currentName.isEmpty() ? currentId : currentName),
                           currentId);
            index = combo->count() - 1;
        }
    }
    combo->setCurrentIndex(index);
    combo->blockSignals(blocked);
}

QString selectedYarnId(const QComboBox *combo)
{
    return combo->currentData().toString();
}

}
