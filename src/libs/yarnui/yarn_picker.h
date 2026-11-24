// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef YARNUI_YARN_PICKER_H
#define YARNUI_YARN_PICKER_H

#include <QIcon>
#include <QString>

#include "yarn.h"

class QComboBox;

namespace yarnui
{

/** Small round icon in the color of a yarn (grey if the yarn has none). */
QIcon yarnColorIcon(const QString &colorHex);

/**
 * Fill a combo box with "no yarn" (data: empty string) and the yarns of the inventory (data: their ids).
 * A yarn that is not in the inventory (a project from another computer) is added with its stored name, marked as missing.
 * The yarn of current is selected.
 */
void fillYarnCombo(QComboBox *combo, const stitchcore::YarnInventory &inventory, const QString &currentId,
                   const QString &currentName = QString(), const QString &currentColor = QString());

/** Id of the yarn the combo shows; empty for "no yarn". */
QString selectedYarnId(const QComboBox *combo);

}

#endif
