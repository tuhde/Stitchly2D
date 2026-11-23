#include "guide_window.h"

#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QScreen>
#include <QWindow>
#include <QDialog>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "settings_dialog.h"
#include "guide_scale.h"
#include "guide_theme.h"
#include "open_dialog.h"
#include "row_counter.h"
#include "step_view.h"

namespace
{
QSettings &settings()
{
    static QSettings s(QStringLiteral("Stitchly2D"), QStringLiteral("guider"));
    return s;
}

QString rowKey(const QString &file, const QString &piece)
{
    return QStringLiteral("rows/") + QString::fromUtf8((file + QLatin1Char('|') + piece).toUtf8().toBase64(QByteArray::Base64UrlEncoding));
}
}

GuideWindow::GuideWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("guider"));
    setFocusPolicy(Qt::StrongFocus);
    const int u = guideUnit(this);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(u, u / 2, u, u);
    root->setSpacing(u / 2);

    auto *top = new QHBoxLayout;
    auto *openButton = new QPushButton(tr("Open"), this);
    m_pieceButton = new QPushButton(tr("Piece"), this);
    auto *settingsButton = new QPushButton(tr("Settings"), this);
    m_title = new QLabel(this);
    m_rowLabel = new QLabel(this);
    QFont big = font();
    big.setPixelSize(u * 2);
    big.setBold(true);
    m_title->setFont(big);
    m_rowLabel->setFont(big);
    m_rowLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    top->addWidget(openButton);
    top->addWidget(m_pieceButton);
    top->addWidget(settingsButton);
    top->addSpacing(u);
    m_counterLabel = new QLabel(this);
    QFont small = font();
    small.setPixelSize(u);
    m_counterLabel->setFont(small);
    m_counterLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_counterLabel->hide();
    top->addWidget(m_title, 1);
    top->addWidget(m_counterLabel);
    top->addSpacing(u);
    top->addWidget(m_rowLabel);
    root->addLayout(top);

    m_view = new StepView(this);
    root->addWidget(m_view, 1);

    auto *bottom = new QHBoxLayout;
    m_prev = new QPushButton(tr("< Previous"), this);
    m_next = new QPushButton(tr("Next >"), this);
    auto *jump = new QPushButton(tr("Go to row..."), this);
    for (QPushButton *b : {m_prev, m_next, jump})
    {
        b->setMinimumHeight(u * 5);          // bigger than the default touch target: used all the time
        b->setFocusPolicy(Qt::NoFocus);
        QFont f = b->font();
        f.setPixelSize(u * 3 / 2 + u / 2);
        b->setFont(f);
    }
    for (QPushButton *b : {openButton, m_pieceButton, settingsButton})
    {
        b->setFocusPolicy(Qt::NoFocus);
    }
    bottom->addWidget(m_prev, 2);
    bottom->addWidget(jump, 1);
    bottom->addWidget(m_next, 2);
    root->addLayout(bottom);

    setAttribute(Qt::WA_StyledBackground, true);

    connect(openButton, &QPushButton::clicked, this, &GuideWindow::chooseFile);
    connect(m_pieceButton, &QPushButton::clicked, this, &GuideWindow::choosePiece);
    connect(settingsButton, &QPushButton::clicked, this, &GuideWindow::openSettings);
    connect(m_prev, &QPushButton::clicked, this, &GuideWindow::previousRow);
    connect(m_next, &QPushButton::clicked, this, &GuideWindow::nextRow);
    connect(jump, &QPushButton::clicked, this, &GuideWindow::jumpToRow);
    connect(m_view, &StepView::tappedAbove, this, &GuideWindow::previousRow);
    connect(m_view, &StepView::tappedBelow, this, &GuideWindow::nextRow);
    updateView();
}

int GuideWindow::rowCount() const
{
    return m_piece >= 0 && m_piece < m_job.pieces.size() ? m_job.pieces.at(m_piece).rowCount : 0;
}

QStringList GuideWindow::recentFiles() const
{
    return settings().value(QStringLiteral("recent")).toStringList();
}

bool GuideWindow::openFile(const QString &file, QString *error)
{
    const QString path = QFileInfo(file).absoluteFilePath();   // the saved "last file" must work from any working directory
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_title->setText(tr("Loading..."));
    QApplication::processEvents();
    KnitJob job;
    QString message;
    const bool ok = buildKnitJob(path, job, &message);
    QApplication::restoreOverrideCursor();
    if (!ok || job.pieces.isEmpty())
    {
        if (error) *error = ok ? tr("The pattern has no pieces.") : message;
        updateView();
        return false;
    }
    m_job = job;
    QStringList recent = recentFiles();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > 12) recent.removeLast();
    settings().setValue(QStringLiteral("recent"), recent);
    settings().setValue(QStringLiteral("lastFile"), path);

    // The piece last worked on, else the first one that has stitches.
    int piece = -1;
    const QString lastPiece = settings().value(QStringLiteral("lastPiece/") + QString::fromUtf8(path.toUtf8().toBase64())).toString();
    for (int i = 0; i < m_job.pieces.size(); ++i)
    {
        if (m_job.pieces.at(i).name == lastPiece && m_job.pieces.at(i).rowCount > 0) piece = i;
    }
    for (int i = 0; piece < 0 && i < m_job.pieces.size(); ++i)
    {
        if (m_job.pieces.at(i).rowCount > 0) piece = i;
    }
    m_piece = -1;                 // force the reload of the row
    setPiece(piece < 0 ? 0 : piece);
    return true;
}

void GuideWindow::setPiece(int piece)
{
    if (piece < 0 || piece >= m_job.pieces.size())
    {
        return;
    }
    if (m_piece >= 0 && m_piece != piece)
    {
        saveState();
    }
    m_piece = piece;
    const KnitPiece &p = m_job.pieces.at(piece);
    m_row = qBound(0, settings().value(rowKey(m_job.input, p.name), 0).toInt(), p.rowCount + 1);
    saveState();
    updateView();
}

void GuideWindow::setRow(int row)
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size())
    {
        return;      // nothing open
    }
    row = qBound(0, row, rowCount() + 1);
    if (row == m_row)
    {
        return;
    }
    m_row = row;
    saveState();
    updateView();
}

void GuideWindow::nextBlock()
{
    if (m_piece >= 0 && m_piece < m_job.pieces.size())
    {
        setRow(stitchcore::nextStepRow(m_job.pieces.at(m_piece).steps, m_row));
    }
}

void GuideWindow::previousBlock()
{
    if (m_piece >= 0 && m_piece < m_job.pieces.size())
    {
        setRow(stitchcore::previousStepRow(m_job.pieces.at(m_piece).steps, m_row));
    }
}

void GuideWindow::saveState() const
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size())
    {
        return;
    }
    const QString name = m_job.pieces.at(m_piece).name;
    settings().setValue(rowKey(m_job.input, name), m_row);
    settings().setValue(QStringLiteral("lastPiece/") + QString::fromUtf8(m_job.input.toUtf8().toBase64()), name);
}

QString GuideWindow::rowText() const
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size())
    {
        return QString();
    }
    const KnitPiece &p = m_job.pieces.at(m_piece);
    if (p.rowCount == 0) return tr("no stitches");
    if (m_row == 0) return tr("Cast on");
    if (m_row > p.rowCount) return tr("Bind off");
    return stitchcore::isRound(p.method) ? tr("Round %1 / %2").arg(m_row).arg(p.rowCount)
                                         : tr("Row %1 / %2").arg(m_row).arg(p.rowCount);
}

void GuideWindow::updateView()
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size())
    {
        m_title->setText(tr("No pattern open"));
        m_rowLabel->clear();
        m_view->setSteps({});
        m_prev->setEnabled(false);
        m_next->setEnabled(false);
        return;
    }
    const KnitPiece &p = m_job.pieces.at(m_piece);
    m_title->setText(p.name);
    m_rowLabel->setText(rowText());
    QStringList texts;
    for (const stitchcore::InstructionStep &s : p.steps) texts << s.text;
    if (m_view->property("piece").toInt() != m_piece + 1 || m_view->property("file").toString() != m_job.input)
    {
        m_view->setSteps(texts);
        m_view->setProperty("piece", m_piece + 1);
        m_view->setProperty("file", m_job.input);
    }
    m_view->setCurrent(stitchcore::stepsForRow(p.steps, m_row));
    m_prev->setEnabled(m_row > 0);
    m_next->setEnabled(m_row <= p.rowCount);
}

void GuideWindow::chooseFile()
{
    const QString last = settings().value(QStringLiteral("lastFile")).toString();
    OpenDialog dialog(recentFiles(), last.isEmpty() ? QString() : QFileInfo(last).absolutePath(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }
    QString error;
    if (!openFile(dialog.chosenFile(), &error))
    {
        m_title->setText(tr("Could not open: %1").arg(error));
    }
}

void GuideWindow::choosePiece()
{
    if (m_job.pieces.isEmpty())
    {
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Piece"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *list = new QListWidget(&dialog);
    for (int i = 0; i < m_job.pieces.size(); ++i)
    {
        const KnitPiece &p = m_job.pieces.at(i);
        auto *item = new QListWidgetItem(p.rowCount > 0 ? tr("%1  (%2 rows)").arg(p.name).arg(p.rowCount) : tr("%1  (no stitches)").arg(p.name), list);
        if (p.rowCount == 0) item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
    list->setCurrentRow(m_piece);
    layout->addWidget(list);
    auto *cancel = new QPushButton(tr("Cancel"), &dialog);
    layout->addWidget(cancel);
    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(list, &QListWidget::itemClicked, &dialog, &QDialog::accept);
    const int u = guideUnit(this);
    dialog.resize(u * 36, u * 30);
    if (dialog.exec() == QDialog::Accepted && list->currentRow() >= 0)
    {
        setPiece(list->currentRow());
    }
}

void GuideWindow::jumpToRow()
{
    bool ok = false;
    const int row = QInputDialog::getInt(this, tr("Go to row"), tr("Row (0 = cast on):"), m_row, 0, rowCount() + 1, 1, &ok);
    if (ok)
    {
        setRow(row);
    }
}

void GuideWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Right: case Qt::Key_Down: case Qt::Key_Space:
            nextRow();
            break;
        case Qt::Key_Left: case Qt::Key_Up: case Qt::Key_Backspace:
            previousRow();
            break;
        case Qt::Key_PageDown:
            nextBlock();
            break;
        case Qt::Key_PageUp:
            previousBlock();
            break;
        case Qt::Key_Home:
            setRow(0);
            break;
        case Qt::Key_End:
            setRow(rowCount() + 1);           // the bind-off block
            break;
        case Qt::Key_O:
            chooseFile();
            break;
        case Qt::Key_P:
            choosePiece();
            break;
        case Qt::Key_G:
            jumpToRow();
            break;
        case Qt::Key_S:
            openSettings();
            break;
        case Qt::Key_F11:
            isFullScreen() ? showNormal() : showFullScreen();
            break;
        case Qt::Key_Escape:
            if (isFullScreen()) showNormal();
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}

void GuideWindow::showOnSavedScreen(bool fullscreen)
{
    QScreen *screen = guideScreen();
    // Fullscreen goes to the screen the window is on, so put the window there before it is shown.
    create();
    windowHandle()->setScreen(screen);
    const QRect area = screen->availableGeometry();
    QRect target(QPoint(), size());
    target.moveCenter(area.center());
    if (!area.contains(target))
    {
        target = QRect(area.topLeft(), size().boundedTo(area.size()));
    }
    setGeometry(target);
    if (fullscreen)
    {
        showFullScreen();
    }
    else
    {
        show();
    }
}

void GuideWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // Remember every move to another monitor, not only the one it is on when closed.
    if (windowHandle() && !m_screenWatched)
    {
        m_screenWatched = true;
        connect(windowHandle(), &QWindow::screenChanged, this, [](QScreen *screen) { saveGuideScreen(screen); });
    }
}

void GuideWindow::closeEvent(QCloseEvent *event)
{
    saveGuideScreen(screen());
    QWidget::closeEvent(event);
}

void GuideWindow::setRowCounter(RowCounterSource *counter)
{
    if (m_counter)
    {
        m_counter->disconnect(this);
        m_counter->deleteLater();
    }
    m_counter = counter;
    m_counterLabel->setVisible(counter != nullptr);
    if (!counter)
    {
        return;
    }
    counter->setParent(this);
    auto showState = [this](RowCounterSource::State state, const QString &message) {
        const bool error = state == RowCounterSource::State::Error;
        setStatusStyle(m_counterLabel, error ? QStringLiteral("error") : QStringLiteral("ok"));
        m_counterLabel->setText(error ? message : (state == RowCounterSource::State::Running ? m_counter->description() : tr("counter stopped")));
        m_counterLabel->setToolTip(message);
    };
    connect(counter, &RowCounterSource::rowsStepped, this, [this](int delta) { setRow(m_row + delta); });
    connect(counter, &RowCounterSource::stateChanged, this, showState);
    showState(counter->state(), counter->message());
}

bool GuideWindow::applyCounterConfig(const RowCounterConfig &config, bool save, QString *error)
{
    const QString problem = config.problem();
    if (!problem.isEmpty())
    {
        if (error) *error = problem;
        return false;
    }
    m_counterConfig = config;
    if (save)
    {
        QSettings s(QStringLiteral("Stitchly2D"), QStringLiteral("guider"));
        config.save(s);
    }
    RowCounterSource *source = createRowCounter(config, nullptr, this);
    setRowCounter(source);               // nullptr for "manual": buttons only
    if (source && !source->start(error))
    {
        return false;
    }
    return true;
}

void GuideWindow::openSettings()
{
    // The test in the dialog needs the pin, so let go of it while the dialog is open.
    if (m_counter)
    {
        m_counter->stop();
    }
    SettingsDialog dialog(m_counterConfig, this);
    if (dialog.exec() == QDialog::Accepted)
    {
        if (dialog.themeChoice() != guideThemeChoice())
        {
            setGuideThemeChoice(dialog.themeChoice());
        }
        applyCounterConfig(dialog.config(), true);
    }
    else if (m_counter)
    {
        m_counter->start();
    }
}
