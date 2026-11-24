#include "yarnbox_window.h"

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include "yarn_inventory_widget.h"

using namespace stitchcore;

YarnboxWindow::YarnboxWindow(const QString &yarnFile, const QStringList &stitchTypes, QWidget *parent)
    : QMainWindow(parent), m_file(yarnFile), m_stitchTypes(stitchTypes)
{
    resize(960, 680);
    QMenu *file = menuBar()->addMenu(tr("&File"));
    QAction *save = file->addAction(tr("&Save"), this, &YarnboxWindow::saveAction, QKeySequence::Save);
    save->setObjectName(QStringLiteral("saveAction"));
    QAction *again = file->addAction(tr("&Reload"), this, &YarnboxWindow::reload, QKeySequence::Refresh);
    again->setObjectName(QStringLiteral("reloadAction"));
    file->addSeparator();
    file->addAction(tr("&Quit"), this, &QWidget::close, QKeySequence::Quit);
    createEditor(YarnInventory());
    setModified(false);
}

const YarnInventory &YarnboxWindow::inventory() const
{
    return m_editor->inventory();
}

void YarnboxWindow::createEditor(const YarnInventory &inventory)
{
    m_editor = new yarnui::YarnInventoryWidget(inventory, m_stitchTypes);
    connect(m_editor, &yarnui::YarnInventoryWidget::changed, this, [this]() { setModified(true); });
    setCentralWidget(m_editor);               // the old editor is deleted by the window
}

void YarnboxWindow::setModified(bool modified)
{
    m_modified = modified;
    setWindowTitle(tr("Yarnbox - %1[*]").arg(QFileInfo(m_file).fileName()));
    setWindowModified(modified);
}

bool YarnboxWindow::load(QString *error)
{
    YarnInventory inventory;
    QString message;
    const bool ok = YarnInventory::load(m_file, inventory, &message);
    m_loadFailed = !ok;
    createEditor(ok ? inventory : YarnInventory());
    m_fileTime = QFileInfo(m_file).exists() ? QFileInfo(m_file).lastModified() : QDateTime();
    setModified(false);
    if (!ok)
    {
        if (error) *error = message;
        statusBar()->showMessage(tr("%1 could not be read: %2").arg(m_file, message));
        return false;
    }
    statusBar()->showMessage(tr("%1 yarns in %2").arg(inventory.size()).arg(m_file), 5000);
    return true;
}

bool YarnboxWindow::save(QString *error)
{
    auto fail = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };
    QString problem;
    if (!m_editor->validate(&problem))
    {
        return fail(problem);
    }
    if (m_loadFailed)
    {
        return fail(tr("The file could not be read, so it is not overwritten. Fix or remove it, then choose Reload."));
    }
    const QFileInfo info(m_file);
    if (info.exists() && info.lastModified() != m_fileTime)
    {
        const QString text = tr("%1 was changed after it was read (the guider may have taken used yarn from the stock). "
                                "Overwrite it with what you see here? Choose No, then Reload to see the new stock.").arg(m_file);
        const bool yes = m_overwritePrompt ? m_overwritePrompt(text)
                                           : QMessageBox::question(this, tr("Yarnbox"), text) == QMessageBox::Yes;
        if (!yes)
        {
            return fail(tr("Not saved: the file changed."));
        }
    }
    QDir().mkpath(info.absolutePath());
    QString message;
    if (!m_editor->inventory().save(m_file, &message))
    {
        return fail(message);
    }
    m_fileTime = QFileInfo(m_file).lastModified();
    setModified(false);
    statusBar()->showMessage(tr("Saved to %1").arg(m_file), 5000);
    return true;
}

void YarnboxWindow::saveAction()
{
    QString error;
    if (!save(&error))
    {
        QMessageBox::warning(this, tr("Yarnbox"), error);
    }
}

void YarnboxWindow::reload()
{
    if (!maybeSave())
    {
        return;
    }
    QString error;
    if (!load(&error))
    {
        QMessageBox::warning(this, tr("Yarnbox"), error);
    }
}

bool YarnboxWindow::maybeSave()
{
    if (!m_modified)
    {
        return true;
    }
    SaveChoice choice;
    if (m_savePrompt)
    {
        choice = m_savePrompt();
    }
    else
    {
        QMessageBox box(QMessageBox::Question, tr("Yarnbox"), tr("The yarns have unsaved changes."),
                        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, this);
        const int answer = box.exec();
        choice = answer == QMessageBox::Save ? SaveChoice::Save : answer == QMessageBox::Discard ? SaveChoice::Discard : SaveChoice::Cancel;
    }
    if (choice == SaveChoice::Cancel)
    {
        return false;
    }
    if (choice == SaveChoice::Save)
    {
        QString error;
        if (!save(&error))
        {
            QMessageBox::warning(this, tr("Yarnbox"), error);
            return false;
        }
    }
    return true;
}

void YarnboxWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) event->accept();
    else event->ignore();
}
