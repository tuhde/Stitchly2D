// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef YARN_DIALOG_H
#define YARN_DIALOG_H

#include <QDialog>
#include <QMap>
#include <QStringList>
#include <QVector>

#include "../../libs/stitchcore/yarn.h"
#include "../../libs/stitchcore/yarn_estimate.h"

class QListWidget;

/**
 * Touch-friendly yarn choice for a piece: one big list per yarn slot ("main", or the color of a color pattern) with
 * the yarns of the inventory, their stock and what the piece needs of them. With a single slot a tap chooses and closes.
 * "Without yarn" rejects the dialog.
 */
class YarnDialog : public QDialog
{
    Q_OBJECT
public:
    /** preselected: yarn id by slot (the choice of last time). */
    YarnDialog(const QString &pieceName, const QStringList &slotNames, const stitchcore::YarnInventory &inventory,
               const QVector<stitchcore::YarnUse> &uses, const QMap<QString, QString> &preselected,
               QWidget *parent = nullptr);

    /** Yarn id by slot; slots left on "no yarn" are not in the map. */
    QMap<QString, QString> choice() const;

private:
    QStringList               m_slots;
    QVector<QListWidget *>    m_lists;
};

#endif
