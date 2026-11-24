#include "guide_window.h"

#include <QInputDevice>

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
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include "settings_dialog.h"
#include "guide_scale.h"
#include "guide_theme.h"
#include "open_dialog.h"
#include "row_counter.h"
#include "step_view.h"
#include "yarn_dialog.h"
#include "../../libs/stitchcore/yarn_estimate.h"

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
    m_yarnButton = new QPushButton(tr("Yarn"), this);
    m_yarnButton->setObjectName(QStringLiteral("yarnButton"));
    m_yarnButton->hide();
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
    top->addWidget(m_yarnButton);
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
    m_yarnLabel = new QLabel(this);
    m_yarnLabel->setObjectName(QStringLiteral("yarnLabel"));
    m_yarnLabel->setTextFormat(Qt::RichText);
    m_yarnLabel->hide();
    QFont yarnFont = font();
    yarnFont.setPixelSize(u * 3 / 2);
    m_yarnLabel->setFont(yarnFont);
    root->addWidget(m_yarnLabel);

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
    for (QPushButton *b : {openButton, m_pieceButton, m_yarnButton, settingsButton})
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
    connect(m_yarnButton, &QPushButton::clicked, this, &GuideWindow::chooseYarn);
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
    m_yarnChoice.clear();
    m_yarnAsked.clear();
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
    m_pass = 0;
    const KnitPiece &p = m_job.pieces.at(piece);
    m_row = qBound(0, settings().value(rowKey(m_job.input, p.name), 0).toInt(), p.rowCount + 1);
    saveState();
    updateView();
    if (m_askForYarn)
    {
        askYarnIfNeeded();
    }
}

int GuideWindow::passesInRow(int row) const
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size())
    {
        return 1;
    }
    const auto &passes = m_job.pieces.at(m_piece).rowPasses;
    return row >= 0 && row < passes.size() ? qMax(1, passes.at(row).count()) : 1;
}

void GuideWindow::stepPasses(int delta)
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size())
    {
        return;      // nothing open
    }
    // The row shown is the one being knitted: after the last pass of a row the next row starts at its first pass.
    for (; delta > 0; --delta)
    {
        if (m_pass + 1 < passesInRow(m_row))      ++m_pass;
        else if (m_row <= rowCount())             { ++m_row; m_pass = 0; }
        else break;                                // after the last row: nothing more to count
    }
    for (; delta < 0; ++delta)
    {
        if (m_pass > 0)                           --m_pass;
        else if (m_row > 0)                       { --m_row; m_pass = passesInRow(m_row) - 1; }
        else break;
    }
    saveState();
    updateView();
    offerYarnDeduction();
}

void GuideWindow::setRow(int row)
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size())
    {
        return;      // nothing open
    }
    row = qBound(0, row, rowCount() + 1);
    m_pass = 0;                                   // a row set by hand starts at its first pass ...
    if (m_counter)
    {
        m_counter->resetPending();                // ... and sensor pulses counted before do not belong to it
    }
    if (row == m_row)
    {
        updateView();
        return;
    }
    m_row = row;
    if (row == 0)
    {
        settings().remove(QStringLiteral("yarnUsed/") + yarnKey(m_piece));       // knitted again: the yarn can be taken off again
    }
    saveState();
    updateView();
    offerYarnDeduction();
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
        m_yarnLabel->hide();
        m_prev->setEnabled(false);
        m_next->setEnabled(false);
        return;
    }
    const KnitPiece &p = m_job.pieces.at(m_piece);
    m_title->setText(p.name);
    // [passes] Row 16 / 200, the pass the machine is at marked
    const stitchcore::RowPasses &passes = m_row < p.rowPasses.size() ? p.rowPasses.at(m_row) : stitchcore::RowPasses();
    const QString passText = stitchcore::passesText(passes, qMin(m_pass, passes.count() - 1), true);
    m_rowLabel->setTextFormat(Qt::RichText);
    m_rowLabel->setText((passText.isEmpty() ? QString() : QStringLiteral("[%1]&nbsp;&nbsp;").arg(passText)) + rowText().toHtmlEscaped());
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
    m_yarnButton->setVisible(!m_inventory.yarns().isEmpty());
    updateYarnLabel();
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
        case Qt::Key_Y:
            if (!m_inventory.yarns().isEmpty()) chooseYarn();
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
    connect(counter, &RowCounterSource::passStepped, this, &GuideWindow::stepPasses);
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

// ---------------------------------------------------------------------------------------------------------------------
// Yarns

using namespace stitchcore;

bool GuideWindow::loadYarns(const QString &path, QString *error)
{
    YarnInventory inventory;
    if (!YarnInventory::load(path, inventory, error))
    {
        m_inventory = YarnInventory();
        m_yarnPath.clear();                  // an unreadable file is left alone
        updateView();
        return false;
    }
    m_inventory = inventory;
    m_yarnPath = path;
    updateView();
    return true;
}

void GuideWindow::setYarnInventory(const YarnInventory &inventory)
{
    m_inventory = inventory;
    m_yarnPath.clear();
    updateView();
}

bool GuideWindow::setMainYarn(const QString &idOrName, QString *error)
{
    QString found;
    int matches = 0;
    for (const Yarn &y : m_inventory.yarns())
    {
        if (y.id == idOrName) { found = y.id; matches = 1; break; }
        if (y.displayName().contains(idOrName, Qt::CaseInsensitive)) { found = y.id; ++matches; }
    }
    if (matches != 1)
    {
        if (error) *error = matches == 0 ? tr("There is no yarn \"%1\" in the inventory.").arg(idOrName)
                                         : tr("More than one yarn matches \"%1\".").arg(idOrName);
        return false;
    }
    m_presetMain = found;
    updateView();
    return true;
}

QString GuideWindow::yarnKey(int piece) const
{
    const QString name = piece >= 0 && piece < m_job.pieces.size() ? m_job.pieces.at(piece).name : QString();
    return QString::fromUtf8((m_job.input + QLatin1Char('|') + name).toUtf8().toBase64(QByteArray::Base64UrlEncoding));
}

QStringList GuideWindow::yarnSlots(int piece) const
{
    QStringList slotNames;
    if (piece < 0 || piece >= m_job.pieces.size())
    {
        return slotNames;
    }
    QStringList colors;
    bool main = false;
    for (const YarnUse &u : m_job.pieces.at(piece).yarnUses)
    {
        if (u.stitches <= 0) continue;
        if (u.slot == QLatin1String(kMainYarnSlot)) main = true;
        else if (!colors.contains(u.slot)) colors << u.slot;
    }
    colors.sort();
    if (main) slotNames << QString::fromLatin1(kMainYarnSlot);
    return slotNames + colors;
}

QMap<QString, QString> GuideWindow::yarnsOfPiece(int piece) const
{
    QMap<QString, QString> ids;
    const QMap<QString, QString> chosen = m_yarnChoice.value(piece);
    for (const QString &slot : yarnSlots(piece))
    {
        QString id = chosen.value(slot);
        if (id.isEmpty()) id = m_job.yarns.value(slot).id;
        if (id.isEmpty() && slot == QLatin1String(kMainYarnSlot)) id = m_presetMain;
        if (!id.isEmpty()) ids.insert(slot, id);
    }
    return ids;
}

QMap<QString, qreal> GuideWindow::yarnNeeded(int piece) const
{
    QMap<QString, Yarn> bySlot;
    const QMap<QString, QString> ids = yarnsOfPiece(piece);
    for (auto it = ids.cbegin(); it != ids.cend(); ++it)
    {
        if (const Yarn *y = m_inventory.find(it.value())) bySlot.insert(it.key(), *y);
    }
    return piece >= 0 && piece < m_job.pieces.size() ? estimateYarnMeters(m_job.pieces.at(piece).yarnUses, bySlot)
                                                      : QMap<QString, qreal>();
}

QStringList GuideWindow::unresolvedYarnSlots(int piece) const
{
    QStringList open;
    const QMap<QString, QString> ids = yarnsOfPiece(piece);
    for (const QString &slot : yarnSlots(piece))
    {
        if (!ids.contains(slot)) open << slot;
    }
    return open;
}

bool GuideWindow::runYarnDialog(const QStringList &slotNames)
{
    if (slotNames.isEmpty() || m_piece < 0 || m_piece >= m_job.pieces.size())
    {
        return false;
    }
    const KnitPiece &p = m_job.pieces.at(m_piece);
    QMap<QString, QString> before = yarnsOfPiece(m_piece);
    for (const QString &slot : slotNames)
    {
        if (!before.contains(slot))
        {
            const QString remembered = settings().value(QStringLiteral("yarn/") + yarnKey(m_piece) + QLatin1Char('/') + slot).toString();
            if (m_inventory.find(remembered)) before.insert(slot, remembered);
        }
    }
    YarnDialog dialog(p.name, slotNames, m_inventory, p.yarnUses, before, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }
    const QMap<QString, QString> chosen = dialog.choice();
    QMap<QString, QString> &mine = m_yarnChoice[m_piece];
    for (const QString &slot : slotNames)
    {
        const QString key = QStringLiteral("yarn/") + yarnKey(m_piece) + QLatin1Char('/') + slot;
        if (chosen.contains(slot))
        {
            mine.insert(slot, chosen.value(slot));
            settings().setValue(key, chosen.value(slot));
        }
        else
        {
            mine.remove(slot);
            settings().remove(key);
        }
    }
    updateView();
    return true;
}

bool GuideWindow::askYarnIfNeeded()
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size() || m_inventory.yarns().isEmpty() || m_yarnAsked.contains(m_piece))
    {
        return false;
    }
    const QStringList open = unresolvedYarnSlots(m_piece);
    if (open.isEmpty())
    {
        return false;
    }
    m_yarnAsked.insert(m_piece);
    return runYarnDialog(open);
}

void GuideWindow::chooseYarn()
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size() || m_inventory.yarns().isEmpty())
    {
        return;
    }
    m_yarnAsked.insert(m_piece);
    runYarnDialog(yarnSlots(m_piece));
}

void GuideWindow::updateYarnLabel()
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size())
    {
        m_yarnLabel->hide();
        return;
    }
    const KnitPiece &p = m_job.pieces.at(m_piece);
    const RowPasses passes = m_row < p.rowPasses.size() ? p.rowPasses.at(m_row) : RowPasses();
    const QString mainSlot = QString::fromLatin1(kMainYarnSlot);
    const QStringList rowSlots = passes.yarns.isEmpty() ? QStringList{mainSlot} : passes.yarns;
    const QString currentSlot = passes.count() > 1 ? passes.passes.at(qMin(m_pass, passes.count() - 1)).yarn : QString();
    const QMap<QString, QString> ids = yarnsOfPiece(m_piece);
    QStringList parts;
    for (const QString &slot : rowSlots)
    {
        const QString id = ids.value(slot);
        QString text;
        QString color;
        if (id.isEmpty())
        {
            if (slot == mainSlot) continue;
            text = slot.toHtmlEscaped();                 // a color without a yarn
        }
        else
        {
            QString name = m_job.yarns.value(slot).id == id ? m_job.yarns.value(slot).name : QString();
            if (const Yarn *y = m_inventory.find(id))
            {
                name = y->displayName();
                if (!y->dyeLot.isEmpty()) name += tr(" (lot %1)").arg(y->dyeLot);
                color = y->colorHex;
            }
            else
            {
                color = m_job.yarns.value(slot).colorHex;
            }
            text = (slot == mainSlot ? QString() : slot.toHtmlEscaped() + QStringLiteral(": ")) + name.toHtmlEscaped();
        }
        if (!color.isEmpty())
        {
            text = QStringLiteral("<span style=\"color:%1\">&#9679;</span> ").arg(color) + text;
        }
        if (!currentSlot.isEmpty() && slot == currentSlot) text = QStringLiteral("<b><u>%1</u></b>").arg(text);
        parts << text;
    }
    m_yarnLabel->setText(parts.join(QStringLiteral("&nbsp;&nbsp;&nbsp;&nbsp;")));
    m_yarnLabel->setVisible(!parts.isEmpty());
}

void GuideWindow::offerYarnDeduction()
{
    if (m_piece < 0 || m_piece >= m_job.pieces.size() || rowCount() == 0 || m_row <= rowCount())
    {
        return;
    }
    const QString key = QStringLiteral("yarnUsed/") + yarnKey(m_piece);
    if (settings().value(key).toBool())
    {
        return;
    }
    const QMap<QString, QString> ids = yarnsOfPiece(m_piece);
    const QMap<QString, qreal> needed = yarnNeeded(m_piece);
    QStringList lines;
    for (auto it = needed.cbegin(); it != needed.cend(); ++it)
    {
        if (it.value() <= 0) continue;
        if (const Yarn *y = m_inventory.find(ids.value(it.key())))
        {
            lines << tr("%1: about %2 m").arg(y->displayName()).arg(it.value(), 0, 'f', 0);
        }
    }
    if (lines.isEmpty())
    {
        return;
    }
    settings().setValue(key, true);                  // asked once, until the piece is started again
    const QString text = tr("%1 is finished. Take the yarn from the stock?\n\n%2").arg(m_job.pieces.at(m_piece).name, lines.join(QLatin1Char('\n')));
    const bool yes = m_deductPrompt ? m_deductPrompt(text)
                                    : QMessageBox::question(this, tr("Yarn"), text) == QMessageBox::Yes;
    if (!yes)
    {
        return;
    }
    for (auto it = needed.cbegin(); it != needed.cend(); ++it)
    {
        if (Yarn *y = m_inventory.find(ids.value(it.key())))
        {
            consumeYarn(*y, qMin(it.value(), y->stockMeters()));
        }
    }
    if (!m_yarnPath.isEmpty())
    {
        QString error;
        if (!m_inventory.save(m_yarnPath, &error))
        {
            QMessageBox::warning(this, tr("Yarn"), tr("The yarn stock could not be saved: %1").arg(error));
        }
    }
    updateView();
}

bool GuideWindow::hasTouchScreen()
{
    const auto devices = QInputDevice::devices();
    for (const QInputDevice *device : devices)
    {
        if (device->type() == QInputDevice::DeviceType::TouchScreen) return true;
    }
    return false;
}

void GuideWindow::updateCursor()
{
    if (hasTouchScreen()) setCursor(Qt::BlankCursor);
    else unsetCursor();
}
