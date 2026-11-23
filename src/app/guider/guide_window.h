// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef GUIDE_WINDOW_H
#define GUIDE_WINDOW_H

#include <QWidget>

#include "../../libs/stitchload/core/knit_job.h"

class QLabel;
class QScreen;
class QPushButton;
class StepView;
class RowCounterSource;

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

public slots:
    void setPiece(int piece);
    void setRow(int row);                 //!< clamped to 0 .. rowCount + 1
    void nextRow() { setRow(m_row + 1); }
    void previousRow() { setRow(m_row - 1); }
    void chooseFile();
    void choosePiece();
    void jumpToRow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void updateView();
    void saveState() const;
    QString rowText() const;

    KnitJob       m_job;
    int           m_piece = 0;
    int           m_row = 0;
    bool          m_screenWatched = false;
    QLabel       *m_title = nullptr;
    QLabel       *m_rowLabel = nullptr;
    QLabel       *m_counterLabel = nullptr;
    RowCounterSource *m_counter = nullptr;
    StepView     *m_view = nullptr;
    QPushButton  *m_prev = nullptr;
    QPushButton  *m_next = nullptr;
    QPushButton  *m_pieceButton = nullptr;
};

#endif
