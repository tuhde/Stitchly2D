// Drives the yarn program's window on the offscreen platform: load, edit, save, and the protection of the file.
#include <QtTest>

#include <QDoubleSpinBox>
#include <QFile>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>

#include "../../app/yarnbox/yarnbox_window.h"
#include "../../libs/yarnui/yarn_inventory_widget.h"

using namespace stitchcore;

namespace
{
Yarn makeYarn(const QString &name)
{
    Yarn y;
    y.id = Yarn::newId();
    y.brand = "Woolly";
    y.name = name;
    y.metersPerSkein = 200;
    y.skeinsInStock = 2;
    return y;
}
}

class TstYarnbox : public QObject
{
    Q_OBJECT
private slots:
    void startsEmptyWithoutAFile();
    void editAndSave();
    void reloadAndUnsavedChanges();
    void changedFileIsNotOverwrittenSilently();
    void unreadableFileIsNotOverwritten();
    void invalidYarnIsNotSaved();
};

void TstYarnbox::startsEmptyWithoutAFile()
{
    QTemporaryDir dir;
    YarnboxWindow w(dir.filePath("sub/yarns.json"), {"Stockinette"});
    QString error;
    QVERIFY2(w.load(&error), qPrintable(error));
    QCOMPARE(w.inventory().size(), 0);
    QVERIFY(!w.isModified());
}

void TstYarnbox::editAndSave()
{
    QTemporaryDir dir;
    const QString file = dir.filePath("config/yarns.json");            // the folder does not exist yet
    YarnboxWindow w(file, {"Stockinette", "Garter"});
    QVERIFY(w.load());
    w.editor()->findChild<QPushButton *>("addYarnButton")->click();
    QVERIFY(w.isModified());
    QVERIFY(w.windowTitle().contains("yarns.json"));
    auto *name = w.editor()->findChild<QLineEdit *>("yarnName");
    name->setText(QString());
    QTest::keyClicks(name, "Soft");
    w.editor()->findChild<QDoubleSpinBox *>("yarnMeters")->setValue(150);
    QString error;
    QVERIFY2(w.save(&error), qPrintable(error));
    QVERIFY(!w.isModified());
    YarnInventory saved;
    QVERIFY2(YarnInventory::load(file, saved, &error), qPrintable(error));
    QCOMPARE(saved.size(), 1);
    QCOMPARE(saved.yarns().first().name, QString("Soft"));
    QCOMPARE(saved.yarns().first().metersPerSkein, 150.0);
    // a second window on the file sees it
    YarnboxWindow other(file, {"Stockinette"});
    QVERIFY(other.load());
    QCOMPARE(other.inventory().size(), 1);
}

void TstYarnbox::reloadAndUnsavedChanges()
{
    QTemporaryDir dir;
    const QString file = dir.filePath("yarns.json");
    YarnInventory inv;
    inv.set(makeYarn("One"));
    QVERIFY(inv.save(file));
    YarnboxWindow w(file, {"Stockinette"});
    QVERIFY(w.load());
    QCOMPARE(w.inventory().size(), 1);
    w.editor()->findChild<QPushButton *>("addYarnButton")->click();
    QCOMPARE(w.inventory().size(), 2);
    // unsaved changes: cancel keeps them, discard drops them
    YarnboxWindow::SaveChoice choice = YarnboxWindow::SaveChoice::Cancel;
    int asked = 0;
    w.setSavePrompt([&]() { ++asked; return choice; });
    w.reload();
    QCOMPARE(asked, 1);
    QCOMPARE(w.inventory().size(), 2);
    QVERIFY(w.isModified());
    choice = YarnboxWindow::SaveChoice::Discard;
    w.reload();
    QCOMPARE(w.inventory().size(), 1);
    QVERIFY(!w.isModified());
    // closing with changes: save writes the file
    w.editor()->findChild<QPushButton *>("addYarnButton")->click();
    choice = YarnboxWindow::SaveChoice::Save;
    QVERIFY(w.maybeSave());
    QVERIFY(!w.isModified());
    YarnInventory saved;
    QVERIFY(YarnInventory::load(file, saved));
    QCOMPARE(saved.size(), 2);
    // nothing changed: nothing is asked
    asked = 0;
    QVERIFY(w.maybeSave());
    QCOMPARE(asked, 0);
}

void TstYarnbox::changedFileIsNotOverwrittenSilently()
{
    QTemporaryDir dir;
    const QString file = dir.filePath("yarns.json");
    YarnInventory inv;
    const QString id = inv.set(makeYarn("Stock"));
    QVERIFY(inv.save(file));
    YarnboxWindow w(file, {"Stockinette"});
    QVERIFY(w.load());
    // the guider takes yarn from the stock meanwhile
    QTest::qSleep(1100);                                        // file times may have a resolution of a second
    Yarn *y = inv.find(id);
    QVERIFY(consumeYarn(*y, 300));
    QVERIFY(inv.save(file));
    w.editor()->findChild<QPushButton *>("addYarnButton")->click();
    bool overwrite = false;
    int asked = 0;
    w.setOverwritePrompt([&](const QString &) { ++asked; return overwrite; });
    QString error;
    QVERIFY(!w.save(&error));
    QCOMPARE(asked, 1);
    QVERIFY(w.isModified());
    YarnInventory onDisk;
    QVERIFY(YarnInventory::load(file, onDisk));
    QCOMPARE(onDisk.size(), 1);                                 // untouched
    QCOMPARE(onDisk.find(id)->stockMeters(), 100.0);
    overwrite = true;
    QVERIFY(w.save(&error));
    QVERIFY(YarnInventory::load(file, onDisk));
    QCOMPARE(onDisk.size(), 2);
    // after saving, the next save does not ask again
    QVERIFY(w.save());
    QCOMPARE(asked, 2);
}

void TstYarnbox::unreadableFileIsNotOverwritten()
{
    QTemporaryDir dir;
    const QString file = dir.filePath("yarns.json");
    QFile f(file);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("{ not valid");
    f.close();
    YarnboxWindow w(file, {"Stockinette"});
    QString error;
    QVERIFY(!w.load(&error));
    QVERIFY(!error.isEmpty());
    w.editor()->findChild<QPushButton *>("addYarnButton")->click();
    QVERIFY(!w.save(&error));
    QVERIFY(error.contains("not overwritten"));
    QFile again(file);
    QVERIFY(again.open(QIODevice::ReadOnly));
    QCOMPARE(again.readAll(), QByteArray("{ not valid"));
}

void TstYarnbox::invalidYarnIsNotSaved()
{
    QTemporaryDir dir;
    const QString file = dir.filePath("yarns.json");
    YarnboxWindow w(file, {"Stockinette"});
    QVERIFY(w.load());
    w.editor()->findChild<QPushButton *>("addYarnButton")->click();
    auto *name = w.editor()->findChild<QLineEdit *>("yarnName");
    name->setText("x");
    QTest::keyClick(name, Qt::Key_Backspace);                   // neither brand nor name
    QString error;
    QVERIFY(!w.save(&error));
    QVERIFY(!error.isEmpty());
    QVERIFY(!QFile::exists(file));
}

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    TstYarnbox test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_yarnbox.moc"
