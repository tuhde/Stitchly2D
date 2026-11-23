#include "open_dialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "guide_scale.h"

OpenDialog::OpenDialog(const QStringList &recent, const QString &startDir, QWidget *parent)
    : QDialog(parent), m_startDir(startDir)
{
    setWindowTitle(tr("Open"));
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel(tr("Choose a project (.stitchly) or pattern (.sm2d)"), this);
    layout->addWidget(title);

    m_list = new QListWidget(this);
    for (const QString &path : recent)
    {
        if (!QFileInfo::exists(path))
        {
            continue;
        }
        auto *item = new QListWidgetItem(QFileInfo(path).fileName() + QLatin1Char('\n') + QFileInfo(path).absolutePath(), m_list);
        item->setData(Qt::UserRole, path);
    }
    m_list->setTextElideMode(Qt::ElideMiddle);                    // long folder names: no sideways scrolling
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(m_list, 1);
    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) { accepted(m_list->row(item)); });

    auto *buttons = new QHBoxLayout;
    auto *browseButton = new QPushButton(tr("Browse..."), this);
    auto *cancelButton = new QPushButton(tr("Cancel"), this);
    buttons->addWidget(browseButton);
    buttons->addStretch(1);
    buttons->addWidget(cancelButton);
    layout->addLayout(buttons);
    connect(browseButton, &QPushButton::clicked, this, &OpenDialog::browse);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    const int u = guideUnit(this);
    resize(u * 40, u * 28);
}

void OpenDialog::accepted(int row)
{
    if (row >= 0)
    {
        m_file = m_list->item(row)->data(Qt::UserRole).toString();
        accept();
    }
}

void OpenDialog::browse()
{
    // Not the native dialog: it ignores our sizes and is hard to use by touch.
    const QString file = QFileDialog::getOpenFileName(this, tr("Open"), m_startDir,
                                                      tr("Knitting projects and patterns (*.stitchly *.sm2d *.val);;All files (*)"),
                                                      nullptr, QFileDialog::DontUseNativeDialog);
    if (!file.isEmpty())
    {
        m_file = file;
        accept();
    }
}
