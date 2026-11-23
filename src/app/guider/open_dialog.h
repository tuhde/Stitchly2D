// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef OPEN_DIALOG_H
#define OPEN_DIALOG_H

#include <QDialog>
#include <QString>
#include <QStringList>

class QListWidget;

/**
 * Touch-friendly "open": the recently used files as big list entries, plus "Browse..." for the file dialog.
 */
class OpenDialog : public QDialog
{
    Q_OBJECT
public:
    OpenDialog(const QStringList &recent, const QString &startDir, QWidget *parent = nullptr);
    QString chosenFile() const { return m_file; }

private slots:
    void browse();
    void accepted(int row);

private:
    QListWidget *m_list = nullptr;
    QString      m_startDir;
    QString      m_file;
};

#endif
