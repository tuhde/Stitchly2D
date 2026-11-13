// Drives the real main window: loads a sample pattern, edits bands and patterns, checks the instructions.
#include <QtTest>

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

#include <QLineEdit>
#include <QTabWidget>
#include <QTemporaryDir>

#include "../../app/stitchly2d/catalog_dialog.h"
#include "../../app/stitchly2d/core/application_2d.h"
#include "../../app/stitchly2d/mainwindow.h"

using namespace stitchcore;

class TstMainWindow : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void loadsPieces();
    void colorOverlayShowsInInstructions();
    void secondBandChangesStitch();
    void removeBandAndOverlay();
    void bandsArePerPiece();
    void badPatternReportsError();
    void dialogEditsBasics();
    void dialogNewColorPattern();
    void dialogRejectsBadInput();
    void dialogResizeKeepsCells();
    void dialogImportExport();
    void setCatalogCleansUp();
    void projectRoundTrip();
    void projectCatalogConflict();
    void projectErrorsKeepState();

private:
    void select(const QString &piece);
    template <typename T> T *find(const QString &name) { return window->findChild<T *>(name); }
    QScopedPointer<MainWindow> window;
};

void TstMainWindow::init()
{
    window.reset(new MainWindow);
    QString error;
    QVERIFY2(window->openPattern(QStringLiteral(TEST_DATA_DIR "/patterns/male_shirt.sm2d"), &error), qPrintable(error));
    window->show();
}

void TstMainWindow::select(const QString &piece)
{
    QListWidget *list = find<QListWidget>("piecesList");
    const auto found = list->findItems(piece, Qt::MatchExactly);
    QVERIFY(!found.isEmpty());
    list->setCurrentItem(found.first());
}

void TstMainWindow::loadsPieces()
{
    QCOMPARE(find<QListWidget>("piecesList")->count(), 17);
    QCOMPARE(find<QTableWidget>("bandTable")->rowCount(), 1);
    QCOMPARE(find<QTableWidget>("overlayTable")->rowCount(), 0);
    select("Cuff");
    QVERIFY(window->instructionsText().startsWith("Cast on 58 stitches."));
    QVERIFY(!window->instructionsText().contains(" x"));      // no overlay, no runs
}

void TstMainWindow::colorOverlayShowsInInstructions()
{
    select("Cuff");
    find<QPushButton>("addOverlayButton")->click();
    QTableWidget *overlays = find<QTableWidget>("overlayTable");
    QCOMPARE(overlays->rowCount(), 1);
    auto *pattern = qobject_cast<QComboBox *>(overlays->cellWidget(0, 0));
    pattern->setCurrentIndex(pattern->findData("Stripes 2 rows"));
    const QString text = window->instructionsText();
    QVERIFY2(text.contains("Row 1: A x58."), qPrintable(text));
    QVERIFY(text.contains("Row 3: B x58."));
    QVERIFY(text.contains("Row 5: A x58."));
    // limit the pattern to the first six rows
    qobject_cast<QSpinBox *>(overlays->cellWidget(0, 2))->setValue(6);
    const QString limited = window->instructionsText();
    QVERIFY(limited.contains("Row 5: B x58") || limited.contains("Row 5: A x58"));
    QVERIFY(limited.contains("Rows 7-17: work even."));
    if (qEnvironmentVariableIsSet("STITCHLY_TEST_SHOT"))
    {
        qobject_cast<QSpinBox *>(overlays->cellWidget(0, 2))->setValue(0);
        window->grab().save(qEnvironmentVariable("STITCHLY_TEST_SHOT"));
    }
}

void TstMainWindow::secondBandChangesStitch()
{
    select("Cuff");
    find<QPushButton>("addBandButton")->click();
    QTableWidget *bands = find<QTableWidget>("bandTable");
    QCOMPARE(bands->rowCount(), 2);
    // the first band gets a 20 mm 2:2 rib, the second keeps the default stitch up to the top
    qobject_cast<QComboBox *>(bands->cellWidget(0, 0))->setCurrentText("2:2 rib");
    qobject_cast<QDoubleSpinBox *>(bands->cellWidget(0, 1))->setValue(20);
    const QString text = window->instructionsText();
    QVERIFY2(text.contains("(2:2 rib)"), qPrintable(text));
    QVERIFY2(text.contains("change to Default stitch"), qPrintable(text));
    QVERIFY2(text.contains("and work in 2:2 rib."), qPrintable(text));
}

void TstMainWindow::removeBandAndOverlay()
{
    select("Cuff");
    find<QPushButton>("addBandButton")->click();
    QTableWidget *bands = find<QTableWidget>("bandTable");
    QCOMPARE(bands->rowCount(), 2);
    find<QPushButton>("removeBandButton")->click();
    QCOMPARE(bands->rowCount(), 1);
    QVERIFY(!find<QPushButton>("removeBandButton")->isEnabled());   // one band always stays
    find<QPushButton>("addOverlayButton")->click();
    QCOMPARE(find<QTableWidget>("overlayTable")->rowCount(), 1);
    find<QPushButton>("removeOverlayButton")->click();
    QCOMPARE(find<QTableWidget>("overlayTable")->rowCount(), 0);
    QVERIFY(!window->instructionsText().contains(" x"));
}

void TstMainWindow::bandsArePerPiece()
{
    select("Cuff");
    find<QPushButton>("addOverlayButton")->click();
    QVERIFY(window->instructionsText().contains(" x"));
    select("Pocket");
    QCOMPARE(find<QTableWidget>("overlayTable")->rowCount(), 0);
    QVERIFY(!window->instructionsText().contains(" x"));
    select("Cuff");
    QCOMPARE(find<QTableWidget>("overlayTable")->rowCount(), 1);     // kept when coming back
    QVERIFY(window->instructionsText().contains(" x"));
}

void TstMainWindow::badPatternReportsError()
{
    QString error;
    QVERIFY(!window->openPattern(QStringLiteral(TEST_DATA_DIR "/patterns/missing.sm2d"), &error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(find<QListWidget>("piecesList")->count(), 17);          // the open pattern stays
}

void TstMainWindow::dialogEditsBasics()
{
    CatalogDialog dialog(StitchCatalog::withDefaults());
    auto *table = dialog.findChild<QTableWidget *>("basicsTable");
    const int n = table->rowCount();
    QVERIFY(n >= 5);
    QCOMPARE(table->item(0, 0)->text(), QString("Stockinette"));
    qobject_cast<QDoubleSpinBox *>(table->cellWidget(0, 1))->setValue(4.25);
    dialog.findChild<QPushButton *>("addBasicButton")->click();
    QCOMPARE(table->rowCount(), n + 1);
    table->item(n, 0)->setText("Moss");
    qobject_cast<QDoubleSpinBox *>(table->cellWidget(n, 2))->setValue(2.75);
    table->setCurrentCell(2, 0);                                  // remove "1:1 rib"
    const QString removed = table->item(2, 0)->text();
    dialog.findChild<QPushButton *>("removeBasicButton")->click();
    QString error;
    QVERIFY2(dialog.apply(&error), qPrintable(error));
    const StitchCatalog c = dialog.catalog();
    QCOMPARE(c.basics().find("Stockinette").size.stitchWidthMm, 4.25);
    QCOMPARE(c.basics().find("Moss").size.rowHeightMm, 2.75);
    QVERIFY(!c.basics().contains(removed));

    // names must be filled in and unique
    table->item(0, 0)->setText("Moss");
    QVERIFY(!dialog.apply(&error));
    QVERIFY(error.contains("Moss"));
    table->item(0, 0)->setText("");
    QVERIFY(!dialog.apply(&error));
}

void TstMainWindow::dialogNewColorPattern()
{
    CatalogDialog dialog(StitchCatalog::withDefaults());
    dialog.findChild<QPushButton *>("newPatternButton")->click();
    dialog.findChild<QLineEdit *>("patternName")->setText("Mine");
    auto *kind = dialog.findChild<QComboBox *>("patternKind");
    kind->setCurrentIndex(kind->findData(int(PatternDef::Color)));
    auto *grid = dialog.findChild<QTableWidget *>("patternGrid");
    QCOMPARE(grid->columnCount(), 2);
    QCOMPARE(grid->rowCount(), 2);
    grid->item(0, 0)->setText("A");      // top row of the chart = last row of the repeat
    grid->item(0, 1)->setText("B");
    grid->item(1, 0)->setText("B");
    grid->item(1, 1)->setText("A");
    QString error;
    QVERIFY2(dialog.apply(&error), qPrintable(error));
    const PatternDef p = dialog.catalog().find("Mine");
    QCOMPARE(int(p.kind), int(PatternDef::Color));
    QCOMPARE(p.legend, QStringList({"A", "B"}));
    QCOMPARE(p.cells, QVector<int>({1, 0, 0, 1}));     // bottom row first: B A / A B
    QVERIFY(p.isValid());
    if (qEnvironmentVariableIsSet("STITCHLY_TEST_DIALOG_SHOT"))
    {
        dialog.findChild<QSpinBox *>("patternWidth")->setValue(4);
        dialog.findChild<QSpinBox *>("patternHeight")->setValue(3);
        for (int r = 0; r < grid->rowCount(); ++r)
            for (int c = 0; c < grid->columnCount(); ++c)
                grid->item(r, c)->setText((r + c) % 3 == 0 ? "MC" : ((r + c) % 3 == 1 ? "A" : "#c03030"));
        dialog.findChild<QTabWidget *>()->setCurrentIndex(1);
        dialog.show();
        dialog.grab().save(qEnvironmentVariable("STITCHLY_TEST_DIALOG_SHOT"));
        grid->item(1, 1)->setText("A");
        grid->item(0, 0)->setText("A");
        dialog.findChild<QSpinBox *>("patternWidth")->setValue(2);
        dialog.findChild<QSpinBox *>("patternHeight")->setValue(2);
        grid->item(0, 0)->setText("A");
        grid->item(0, 1)->setText("B");
        grid->item(1, 0)->setText("B");
        grid->item(1, 1)->setText("A");
    }
    // an empty cell leaves the base stitch alone
    grid->item(1, 1)->setText("");
    QVERIFY(dialog.apply(&error));
    QCOMPARE(dialog.catalog().find("Mine").cells.at(1), -1);
}

void TstMainWindow::dialogRejectsBadInput()
{
    CatalogDialog dialog(StitchCatalog::withDefaults());
    dialog.findChild<QPushButton *>("newPatternButton")->click();
    auto *name = dialog.findChild<QLineEdit *>("patternName");
    auto *grid = dialog.findChild<QTableWidget *>("patternGrid");
    QString error;
    name->setText("");
    QVERIFY(!dialog.apply(&error));
    QVERIFY(error.contains("no name"));
    name->setText("Seed stitch");                                  // already in the catalog
    QVERIFY(!dialog.apply(&error));
    QVERIFY(error.contains("Seed stitch"));
    name->setText("Empty");
    for (int r = 0; r < grid->rowCount(); ++r)
        for (int c = 0; c < grid->columnCount(); ++c)
            grid->item(r, c)->setText("");
    QVERIFY(!dialog.apply(&error));                                // nothing in the chart
    grid->item(0, 0)->setText("k");
    QVERIFY(dialog.apply(&error));
}

void TstMainWindow::dialogResizeKeepsCells()
{
    CatalogDialog dialog(StitchCatalog::withDefaults());
    auto *list = dialog.findChild<QListWidget *>("patternList");
    const auto found = list->findItems("Seed stitch (special)", Qt::MatchExactly);
    QVERIFY(!found.isEmpty());
    list->setCurrentItem(found.first());
    auto *grid = dialog.findChild<QTableWidget *>("patternGrid");
    QCOMPARE(grid->columnCount(), 2);
    // seed stitch: row 1 (bottom) = k p, row 2 (top) = p k
    QCOMPARE(grid->item(1, 0)->text(), QString("k"));
    QCOMPARE(grid->item(0, 0)->text(), QString("p"));
    dialog.findChild<QSpinBox *>("patternWidth")->setValue(4);
    dialog.findChild<QSpinBox *>("patternHeight")->setValue(3);
    QCOMPARE(grid->columnCount(), 4);
    QCOMPARE(grid->rowCount(), 3);
    QCOMPARE(grid->item(2, 0)->text(), QString("k"));              // the bottom row stayed at the bottom
    QCOMPARE(grid->item(2, 1)->text(), QString("p"));
    QCOMPARE(grid->item(2, 3)->text(), QString(""));               // new cells are empty
    QCOMPARE(grid->item(0, 0)->text(), QString(""));               // and the new row is on top
    QString error;
    QVERIFY2(dialog.apply(&error), qPrintable(error));
    const PatternDef p = dialog.catalog().find("Seed stitch");
    QCOMPARE(p.width, 4);
    QCOMPARE(p.height, 3);
    QCOMPARE(p.legend.at(p.cells.at(0)), QString("k"));            // k at the bottom left
}

void TstMainWindow::dialogImportExport()
{
    QTemporaryDir dir;
    const QString file = dir.filePath("one.stpat");
    CatalogDialog dialog(StitchCatalog::withDefaults());
    QString error;
    QVERIFY2(dialog.exportPatterns(file, {"Seed stitch", "Checks 2x2"}, &error), qPrintable(error));
    QVERIFY(!dialog.exportPatterns(dir.filePath("x.stpat"), {"nope"}, &error));

    ImportResult res;
    QVERIFY2(dialog.importFile(file, ConflictPolicy::Skip, &res, &error), qPrintable(error));
    QCOMPARE(res.skipped, 2);
    QVERIFY2(dialog.importFile(file, ConflictPolicy::Rename, &res, &error), qPrintable(error));
    QCOMPARE(res.renamed, 2);
    QVERIFY(dialog.apply(&error));
    QVERIFY(dialog.catalog().contains("Seed stitch (2)"));
    QVERIFY2(dialog.importFile(file, ConflictPolicy::Replace, &res, &error), qPrintable(error));
    QCOMPARE(res.replaced, 2);

    // into an empty catalog: everything is added
    StitchCatalog empty;
    CatalogDialog fresh(empty);
    QVERIFY2(fresh.importFile(file, ConflictPolicy::Skip, &res, &error), qPrintable(error));
    QCOMPARE(res.added, 2);
    QVERIFY(fresh.apply(&error));
    QCOMPARE(fresh.catalog().find("Checks 2x2").cells, StitchCatalog::withDefaults().find("Checks 2x2").cells);

    QVERIFY(!fresh.importFile(dir.filePath("missing.stpat"), ConflictPolicy::Skip, &res, &error));
    QVERIFY(!error.isEmpty());
}

void TstMainWindow::setCatalogCleansUp()
{
    select("Cuff");
    // a rib band with a stripe pattern
    find<QPushButton>("addBandButton")->click();
    QTableWidget *bands = find<QTableWidget>("bandTable");
    qobject_cast<QComboBox *>(bands->cellWidget(0, 0))->setCurrentText("2:2 rib");
    qobject_cast<QDoubleSpinBox *>(bands->cellWidget(0, 1))->setValue(20);
    bands->selectRow(0);
    find<QPushButton>("addOverlayButton")->click();
    QVERIFY(window->instructionsText().contains("(2:2 rib)"));
    QVERIFY(window->instructionsText().contains(" x"));

    StitchCatalog c = window->catalog();
    QVERIFY(c.basics().remove("2:2 rib"));
    QVERIFY(c.remove("Seed stitch"));           // "Add pattern" put the first pattern of the catalog on the band
    window->setCatalog(c);

    QCOMPARE(qobject_cast<QComboBox *>(find<QTableWidget>("bandTable")->cellWidget(0, 0))->currentText(),
             QString("Default stitch"));                       // fell back
    QCOMPARE(find<QTableWidget>("overlayTable")->rowCount(), 0);   // pattern taken off
    const QString text = window->instructionsText();
    QVERIFY2(!text.contains("2:2 rib"), qPrintable(text));
    QVERIFY(!text.contains(" x"));

    // changing the size of a basic stitch changes the result: 290 mm wide cuff, 4 mm stitches
    c.basics().set({"Stockinette", StitchSize{4.0, 3.5}});
    window->setCatalog(c);
    QVERIFY2(window->instructionsText().contains("Cast on 73"), qPrintable(window->instructionsText()));
}

void TstMainWindow::projectRoundTrip()
{
    // settings
    find<QComboBox>("stitchTypeCombo")->setCurrentText("Garter");
    find<QCheckBox>("allowanceCheck")->setChecked(true);
    find<QSpinBox>("edgeStitchesSpin")->setValue(3);
    auto *increase = find<QComboBox>("increaseCombo");
    increase->setCurrentIndex(increase->findData(int(IncreaseMethod::KFB)));
    // bands and patterns on two pieces
    select("Cuff");
    find<QPushButton>("addBandButton")->click();
    QTableWidget *bands = find<QTableWidget>("bandTable");
    qobject_cast<QComboBox *>(bands->cellWidget(0, 0))->setCurrentText("2:2 rib");
    qobject_cast<QDoubleSpinBox *>(bands->cellWidget(0, 1))->setValue(20);
    bands->selectRow(1);
    find<QPushButton>("addOverlayButton")->click();
    auto *pattern = qobject_cast<QComboBox *>(find<QTableWidget>("overlayTable")->cellWidget(0, 0));
    pattern->setCurrentIndex(pattern->findData("Stripes 2 rows"));
    select("Pocket");
    find<QPushButton>("addOverlayButton")->click();
    select("Cuff");
    const QString cuff = window->instructionsText();
    select("Pocket");
    const QString pocket = window->instructionsText();

    QTemporaryDir dir;
    const QString file = dir.filePath("work.stitchly");
    QString error;
    QVERIFY2(window->saveProject(file, &error), qPrintable(error));
    QCOMPARE(window->projectFile(), file);

    MainWindow other;
    QVERIFY2(other.openProject(file, &error, MainWindow::CatalogConflict::UseProject), qPrintable(error));
    QCOMPARE(other.projectFile(), file);
    QCOMPARE(other.findChild<QListWidget *>("piecesList")->count(), 17);
    QCOMPARE(other.findChild<QComboBox *>("stitchTypeCombo")->currentText(), QString("Garter"));
    QVERIFY(other.findChild<QCheckBox *>("allowanceCheck")->isChecked());
    QCOMPARE(other.findChild<QSpinBox *>("edgeStitchesSpin")->value(), 3);
    QCOMPARE(other.findChild<QComboBox *>("increaseCombo")->currentData().toInt(), int(IncreaseMethod::KFB));
    auto select2 = [&other](const QString &piece) {
        QListWidget *list = other.findChild<QListWidget *>("piecesList");
        list->setCurrentItem(list->findItems(piece, Qt::MatchExactly).first());
    };
    select2("Cuff");
    QCOMPARE(other.instructionsText(), cuff);
    QCOMPARE(other.findChild<QTableWidget *>("bandTable")->rowCount(), 2);
    select2("Pocket");
    QCOMPARE(other.instructionsText(), pocket);

    // saving again over the same file works, and a new pattern starts a new project
    QVERIFY2(other.saveProject(file, &error), qPrintable(error));
    QVERIFY(other.openPattern(QStringLiteral(TEST_DATA_DIR "/patterns/male_shirt.sm2d"), &error));
    QVERIFY(other.projectFile().isEmpty());
    select2("Cuff");
    QCOMPARE(other.findChild<QTableWidget *>("bandTable")->rowCount(), 1);
    QVERIFY(!other.instructionsText().contains(" x"));
}

void TstMainWindow::projectCatalogConflict()
{
    select("Cuff");
    find<QPushButton>("addBandButton")->click();
    QTableWidget *bands = find<QTableWidget>("bandTable");
    qobject_cast<QComboBox *>(bands->cellWidget(0, 0))->setCurrentText("2:2 rib");
    qobject_cast<QDoubleSpinBox *>(bands->cellWidget(0, 1))->setValue(20);
    QTemporaryDir dir;
    const QString file = dir.filePath("c.stitchly");
    QString error;
    QVERIFY2(window->saveProject(file, &error), qPrintable(error));

    // Another machine: same name, other size. And a catalog that lacks the stitch altogether.
    StitchCatalog changed = StitchCatalog::withDefaults();
    changed.basics().set({"2:2 rib", StitchSize{2.0, 2.0}});
    StitchCatalog lacking = StitchCatalog::withDefaults();
    lacking.basics().remove("2:2 rib");

    MainWindow keep;
    keep.setCatalog(changed);
    QVERIFY2(keep.openProject(file, &error, MainWindow::CatalogConflict::KeepMine), qPrintable(error));
    QCOMPARE(keep.catalog().basics().find("2:2 rib").size.stitchWidthMm, 2.0);       // mine stays

    MainWindow use;
    use.setCatalog(changed);
    QVERIFY2(use.openProject(file, &error, MainWindow::CatalogConflict::UseProject), qPrintable(error));
    QCOMPARE(use.catalog().basics().find("2:2 rib").size.stitchWidthMm, 3.5);        // the project's

    MainWindow missing;
    missing.setCatalog(lacking);
    QVERIFY2(missing.openProject(file, &error, MainWindow::CatalogConflict::KeepMine), qPrintable(error));
    QVERIFY(missing.catalog().basics().contains("2:2 rib"));                          // added, nothing to lose
    QListWidget *list = missing.findChild<QListWidget *>("piecesList");
    list->setCurrentItem(list->findItems("Cuff", Qt::MatchExactly).first());
    QVERIFY(missing.instructionsText().contains("(2:2 rib)"));
}

void TstMainWindow::projectErrorsKeepState()
{
    select("Cuff");
    find<QPushButton>("addOverlayButton")->click();
    const QString before = window->instructionsText();

    QTemporaryDir dir;
    Project p;
    p.patternFile = dir.filePath("gone.sm2d");
    p.settings.size = StitchSize{5.0, 3.5};
    QString error;
    const QString file = dir.filePath("bad.stitchly");
    QVERIFY(stitchcore::saveProject(file, p, &error));
    QVERIFY(!window->openProject(file, &error, MainWindow::CatalogConflict::UseProject));
    QVERIFY2(error.contains("gone.sm2d"), qPrintable(error));
    QCOMPARE(window->instructionsText(), before);                    // the open pattern and its bands stay
    QCOMPARE(find<QListWidget>("piecesList")->count(), 17);

    QVERIFY(!window->openProject(dir.filePath("missing.stitchly"), &error));
    QVERIFY(!error.isEmpty());

    MainWindow empty;
    QVERIFY(!empty.saveProject(dir.filePath("none.stitchly"), &error));      // nothing to save without a pattern
    QVERIFY(error.contains("pattern"));
    QVERIFY(!QFile::exists(dir.filePath("none.stitchly")));
}

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(schema);
    Q_INIT_RESOURCE(icon);
    Q_INIT_RESOURCE(theme);
    Q_INIT_RESOURCE(flags);
    Q_INIT_RESOURCE(sounds);
    Q_INIT_RESOURCE(diagrams);
    qputenv("QT_QPA_PLATFORM", "offscreen");
    Application2D app(argc, argv);
    TstMainWindow test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_mainwindow.moc"
