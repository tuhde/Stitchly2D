// yarnbox - the yarn inventory of Stitchly2D on its own. GPLv3, see LICENSE.
#ifndef YARNBOX_WINDOW_H
#define YARNBOX_WINDOW_H

#include <QDateTime>
#include <QMainWindow>
#include <QStringList>

#include <functional>

#include "yarn.h"

namespace yarnui { class YarnInventoryWidget; }

/**
 * Window of the yarn program: the inventory editor on the file yarns.json, with Save, Reload and a question
 * before unsaved changes are lost. Saving checks that the file was not changed meanwhile (the guider takes used
 * yarn from the stock while this window may be open) and asks before overwriting such a change.
 */
class YarnboxWindow : public QMainWindow
{
    Q_OBJECT
public:
    /** stitchTypes: the basic stitches a gauge can be entered for. */
    YarnboxWindow(const QString &yarnFile, const QStringList &stitchTypes, QWidget *parent = nullptr);

    /** Reads the file (a missing one is an empty inventory). On failure the window shows an empty inventory and saving is refused. */
    bool load(QString *error = nullptr);
    /** Writes the inventory; asks first if the file changed since it was read. */
    bool save(QString *error = nullptr);

    bool isModified() const { return m_modified; }
    const stitchcore::YarnInventory &inventory() const;
    yarnui::YarnInventoryWidget *editor() const { return m_editor; }
    QString yarnFile() const { return m_file; }

    enum class SaveChoice { Save, Discard, Cancel };
    using SavePrompt = std::function<SaveChoice()>;
    using OverwritePrompt = std::function<bool(const QString &text)>;
    /** Replace the question for unsaved changes / for overwriting a file that changed (the tests use these). */
    void setSavePrompt(const SavePrompt &prompt) { m_savePrompt = prompt; }
    void setOverwritePrompt(const OverwritePrompt &prompt) { m_overwritePrompt = prompt; }

    /** Asks what to do with unsaved changes. False means "cancel". */
    bool maybeSave();

public slots:
    void saveAction();
    void reload();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void createEditor(const stitchcore::YarnInventory &inventory);
    void setModified(bool modified);

    QString              m_file;
    QStringList          m_stitchTypes;
    yarnui::YarnInventoryWidget *m_editor = nullptr;
    bool                 m_modified = false;
    bool                 m_loadFailed = false;      //!< the file could not be read: it is not overwritten
    QDateTime            m_fileTime;                //!< modification time of the file when it was read or written
    SavePrompt           m_savePrompt;
    OverwritePrompt      m_overwritePrompt;
};

#endif
