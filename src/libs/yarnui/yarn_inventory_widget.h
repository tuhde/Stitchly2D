// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef YARNUI_YARN_INVENTORY_WIDGET_H
#define YARNUI_YARN_INVENTORY_WIDGET_H

#include <QDialog>
#include <QStringList>
#include <QWidget>

#include "yarn.h"

class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTableWidget;

namespace yarnui
{

/**
 * Editor for the yarn inventory: the list of yarns on the left, the selected yarn on the right (what it is, how much
 * is left, and what it knits to with each basic stitch). Edits go into the inventory at once; the owner saves it.
 */
class YarnInventoryWidget : public QWidget
{
    Q_OBJECT
public:
    /** stitchTypes: the names of the basic stitches a gauge can be entered for. */
    YarnInventoryWidget(const stitchcore::YarnInventory &inventory, const QStringList &stitchTypes,
                        QWidget *parent = nullptr);

    const stitchcore::YarnInventory &inventory() const { return m_inventory; }
    void selectYarn(const QString &id);
    /** False, with a message, if a yarn has neither a brand nor a name. */
    bool validate(QString *error = nullptr) const;

signals:
    void changed();

private slots:
    void addYarn();
    void duplicateYarn();
    void removeYarn();
    void yarnSelected();
    void commit();
    void pickColor();
    void browseImage();
    void addGauge();
    void removeGauge();

private:
    void fillList(const QString &select);
    void showYarn(const stitchcore::Yarn *yarn);
    void fillGaugeRow(int row, const stitchcore::YarnGauge &gauge);
    QString currentId() const;

    stitchcore::YarnInventory m_inventory;
    QStringList m_stitchTypes;
    bool        m_loading = false;

    QListWidget    *m_list = nullptr;
    QPushButton    *m_duplicate = nullptr;
    QPushButton    *m_remove = nullptr;
    QWidget        *m_form = nullptr;
    QLineEdit      *m_brand = nullptr;
    QLineEdit      *m_name = nullptr;
    QLineEdit      *m_fiber = nullptr;
    QLineEdit      *m_weight = nullptr;
    QLineEdit      *m_colorName = nullptr;
    QLineEdit      *m_colorHex = nullptr;
    QPushButton    *m_colorButton = nullptr;
    QLineEdit      *m_image = nullptr;
    QLabel         *m_preview = nullptr;
    QLineEdit      *m_dyeLot = nullptr;
    QPlainTextEdit *m_notes = nullptr;
    QDoubleSpinBox *m_meters = nullptr;
    QDoubleSpinBox *m_grams = nullptr;
    QSpinBox       *m_skeins = nullptr;
    QDoubleSpinBox *m_partial = nullptr;
    QLabel         *m_stock = nullptr;
    QTableWidget   *m_gauges = nullptr;
};

/** The inventory editor in a dialog with OK and Cancel. */
class YarnInventoryDialog : public QDialog
{
    Q_OBJECT
public:
    YarnInventoryDialog(const stitchcore::YarnInventory &inventory, const QStringList &stitchTypes,
                        QWidget *parent = nullptr);
    const stitchcore::YarnInventory &inventory() const { return m_widget->inventory(); }
    YarnInventoryWidget *widget() const { return m_widget; }

public slots:
    void accept() override;

private:
    YarnInventoryWidget *m_widget = nullptr;
};

}

#endif
