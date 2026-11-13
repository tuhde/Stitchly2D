// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef CATALOG_DIALOG_H
#define CATALOG_DIALOG_H

#include <QDialog>

#include "../../libs/stitchcore/catalog.h"
#include "../../libs/stitchcore/patternfile.h"

class QComboBox;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QTableWidget;

/**
 * Editor for the catalog: basic stitch types (name, stitch width, row height) and special / color patterns.
 *
 * A pattern is edited as a chart: every cell holds the stitch symbol (special pattern) or the yarn name (color
 * pattern), an empty cell leaves the base stitch alone. The legend is built from what is typed in the cells.
 * The top row of the chart is the top row of the repeat, as in a knitting chart. Patterns can be imported from and
 * exported to .stpat files.
 */
class CatalogDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CatalogDialog(const stitchcore::StitchCatalog &catalog, QWidget *parent = nullptr);

    /** The edited catalog; only complete after apply() or accept(). */
    stitchcore::StitchCatalog catalog() const { return m_catalog; }

    /** Take over the editors. False, with a message, when something is not valid (empty or duplicate names, ...). */
    bool apply(QString *error = nullptr);

    bool importFile(const QString &path, stitchcore::ConflictPolicy policy, stitchcore::ImportResult *result,
                    QString *error);
    bool exportPatterns(const QString &path, const QStringList &names, QString *error);

public slots:
    void accept() override;

private slots:
    void addBasic();
    void removeBasic();
    void newPattern();
    void deletePattern();
    void patternSelected();
    void gridSizeChanged();
    void cellChanged();
    void importClicked();
    void exportSelected();
    void exportAll();

private:
    void fillBasics();
    void fillPatternList(int select);
    void showPattern(int index);
    void resizeGrid(int width, int height);
    void recolorCells();
    bool commitBasics(QString *error);
    bool commitPattern(QString *error);
    stitchcore::PatternDef patternFromEditor() const;

    stitchcore::StitchCatalog m_catalog;
    int  m_current = -1;           //!< index in m_catalog.patterns() that the editor shows
    bool m_loading = false;

    QTableWidget *m_basics = nullptr;
    QListWidget  *m_list = nullptr;
    QLineEdit    *m_name = nullptr;
    QComboBox    *m_kind = nullptr;
    QSpinBox     *m_width = nullptr;
    QSpinBox     *m_height = nullptr;
    QTableWidget *m_grid = nullptr;
};

#endif
