// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef GUIDE_WINDOW_H
#define GUIDE_WINDOW_H

#include <functional>

#include <QHash>
#include <QMap>
#include <QSet>
#include <QWidget>

#include "row_counter.h"

#include "../../libs/stitchload/core/knit_job.h"
#include "../../libs/stitchcore/yarn.h"

class QLabel;
class QScreen;
class QPushButton;
class StepView;

/**
 * The guide: piece and row at the top, the rolling instructions in the middle, big buttons at the bottom.
 * Row 0 is "before the first row" (cast on), rowCount + 1 is "after the last row" (bind off).
 */
class GuideWindow : public QWidget
{
    Q_OBJECT
public:
    explicit GuideWindow(QWidget *parent = nullptr);

    /** Loads the file and shows its first piece (or the piece and row last used with this file). */
    bool openFile(const QString &path, QString *error = nullptr);

    int  piece() const { return m_piece; }
    int  row() const { return m_row; }
    int  rowCount() const;
    /** The carriage pass of the current row the machine is at (0-based), when a counter counts passes. */
    int  pass() const { return m_pass; }
    int  passesInRow(int row) const;
    const KnitJob &job() const { return m_job; }
    QStringList recentFiles() const;

    /** Shows the window on the saved screen (default: monitor 0), fullscreen or as a normal window. */
    void showOnSavedScreen(bool fullscreen);

    /**
     * Follows this row counter: its rows move the current row. The window takes ownership and shows its state in
     * the top bar; the counter is started by the caller. nullptr = buttons only.
     */
    void setRowCounter(RowCounterSource *counter);
    RowCounterSource *rowCounter() const { return m_counter; }

    /**
     * Uses the counter described by config (buttons only for kind "manual"): replaces the current one, starts the new
     * one and, with save, keeps the config for the next start. A counter that cannot start is still shown, with the
     * reason in the top bar. Returns false and the reason in error if the config itself is wrong or the counter did not start.
     */
    bool applyCounterConfig(const RowCounterConfig &config, bool save, QString *error = nullptr);
    const RowCounterConfig &counterConfig() const { return m_counterConfig; }

    // ---- Yarns
    /** Reads the yarn inventory; a missing file is no yarns. The file is also where used yarn is deducted. Never overwrites an unreadable file. */
    bool loadYarns(const QString &path, QString *error = nullptr);
    /** Use this inventory (and do not save it anywhere). */
    void setYarnInventory(const stitchcore::YarnInventory &inventory);
    const stitchcore::YarnInventory &yarnInventory() const { return m_inventory; }
    /** With false the yarn of a piece is never asked for (screenshots). */
    void setAskForYarn(bool ask) { m_askForYarn = ask; }
    /** Knit the main yarn of every piece with this yarn (id or name) unless the project names one. False if there is no such yarn. */
    bool setMainYarn(const QString &idOrName, QString *error = nullptr);
    /** Yarn slots a piece has stitches in: "main" first, then the colors of its color patterns. */
    QStringList yarnSlots(int piece) const;
    /** Yarn id by slot for a piece: what was chosen, else what the project names. Slots without a yarn are left out. */
    QMap<QString, QString> yarnsOfPiece(int piece) const;
    /** Meters (with margin) each slot of a piece needs with its yarn. */
    QMap<QString, qreal> yarnNeeded(int piece) const;
    /** Asks for the yarn of the current piece if the project does not name it, there are yarns, and it was not asked yet. */
    bool askYarnIfNeeded();
    /** Question "take used yarn from the stock?"; returns true for yes. Replaced by the tests. */
    using DeductPrompt = std::function<bool(const QString &text)>;
    void setDeductPrompt(const DeductPrompt &prompt) { m_deductPrompt = prompt; }

public slots:
    void setPiece(int piece);
    void setRow(int row);                 //!< clamped to 0 .. rowCount + 1; starts at the first pass of the row
    void nextRow() { setRow(m_row + 1); }
    void previousRow() { setRow(m_row - 1); }
    void nextBlock();                     //!< to the first row of the next block of instructions (Page Down)
    void previousBlock();                 //!< to the first row of this block, or of the one before if already there (Page Up)
    void chooseFile();
    void choosePiece();
    void jumpToRow();
    void openSettings();
    void chooseYarn();                    //!< choose the yarns of this piece again

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void updateView();
    void stepPasses(int delta);          //!< from the row counter: a pass of the machine, or several
    void saveState() const;
    QString rowText() const;
    QString yarnKey(int piece) const;
    QStringList unresolvedYarnSlots(int piece) const;
    bool runYarnDialog(const QStringList &slotNames);
    void updateYarnLabel();
    void offerYarnDeduction();

    KnitJob       m_job;
    int           m_piece = 0;
    int           m_row = 0;
    int           m_pass = 0;
    bool          m_screenWatched = false;
    QLabel       *m_title = nullptr;
    QLabel       *m_rowLabel = nullptr;
    QLabel       *m_counterLabel = nullptr;
    RowCounterSource *m_counter = nullptr;
    RowCounterConfig  m_counterConfig;
    StepView     *m_view = nullptr;
    QPushButton  *m_prev = nullptr;
    QPushButton  *m_next = nullptr;
    QPushButton  *m_pieceButton = nullptr;
    QPushButton  *m_yarnButton = nullptr;
    QLabel       *m_yarnLabel = nullptr;
    stitchcore::YarnInventory m_inventory;
    QString       m_yarnPath;
    bool          m_askForYarn = true;
    QString       m_presetMain;                                   //!< yarn id for the main slot from --yarn
    QHash<int, QMap<QString, QString>> m_yarnChoice;              //!< by piece: yarn id by slot
    QSet<int>     m_yarnAsked;
    DeductPrompt  m_deductPrompt;
};

#endif
