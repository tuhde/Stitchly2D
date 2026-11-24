// Drives the real main window: loads a sample pattern, edits bands and patterns, checks the instructions.
#include <QtTest>
#include <QRegularExpression>

#include <QCheckBox>
#include <QDockWidget>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

#include <QLineEdit>
#include <QAction>
#include <QTabWidget>
#include <QTemporaryDir>

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include "../../app/stitchly2d/mcp/mcp_integration.h"
#include "../../libs/vmcp/mcptoolregistry.h"
#include "../../app/stitchly2d/catalog_dialog.h"
#include "../../libs/yarnui/yarn_inventory_widget.h"
#include "../../libs/stitchcore/yarn_estimate.h"
#include "../../libs/stitchload/core/application_2d.h"
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
    void toolsLookAndSettings();
    void toolsCatalog();
    void toolsBands();
    void toolsFiles();
    void toolsWithoutPattern();
    void toolsOverHttp();
    void undoSettings();
    void undoMergesTyping();
    void undoBandsAndCatalog();
    void modifiedAndSaved();
    void openingResetsHistory();
    void unsavedChangesQuestion();
    void closingAsksFirst();
    void undoByClaude();
    void bulkFromSetting();
    void shapeOnSetting();
    void methodDefaultAndPiece();
    void methodMachineNeedles();
    void methodInProjects();
    void methodTools();
    void yarnsChosenAndReported();
    void yarnsInProjects();
    void yarnGaugeSetsStitchSizes();
    void yarnPanel();
    void yarnInventoryEditor();
    void toolsYarns();

private:
    McpToolResult call(const QString &tool, const QJsonObject &args = QJsonObject())
    {
        return window->mcp()->callTool(tool, args);
    }
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
        if (QDockWidget *settings = window->findChild<QDockWidget *>("settingsDock")) settings->raise();
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

void TstMainWindow::toolsLookAndSettings()
{
    McpToolResult r = call("get_project_info");
    QVERIFY(!r.isError);
    const QJsonObject info = r.structured.toObject();
    QCOMPARE(info["pieces"].toArray().size(), 17);
    QCOMPARE(info["settings"].toObject()["stitchType"].toString(), QString("Stockinette"));
    QVERIFY(info["basicStitches"].toArray().contains("Patent"));
    QVERIFY(info["patterns"].toArray().contains("Seed stitch"));

    r = call("list_pieces");
    const QJsonArray pieces = r.structured.toObject()["pieces"].toArray();
    QCOMPARE(pieces.size(), 17);
    bool foundCuff = false;
    for (const QJsonValue &v : pieces)
    {
        if (v.toObject()["name"].toString() == "Cuff")
        {
            foundCuff = true;
            QCOMPARE(v.toObject()["castOn"].toInt(), 58);
            QCOMPARE(v.toObject()["rows"].toInt(), 17);
        }
    }
    QVERIFY(foundCuff);

    // settings: a catalog stitch takes its size, a typed size makes it custom
    r = call("set_stitch_settings", {{"stitchType", "Garter"}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QCOMPARE(r.structured.toObject()["rowHeightMm"].toDouble(), 2.5);
    r = call("set_stitch_settings", {{"stitchWidthMm", 4.0}, {"includeSeamAllowance", true}, {"edgeStitches", 2},
                                     {"increase", "kfb"}, {"decrease", "k2tog"}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QCOMPARE(r.structured.toObject()["stitchWidthMm"].toDouble(), 4.0);
    QCOMPARE(find<QComboBox>("stitchTypeCombo")->currentText(), QString("Custom"));
    QVERIFY(find<QCheckBox>("allowanceCheck")->isChecked());
    QCOMPARE(find<QSpinBox>("edgeStitchesSpin")->value(), 2);
    QCOMPARE(window->settings().shaping.increase, IncreaseMethod::KFB);
    QVERIFY(window->settings().shaping.decrease == DecreaseMethod::K2tog);
    // bad input is refused and changes nothing
    QVERIFY(call("set_stitch_settings", {{"stitchWidthMm", 0}}).isError);
    QVERIFY(call("set_stitch_settings", {{"stitchWidthMm", 500}}).isError);
    QVERIFY(call("set_stitch_settings", {{"stitchType", "Nonexistent"}}).isError);
    QVERIFY(call("set_stitch_settings", {{"increase", "bogus"}}).isError);
    QVERIFY(call("set_stitch_settings", {{"edgeStitches", 11}}).isError);
    QCOMPARE(window->settings().size.stitchWidthMm, 4.0);

    // selecting and reading a piece
    QVERIFY(call("select_piece", {{"piece", "nope"}}).isError);
    QVERIFY(!call("select_piece", {{"piece", "Cuff"}}).isError);
    QCOMPARE(window->currentPiece(), QString("Cuff"));
    r = call("get_instructions");
    QVERIFY(r.text.startsWith("Cast on"));
    QCOMPARE(call("get_instructions", {{"piece", "Cuff"}}).text, r.text);
    QVERIFY(call("get_instructions", {{"piece", "nope"}}).isError);
    QVERIFY(call("get_row_table", {{"piece", "Cuff"}}).text.startsWith("row;band;stitch type"));

    // the picture
    r = call("screenshot");
    QVERIFY(!r.isError);
    QVERIFY(r.imagePng.startsWith("\x89PNG"));
}

void TstMainWindow::toolsCatalog()
{
    QJsonObject catalog = call("get_catalog").structured.toObject();
    QVERIFY(catalog["basicStitches"].toArray().size() >= 5);
    // a chart is listed top row first, like a knitting chart
    QJsonObject seed;
    for (const QJsonValue &v : catalog["patterns"].toArray())
        if (v.toObject()["name"].toString() == "Seed stitch") seed = v.toObject();
    QCOMPARE(seed["rows"].toArray().at(0).toArray().at(0).toString(), QString("p"));

    McpToolResult r = call("set_pattern", {{"name", "Zigzag"}, {"kind", "special"},
                                           {"rows", QJsonArray{QJsonArray{"k", "p"}, QJsonArray{"p", "k"}}}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QVERIFY(window->catalog().contains("Zigzag"));
    QCOMPARE(window->catalog().find("Zigzag").toChart(), QVector<QStringList>({{"k", "p"}, {"p", "k"}}));
    // the pattern is then in the band editor's list
    select("Cuff");
    find<QPushButton>("addOverlayButton")->click();
    auto *combo = qobject_cast<QComboBox *>(find<QTableWidget>("overlayTable")->cellWidget(0, 0));
    QVERIFY(combo->findData("Zigzag") >= 0);
    combo->setCurrentIndex(combo->findData("Zigzag"));                // use it, so that deleting it matters below
    QVERIFY(window->instructionsText().contains("k x1, p x1"));
    // replacing keeps one entry
    QVERIFY(!call("set_pattern", {{"name", "Zigzag"}, {"kind", "color"}, {"rows", QJsonArray{QJsonArray{"A", "B"}}}}).isError);
    QCOMPARE(window->catalog().find("Zigzag").width, 2);
    QCOMPARE(int(window->catalog().find("Zigzag").kind), int(PatternDef::Color));

    QVERIFY(call("set_pattern", {{"name", "X"}, {"kind", "weird"}, {"rows", QJsonArray{QJsonArray{"k"}}}}).isError);
    QVERIFY(call("set_pattern", {{"name", "X"}, {"kind", "special"},
                                 {"rows", QJsonArray{QJsonArray{"k", "p"}, QJsonArray{"k"}}}}).isError);      // ragged
    QVERIFY(call("set_pattern", {{"name", "X"}, {"kind", "special"}, {"rows", QJsonArray{QJsonArray{"", ""}}}}).isError);
    QVERIFY(call("set_pattern", {{"name", ""}, {"kind", "special"}, {"rows", QJsonArray{QJsonArray{"k"}}}}).isError);
    QVERIFY(!window->catalog().contains("X"));

    // basic stitches
    QVERIFY(!call("set_basic_stitch", {{"name", "Moss"}, {"stitchWidthMm", 4.5}, {"rowHeightMm", 3.0}}).isError);
    QCOMPARE(window->catalog().basics().find("Moss").size.stitchWidthMm, 4.5);
    QVERIFY(call("set_basic_stitch", {{"name", "Default stitch"}, {"stitchWidthMm", 4}, {"rowHeightMm", 3}}).isError);
    QVERIFY(call("set_basic_stitch", {{"name", "Bad"}, {"stitchWidthMm", 0}, {"rowHeightMm", 3}}).isError);
    QVERIFY(call("set_basic_stitch", {{"name", ""}, {"stitchWidthMm", 4}, {"rowHeightMm", 3}}).isError);
    // deleting takes it off the bands
    select("Cuff");
    QVERIFY(!call("delete_pattern", {{"name", "Zigzag"}}).isError);
    QVERIFY(!window->catalog().contains("Zigzag"));
    QCOMPARE(find<QTableWidget>("overlayTable")->rowCount(), 0);
    QVERIFY(call("delete_pattern", {{"name", "Zigzag"}}).isError);
    QVERIFY(!call("delete_basic_stitch", {{"name", "Moss"}}).isError);
    QVERIFY(call("delete_basic_stitch", {{"name", "Moss"}}).isError);
}

void TstMainWindow::toolsBands()
{
    const QJsonArray bands{
        QJsonObject{{"stitchType", "2:2 rib"}, {"heightMm", 20}},
        QJsonObject{{"stitchType", "Default stitch"},
                    {"patterns", QJsonArray{QJsonObject{{"pattern", "Stripes 2 rows"}, {"rowCount", 6}}}}}};
    McpToolResult r = call("set_bands", {{"piece", "Cuff"}, {"bands", bands}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QCOMPARE(window->currentPiece(), QString("Cuff"));                // shown to the user
    QCOMPARE(find<QTableWidget>("bandTable")->rowCount(), 2);
    const QString text = window->instructionsText();
    QVERIFY2(text.contains("(2:2 rib)"), qPrintable(text));
    QVERIFY2(text.contains("change to Default stitch"), qPrintable(text));
    QCOMPARE(call("get_instructions", {{"piece", "Cuff"}}).text, text);

    r = call("get_bands", {{"piece", "Cuff"}});
    const QJsonArray back = r.structured.toObject()["bands"].toArray();
    QCOMPARE(back.size(), 2);
    QCOMPARE(back.at(0).toObject()["heightMm"].toDouble(), 20.0);
    QCOMPARE(back.at(1).toObject()["patterns"].toArray().at(0).toObject()["rowCount"].toInt(), 6);

    // the same bands again after the change is idempotent; bad input is refused and changes nothing
    const QString before = window->instructionsText();
    QVERIFY(call("set_bands", {{"piece", "Cuff"}, {"bands", QJsonArray()}}).isError);
    QVERIFY(call("set_bands", {{"piece", "Cuff"}, {"bands", QJsonArray{QJsonObject{{"stitchType", "Nope"}}}}}).isError);
    QVERIFY(call("set_bands", {{"piece", "Cuff"},
                               {"bands", QJsonArray{QJsonObject{{"stitchType", "Default stitch"},
                                                                {"patterns", QJsonArray{QJsonObject{{"pattern", "Nope"}}}}}}}}).isError);
    QVERIFY(call("set_bands", {{"piece", "Cuff"},      // a band that is not the last needs a height
                               {"bands", QJsonArray{QJsonObject{{"stitchType", "Garter"}},
                                                    QJsonObject{{"stitchType", "Default stitch"}}}}}).isError);
    QVERIFY(call("set_bands", {{"piece", "Nope"}, {"bands", bands}}).isError);
    QVERIFY(call("set_bands", {{"piece", "Cuff"},
                               {"bands", QJsonArray{QJsonObject{{"stitchType", "Default stitch"},
                                                                {"patterns", QJsonArray{QJsonObject{{"pattern", "Seed stitch"}, {"firstRow", -1}}}}}}}}).isError);
    QCOMPARE(window->instructionsText(), before);

    // a single band with the last one's height ignored
    r = call("set_bands", {{"piece", "Cuff"}, {"bands", QJsonArray{QJsonObject{{"stitchType", "Garter"}, {"heightMm", 15}}}}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QCOMPARE(window->bands("Cuff").first().heightMm, 0.0);
    QCOMPARE(find<QTableWidget>("bandTable")->rowCount(), 1);
}

void TstMainWindow::toolsFiles()
{
    QTemporaryDir dir;
    QVERIFY(!call("set_bands", {{"piece", "Cuff"},
                                {"bands", QJsonArray{QJsonObject{{"stitchType", "Default stitch"},
                                                                 {"patterns", QJsonArray{QJsonObject{{"pattern", "Stripes 2 rows"}}}}}}}}).isError);
    const QString project = dir.filePath("t.stitchly");
    McpToolResult r = call("save_project", {{"path", project}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QVERIFY(QFileInfo::exists(project));
    r = call("save_project", {{"path", dir.filePath("noext")}});             // gets the suffix
    QVERIFY2(!r.isError, qPrintable(r.text));
    QVERIFY(QFileInfo::exists(dir.filePath("noext.stitchly")));

    select("Pocket");
    r = call("open_project", {{"path", project}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    select("Cuff");
    QVERIFY(window->instructionsText().contains("A x"));
    QVERIFY(call("open_project", {{"path", dir.filePath("missing.stitchly")}}).isError);

    r = call("export_instructions", {{"path", dir.filePath("knit.txt")}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QFile f(dir.filePath("knit.txt"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(f.readAll());
    QVERIFY(text.contains("== Cuff =="));
    QVERIFY(text.contains("== FrontPanel =="));
    r = call("export_row_table", {{"path", dir.filePath("rows.csv")}, {"piece", "Cuff"}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QVERIFY(QFileInfo(dir.filePath("rows.csv")).size() > 50);
    QVERIFY(call("export_row_table", {{"path", dir.filePath("x.csv")}, {"piece", "Nope"}}).isError);
    QVERIFY(call("export_instructions", {{"path", ""}}).isError);

    // opening a pattern starts a new project
    r = call("open_pattern", {{"path", QStringLiteral(TEST_DATA_DIR "/patterns/trousers_gl.sm2d")}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QCOMPARE(window->pieceNames().size(), 11);
    QVERIFY(window->projectFile().isEmpty());
    QVERIFY(call("open_pattern", {{"path", dir.filePath("missing.sm2d")}}).isError);
    QCOMPARE(window->pieceNames().size(), 11);                             // unchanged
}

void TstMainWindow::toolsWithoutPattern()
{
    MainWindow empty;
    auto run = [&empty](const QString &tool, const QJsonObject &args = QJsonObject()) {
        return empty.mcp()->callTool(tool, args);
    };
    McpToolResult r = run("get_instructions");
    QVERIFY(r.isError);
    QVERIFY2(r.text.contains("open_pattern"), qPrintable(r.text));
    QVERIFY(run("get_row_table").isError);
    QVERIFY(run("get_bands").isError);
    QVERIFY(run("save_project", {{"path", "/nonexistent/x.stitchly"}}).isError);
    QVERIFY(run("select_piece", {{"piece", "x"}}).isError);
    QVERIFY(!run("get_project_info").isError);                              // always works
    QVERIFY(run("list_pieces").structured.toObject()["pieces"].toArray().isEmpty());
    QVERIFY(!run("get_catalog").isError);
    QVERIFY(!run("set_stitch_settings", {{"stitchWidthMm", 4}}).isError);   // the settings exist without a pattern
}

void TstMainWindow::toolsOverHttp()
{
    McpIntegration *mcp = window->mcp();
    QVERIFY(!mcp->isRunning());                                             // off until switched on
    QVERIFY(mcp->start(0));
    QVERIFY(mcp->isRunning());
    QCOMPARE(mcp->token(), QString("test-token"));
    QVERIFY(mcp->port() > 0);
    QVERIFY(mcp->claudeCodeCommand().contains("stitchly2d"));
    QVERIFY(mcp->mcpJsonSnippet().contains("Bearer test-token"));

    QNetworkAccessManager network;
    auto post = [&](const QJsonObject &body, const QByteArray &token, int *status) {
        QNetworkRequest request(QUrl(QStringLiteral("http://127.0.0.1:%1/mcp").arg(mcp->port())));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Accept", "application/json, text/event-stream");
        if (!token.isEmpty()) request.setRawHeader("Authorization", "Bearer " + token);
        QNetworkReply *reply = network.post(request, QJsonDocument(body).toJson());
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        *status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QJsonObject json = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();
        return json;
    };
    auto rpc = [](int id, const QString &method, const QJsonObject &params = QJsonObject()) {
        return QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}};
    };

    int status = 0;
    post(rpc(1, "initialize", {{"protocolVersion", "2025-03-26"}, {"capabilities", QJsonObject()},
                               {"clientInfo", QJsonObject{{"name", "test"}, {"version", "1"}}}}), "wrong", &status);
    QCOMPARE(status, 401);                                                  // no access without the token

    QJsonObject init = post(rpc(1, "initialize", {{"protocolVersion", "2025-03-26"}, {"capabilities", QJsonObject()},
                                                  {"clientInfo", QJsonObject{{"name", "test"}, {"version", "1"}}}}),
                            "test-token", &status);
    QCOMPARE(status, 200);
    QCOMPARE(init["result"].toObject()["serverInfo"].toObject()["name"].toString(), QString("stitchly2d"));
    QVERIFY(init["result"].toObject()["instructions"].toString().contains("knitting"));

    const QJsonObject list = post(rpc(2, "tools/list"), "test-token", &status);
    QCOMPARE(status, 200);
    QStringList names;
    for (const QJsonValue &t : list["result"].toObject()["tools"].toArray()) names << t.toObject()["name"].toString();
    for (const QString &expected : {"get_project_info", "set_stitch_settings", "set_bands", "set_pattern",
                                    "get_instructions", "save_project", "screenshot", "list_patterns"})
    {
        QVERIFY2(names.contains(expected), qPrintable(expected));
    }
    QVERIFY(!names.contains("add_point"));                                   // no Seamly2D drawing tools here

    // a change through the connection shows in the window
    const QJsonObject changed = post(rpc(3, "tools/call", {{"name", "set_stitch_settings"},
                                                           {"arguments", QJsonObject{{"stitchType", "Patent"}}}}),
                                     "test-token", &status);
    QCOMPARE(status, 200);
    QVERIFY(!changed["result"].toObject()["isError"].toBool());
    QCOMPARE(find<QComboBox>("stitchTypeCombo")->currentText(), QString("Patent"));
    const QJsonObject instructions = post(rpc(4, "tools/call", {{"name", "get_instructions"},
                                                                {"arguments", QJsonObject{{"piece", "Cuff"}}}}),
                                          "test-token", &status);
    QVERIFY(instructions["result"].toObject()["content"].toArray().at(0).toObject()["text"].toString().startsWith("Cast on"));
    mcp->stop();
    QVERIFY(!mcp->isRunning());
}

void TstMainWindow::undoSettings()
{
    QVERIFY(!window->isModified());
    QVERIFY(window->undoTexts().isEmpty());
    QVERIFY(window->undo().isEmpty());                                     // nothing to undo
    QVERIFY(window->redo().isEmpty());

    find<QComboBox>("stitchTypeCombo")->setCurrentText("Garter");
    QVERIFY(window->isModified());
    QVERIFY(window->isWindowModified());
    QVERIFY(window->windowTitle().contains("[*]"));
    QCOMPARE(window->undoTexts().first(), QString("Change basic stitch"));
    bool menuShowsIt = false;                                               // the Edit menu knows the step
    for (QAction *a : window->findChildren<QAction *>())
        menuShowsIt = menuShowsIt || a->text().contains("Change basic stitch");
    QVERIFY(menuShowsIt);

    find<QCheckBox>("allowanceCheck")->setChecked(true);
    QCOMPARE(window->undoTexts().size(), 2);
    QCOMPARE(window->undo(), QString("Change stitch settings"));
    QVERIFY(!find<QCheckBox>("allowanceCheck")->isChecked());
    QCOMPARE(find<QComboBox>("stitchTypeCombo")->currentText(), QString("Garter"));
    QCOMPARE(window->undo(), QString("Change basic stitch"));
    QCOMPARE(find<QComboBox>("stitchTypeCombo")->currentText(), QString("Stockinette"));
    QCOMPARE(find<QDoubleSpinBox>("rowHeightSpin")->value(), 3.5);
    QVERIFY(!window->isModified());                                         // back at the clean point
    QVERIFY(!window->windowTitle().contains("[*]") || !window->isWindowModified());
    QCOMPARE(window->redo(), QString("Change basic stitch"));
    QVERIFY(window->isModified());
    QCOMPARE(find<QDoubleSpinBox>("rowHeightSpin")->value(), 2.5);
    QCOMPARE(window->redo(), QString("Change stitch settings"));
    QVERIFY(find<QCheckBox>("allowanceCheck")->isChecked());

    // changing nothing is no step
    const int steps = window->undoTexts().size();
    find<QCheckBox>("allowanceCheck")->setChecked(true);
    window->setSettings(window->settings());
    QCOMPARE(window->undoTexts().size(), steps);
    // a new edit after an undo drops the redo steps
    window->undo();
    find<QSpinBox>("edgeStitchesSpin")->setValue(4);
    QVERIFY(window->redo().isEmpty());
}

void TstMainWindow::undoMergesTyping()
{
    auto *width = find<QDoubleSpinBox>("stitchWidthSpin");
    width->setValue(4.0);
    width->setValue(4.5);
    width->setValue(4.8);
    QCOMPARE(window->undoTexts().size(), 1);                                // one step for the typing
    QCOMPARE(window->undoTexts().first(), QString("Change stitch width"));
    QCOMPARE(find<QComboBox>("stitchTypeCombo")->currentText(), QString("Custom"));
    auto *rows = find<QDoubleSpinBox>("rowHeightSpin");
    rows->setValue(3.0);
    rows->setValue(3.2);
    QCOMPARE(window->undoTexts().size(), 2);                                // another field, another step
    window->undo();
    QCOMPARE(rows->value(), 3.5);
    QCOMPARE(width->value(), 4.8);
    window->undo();
    QCOMPARE(width->value(), 5.0);
    QCOMPARE(find<QComboBox>("stitchTypeCombo")->currentText(), QString("Stockinette"));
    QVERIFY(!window->isModified());
    auto *edge = find<QSpinBox>("edgeStitchesSpin");
    edge->setValue(2);
    edge->setValue(3);
    QCOMPARE(window->undoTexts().size(), 1);
    window->undo();
    QCOMPARE(edge->value(), 1);
}

void TstMainWindow::undoBandsAndCatalog()
{
    select("Cuff");
    find<QPushButton>("addBandButton")->click();
    QCOMPARE(window->undoTexts().first(), QString("Add band"));
    QTableWidget *bands = find<QTableWidget>("bandTable");
    qobject_cast<QComboBox *>(bands->cellWidget(0, 0))->setCurrentText("2:2 rib");
    auto *height = qobject_cast<QDoubleSpinBox *>(bands->cellWidget(0, 1));
    height->setValue(20);
    height->setValue(25);
    QCOMPARE(window->undoTexts().size(), 3);                                // add, stitch, height (typing merged)
    bands->selectRow(1);
    find<QPushButton>("addOverlayButton")->click();
    QCOMPARE(window->undoTexts().first(), QString("Add pattern to band"));
    QVERIFY(window->instructionsText().contains("(2:2 rib)"));

    // deleting the stitch from the catalog changes the band; one undo brings back both
    StitchCatalog c = window->catalog();
    c.basics().remove("2:2 rib");
    window->setCatalog(c, "Delete rib");
    QCOMPARE(window->undoTexts().first(), QString("Delete rib"));
    QCOMPARE(qobject_cast<QComboBox *>(find<QTableWidget>("bandTable")->cellWidget(0, 0))->currentText(), QString("Default stitch"));
    QCOMPARE(window->undo(), QString("Delete rib"));
    QVERIFY(window->catalog().basics().contains("2:2 rib"));
    QCOMPARE(qobject_cast<QComboBox *>(find<QTableWidget>("bandTable")->cellWidget(0, 0))->currentText(), QString("2:2 rib"));
    QVERIFY(window->instructionsText().contains("(2:2 rib)"));

    QCOMPARE(window->undo(), QString("Add pattern to band"));
    QCOMPARE(find<QTableWidget>("overlayTable")->rowCount(), 0);
    window->undo();                                                         // height
    QCOMPARE(qobject_cast<QDoubleSpinBox *>(find<QTableWidget>("bandTable")->cellWidget(0, 1))->value(), 0.0 + 50.0);
    window->undo();                                                         // stitch
    window->undo();                                                         // band
    QCOMPARE(find<QTableWidget>("bandTable")->rowCount(), 1);
    QVERIFY(!window->isModified());
    window->redo(); window->redo(); window->redo();
    QCOMPARE(find<QTableWidget>("bandTable")->rowCount(), 2);
    QCOMPARE(qobject_cast<QComboBox *>(find<QTableWidget>("bandTable")->cellWidget(0, 0))->currentText(), QString("2:2 rib"));
    QCOMPARE(qobject_cast<QDoubleSpinBox *>(find<QTableWidget>("bandTable")->cellWidget(0, 1))->value(), 25.0);
}

void TstMainWindow::modifiedAndSaved()
{
    QTemporaryDir dir;
    const QString file = dir.filePath("m.stitchly");
    find<QCheckBox>("allowanceCheck")->setChecked(true);
    QVERIFY(window->isModified());
    QString error;
    QVERIFY2(window->saveProject(file, &error), qPrintable(error));
    QVERIFY(!window->isModified());                                         // saving is the clean point
    QVERIFY(!window->isWindowModified());
    QVERIFY(window->windowTitle().startsWith("m.stitchly"));
    QVERIFY(!call("get_project_info").structured.toObject()["modified"].toBool());
    find<QSpinBox>("edgeStitchesSpin")->setValue(3);
    QVERIFY(window->isModified());
    QVERIFY(call("get_project_info").structured.toObject()["modified"].toBool());
    window->undo();
    QVERIFY(!window->isModified());                                         // undone back to the saved state
    window->redo();
    QVERIFY(window->isModified());
    window->undo();
    window->undo();                                                         // before the save: modified again
    QVERIFY(window->isModified());
}

void TstMainWindow::openingResetsHistory()
{
    find<QCheckBox>("allowanceCheck")->setChecked(true);
    QVERIFY(window->isModified());
    QString error;
    QVERIFY(window->openPattern(QStringLiteral(TEST_DATA_DIR "/patterns/male_shirt.sm2d"), &error));
    QVERIFY(!window->isModified());
    QVERIFY(window->undoTexts().isEmpty());
    QVERIFY(window->undo().isEmpty());
    QTemporaryDir dir;
    const QString file = dir.filePath("o.stitchly");
    QVERIFY(window->saveProject(file, &error));
    find<QSpinBox>("edgeStitchesSpin")->setValue(2);
    QVERIFY(window->openProject(file, &error, MainWindow::CatalogConflict::KeepMine));
    QVERIFY(!window->isModified());
    QVERIFY(window->undoTexts().isEmpty());
    QCOMPARE(find<QSpinBox>("edgeStitchesSpin")->value(), 1);               // what was saved
}

void TstMainWindow::unsavedChangesQuestion()
{
    int asked = 0;
    MainWindow::SaveChoice answer = MainWindow::SaveChoice::Cancel;
    window->setSavePrompt([&]() { ++asked; return answer; });
    QVERIFY(window->maybeSave());                                           // nothing changed: no question
    QCOMPARE(asked, 0);

    find<QCheckBox>("allowanceCheck")->setChecked(true);
    QVERIFY(!window->maybeSave());                                          // cancel: do not go on
    QCOMPARE(asked, 1);
    QVERIFY(window->isModified());
    answer = MainWindow::SaveChoice::Discard;
    QVERIFY(window->maybeSave());
    QVERIFY(window->isModified());                                          // discard keeps the state, the caller replaces it
    QTemporaryDir dir;
    QString error;
    QVERIFY(window->saveProject(dir.filePath("q.stitchly"), &error));
    find<QCheckBox>("allowanceCheck")->setChecked(false);
    answer = MainWindow::SaveChoice::Save;                                  // a project file is known: saved without a dialog
    QVERIFY(window->maybeSave());
    QVERIFY(!window->isModified());
    Project saved;
    QVERIFY(loadProject(dir.filePath("q.stitchly"), saved, &error));
    QVERIFY(!saved.settings.includeSeamAllowance);

    // no pattern open: nothing to lose, no question
    MainWindow empty;
    empty.setSavePrompt([&]() { ++asked; return MainWindow::SaveChoice::Cancel; });
    const int before = asked;
    empty.findChild<QCheckBox *>("allowanceCheck")->setChecked(true);
    QVERIFY(empty.maybeSave());
    QCOMPARE(asked, before);
}

void TstMainWindow::closingAsksFirst()
{
    window->show();
    find<QCheckBox>("allowanceCheck")->setChecked(true);
    window->setSavePrompt([]() { return MainWindow::SaveChoice::Cancel; });
    QVERIFY(!window->close());                                              // stays open
    QVERIFY(window->isVisible());
    window->setSavePrompt([]() { return MainWindow::SaveChoice::Discard; });
    QVERIFY(window->close());
    QVERIFY(!window->isVisible());

    window.reset(new MainWindow);                                           // unchanged window closes without a question
    QString error;
    QVERIFY(window->openPattern(QStringLiteral(TEST_DATA_DIR "/patterns/male_shirt.sm2d"), &error));
    window->show();
    window->setSavePrompt([]() { QTest::qFail("must not be asked", __FILE__, __LINE__); return MainWindow::SaveChoice::Cancel; });
    QVERIFY(window->close());
}

void TstMainWindow::undoByClaude()
{
    QVERIFY(call("undo").isError);                                          // nothing yet
    QVERIFY(call("redo").isError);
    QVERIFY(!call("set_stitch_settings", {{"stitchType", "Garter"}}).isError);
    QCOMPARE(window->undoTexts().first(), QString("Claude: Change stitch settings"));
    QVERIFY(!call("set_basic_stitch", {{"name", "Moss"}, {"stitchWidthMm", 4.5}, {"rowHeightMm", 3.0}}).isError);
    QCOMPARE(window->undoTexts().first(), QString("Claude: Add basic stitch Moss"));
    QVERIFY(!call("set_bands", {{"piece", "Cuff"}, {"bands", QJsonArray{QJsonObject{{"stitchType", "Moss"}}}}}).isError);
    QCOMPARE(window->undoTexts().first(), QString("Claude: Change bands of Cuff"));
    QCOMPARE(window->undoTexts().size(), 3);
    // the user's own step is not labelled, and the label does not stay on afterwards
    find<QSpinBox>("edgeStitchesSpin")->setValue(2);
    QCOMPARE(window->undoTexts().first(), QString("Change edge stitches"));

    McpToolResult r = call("undo");                                         // undoes the user's step too
    QVERIFY(!r.isError);
    QVERIFY(r.text.contains("Change edge stitches"));
    r = call("undo");
    QVERIFY2(r.text.contains("Claude: Change bands of Cuff"), qPrintable(r.text));
    QCOMPARE(window->bands("Cuff").first().stitchType, QString("Default stitch"));
    r = call("redo");
    QVERIFY(r.text.contains("Claude: Change bands of Cuff"));
    QCOMPARE(window->bands("Cuff").first().stitchType, QString("Moss"));
    call("undo"); call("undo"); call("undo");                               // bands, stitch, settings
    call("undo");
    QVERIFY(!window->catalog().basics().contains("Moss"));
    QVERIFY(!window->isModified());
    QVERIFY(call("undo").isError);                                          // the start of the history
}

void TstMainWindow::bulkFromSetting()
{
    select("FrontPanel");
    auto *shapeOn = find<QComboBox>("shapeOnCombo");
    shapeOn->setCurrentIndex(shapeOn->findData(int(ShapingRows::EveryRow)));       // these numbers are for shaping on every row
    const QString standard = window->instructionsText();
    QVERIFY2(standard.contains("cast on 12 stitches at right edge"), qPrintable(standard));
    QVERIFY(standard.contains("every row"));                                  // regular shaping in one line
    auto *bulk = find<QSpinBox>("bulkFromSpin");
    QCOMPARE(bulk->value(), 4);
    bulk->setValue(0);                                                        // never
    QVERIFY(!window->instructionsText().contains("cast on 12"));
    QVERIFY(window->instructionsText().contains("increase 12 (M1)"));
    QCOMPARE(window->undoTexts().first(), QString("Change cast on / bind off limit"));
    window->undo();
    QCOMPARE(bulk->value(), 4);
    QCOMPARE(window->instructionsText(), standard);

    // through Claude
    McpToolResult r = call("set_stitch_settings", {{"bulkFrom", 13}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QCOMPARE(r.structured.toObject()["bulkFrom"].toInt(), 13);
    QVERIFY(window->instructionsText().contains("increase 12 (M1)"));
    QVERIFY(call("set_stitch_settings", {{"bulkFrom", -1}}).isError);
    QVERIFY(call("set_stitch_settings", {{"bulkFrom", 51}}).isError);
    QCOMPARE(window->settings().shaping.bulkFrom, 13);
    // and it is part of a saved project
    QTemporaryDir dir;
    QString error;
    QVERIFY(window->saveProject(dir.filePath("s.stitchly"), &error));
    MainWindow other;
    QVERIFY(other.openProject(dir.filePath("s.stitchly"), &error, MainWindow::CatalogConflict::KeepMine));
    QCOMPARE(other.findChild<QSpinBox *>("bulkFromSpin")->value(), 13);
}

void TstMainWindow::shapeOnSetting()
{
    select("FrontPanel");
    auto *shapeOn = find<QComboBox>("shapeOnCombo");
    QCOMPARE(shapeOn->currentData().toInt(), int(ShapingRows::OutsideRows));         // the default
    const QString outside = window->instructionsText();
    QVERIFY2(outside.contains("Row 1 is an outside row"), qPrintable(outside));
    // every shaping line in the text is on an outside row (odd row number)
    QRegularExpression shapingRow("^Rows? (\\d+)(?:-\\d+)?[^:]*: (?:keep|cast on|bind off)");
    int shapingLines = 0;
    for (const QString &line : outside.split('\n'))
    {
        const QRegularExpressionMatch m = shapingRow.match(line);
        if (m.hasMatch())
        {
            ++shapingLines;
            QCOMPARE(m.captured(1).toInt() % 2, 1);
        }
    }
    QVERIFY(shapingLines > 3);

    shapeOn->setCurrentIndex(shapeOn->findData(int(ShapingRows::EveryRow)));
    const QString every = window->instructionsText();
    QVERIFY(!every.contains("outside row"));
    QVERIFY(every != outside);
    QCOMPARE(window->undoTexts().first(), QString("Change stitch settings"));
    window->undo();
    QCOMPARE(window->instructionsText(), outside);
    QCOMPARE(shapeOn->currentData().toInt(), int(ShapingRows::OutsideRows));

    // the row table in the file is the one of the plan
    const QString table = call("get_row_table", {{"piece", "FrontPanel"}}).text;
    QVERIFY(table.startsWith("row;band;stitch type"));
    // Claude
    McpToolResult r = call("set_stitch_settings", {{"shapeOn", "every"}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QCOMPARE(r.structured.toObject()["shapeOn"].toString(), QString("every"));
    QCOMPARE(shapeOn->currentData().toInt(), int(ShapingRows::EveryRow));
    QVERIFY(call("set_stitch_settings", {{"shapeOn", "sideways"}}).isError);
    QCOMPARE(shapeOn->currentData().toInt(), int(ShapingRows::EveryRow));
    // saved in the project
    QTemporaryDir dir;
    QString error;
    QVERIFY(window->saveProject(dir.filePath("p.stitchly"), &error));
    MainWindow other;
    QVERIFY(other.openProject(dir.filePath("p.stitchly"), &error, MainWindow::CatalogConflict::KeepMine));
    QCOMPARE(other.findChild<QComboBox *>("shapeOnCombo")->currentData().toInt(), int(ShapingRows::EveryRow));
}

void TstMainWindow::methodDefaultAndPiece()
{
    auto *method = find<QComboBox>("methodCombo");
    auto *own = find<QComboBox>("pieceMethodCombo");
    QCOMPARE(method->currentData().toInt(), int(KnitMethod::Flat));
    select("Cuff");
    QCOMPARE(own->currentData().toInt(), -1);                                  // like the default
    QVERIFY(window->instructionsText().startsWith("Cast on 58 stitches."));
    QVERIFY(!window->hasPieceMethod("Cuff"));

    // the default for all pieces
    method->setCurrentIndex(method->findData(int(KnitMethod::Round)));
    QVERIFY2(window->instructionsText().startsWith("Cast on 58 stitches. Join in the round"), qPrintable(window->instructionsText()));
    QVERIFY(!window->instructionsText().contains("Row "));
    QVERIFY(window->pieceMethod("Pocket") == KnitMethod::Round);
    select("FrontPanel");
    QVERIFY(window->instructionsText().contains("Join in the round"));
    QVERIFY(!window->instructionsText().contains("outside row"));
    QVERIFY2(window->instructionsText().contains("Round 2"), "shaping on inside rounds exists in a tube");
    window->undo();
    QCOMPARE(method->currentData().toInt(), int(KnitMethod::Flat));
    QVERIFY(window->instructionsText().contains("Row 1 is an outside row"));

    // one piece on its own
    select("Cuff");
    own->setCurrentIndex(own->findData(int(KnitMethod::CircularMachine)));
    QVERIFY(window->hasPieceMethod("Cuff"));
    QCOMPARE(window->undoTexts().first(), QString("Change knitting method of Cuff"));
    QVERIFY2(window->instructionsText().startsWith("Put 58 needles in work and cast on"), qPrintable(window->instructionsText()));
    QVERIFY(window->instructionsText().contains("Take the work off the machine"));
    select("Pocket");
    QCOMPARE(own->currentData().toInt(), -1);                                  // other pieces keep the default
    QVERIFY(window->instructionsText().startsWith("Cast on"));
    QVERIFY(!window->instructionsText().contains("Put "));
    select("Cuff");
    QCOMPARE(own->currentData().toInt(), int(KnitMethod::CircularMachine));    // and the choice is remembered
    // the whole export uses each piece's own method
    const QString all = window->allInstructionsText();
    QVERIFY(all.contains("Put 58 needles in work"));
    QVERIFY(all.contains("== Pocket =="));
    QVERIFY(call("get_instructions", {{"piece", "Cuff"}}).text.startsWith("Put 58 needles"));
    QVERIFY(call("get_row_table", {{"piece", "Cuff"}}).text.startsWith("row;band"));
    const QJsonArray pieces = call("list_pieces").structured.toObject()["pieces"].toArray();
    for (const QJsonValue &v : pieces)
        if (v.toObject()["name"].toString() == "Cuff") QCOMPARE(v.toObject()["method"].toString(), QString("circular_machine"));
        else if (v.toObject()["name"].toString() == "Pocket") QCOMPARE(v.toObject()["method"].toString(), QString("flat"));
    // back to the default; undo and redo
    own->setCurrentIndex(own->findData(-1));
    QVERIFY(!window->hasPieceMethod("Cuff"));
    QVERIFY(window->instructionsText().startsWith("Cast on 58 stitches."));
    window->undo();
    QVERIFY(window->hasPieceMethod("Cuff"));
    QCOMPARE(own->currentData().toInt(), int(KnitMethod::CircularMachine));
    window->undo();
    QVERIFY(!window->hasPieceMethod("Cuff"));
    QVERIFY(!window->isModified());
}

void TstMainWindow::methodMachineNeedles()
{
    auto *needles = find<QSpinBox>("needlesSpin");
    auto *own = find<QComboBox>("pieceMethodCombo");
    select("Cuff");
    own->setCurrentIndex(own->findData(int(KnitMethod::CircularMachine)));
    QVERIFY(!window->instructionsText().contains("Warning"));                   // needles are not checked
    needles->setValue(40);
    QVERIFY2(window->instructionsText().contains("Warning: the piece needs 58 needles, the machine has 40."), qPrintable(window->instructionsText()));
    needles->setValue(58);
    QVERIFY(!window->instructionsText().contains("Warning"));
    // typing in the box is one undo step; the double-bed machine counts per bed
    needles->setValue(10); needles->setValue(20); needles->setValue(25);
    QCOMPARE(window->undoTexts().first(), QString("Change machine needles"));
    own->setCurrentIndex(own->findData(int(KnitMethod::DoubleBedMachine)));
    QVERIFY2(window->instructionsText().contains("Warning: the piece needs 29 needles per bed, the machine has 25."), qPrintable(window->instructionsText()));
    needles->setValue(29);
    QVERIFY(!window->instructionsText().contains("Warning"));
    QVERIFY(window->instructionsText().contains("tubular: 29 needles on the front bed and 29 on the back bed"));
    // by hand the machine does not matter
    own->setCurrentIndex(own->findData(int(KnitMethod::Round)));
    needles->setValue(5);
    QVERIFY(!window->instructionsText().contains("Warning"));
}

void TstMainWindow::methodInProjects()
{
    auto *method = find<QComboBox>("methodCombo");
    method->setCurrentIndex(method->findData(int(KnitMethod::DoubleBedMachine)));
    find<QSpinBox>("needlesSpin")->setValue(120);
    select("Cuff");
    auto *own = find<QComboBox>("pieceMethodCombo");
    own->setCurrentIndex(own->findData(int(KnitMethod::Round)));
    select("Pocket");
    const QString pocket = window->instructionsText();
    select("Cuff");
    const QString cuff = window->instructionsText();
    QVERIFY(cuff.contains("Join in the round"));
    QVERIFY(pocket.contains("tubular"));

    QTemporaryDir dir;
    QString error;
    QVERIFY(window->saveProject(dir.filePath("m.stitchly"), &error));
    MainWindow other;
    QVERIFY2(other.openProject(dir.filePath("m.stitchly"), &error, MainWindow::CatalogConflict::KeepMine), qPrintable(error));
    QCOMPARE(other.findChild<QComboBox *>("methodCombo")->currentData().toInt(), int(KnitMethod::DoubleBedMachine));
    QCOMPARE(other.findChild<QSpinBox *>("needlesSpin")->value(), 120);
    QVERIFY(other.hasPieceMethod("Cuff"));
    QVERIFY(!other.hasPieceMethod("Pocket"));
    QVERIFY(other.pieceMethod("Cuff") == KnitMethod::Round);
    QVERIFY(other.pieceMethod("Pocket") == KnitMethod::DoubleBedMachine);
    QCOMPARE(other.instructionsFor("Cuff").join('\n'), cuff);
    QCOMPARE(other.instructionsFor("Pocket").join('\n'), pocket);
    QVERIFY(!other.isModified());
    // a new pattern starts without the piece methods
    QVERIFY(other.openPattern(QStringLiteral(TEST_DATA_DIR "/patterns/male_shirt.sm2d"), &error));
    QVERIFY(!other.hasPieceMethod("Cuff"));
}

void TstMainWindow::methodTools()
{
    McpToolResult r = call("set_knitting_method", {{"piece", "Cuff"}, {"method", "double_bed_machine"}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QCOMPARE(window->currentPiece(), QString("Cuff"));
    QVERIFY(window->pieceMethod("Cuff") == KnitMethod::DoubleBedMachine);
    QCOMPARE(window->undoTexts().first(), QString("Claude: Change knitting method of Cuff"));
    QVERIFY(window->instructionsText().contains("two carriage passes"));
    QCOMPARE(call("get_project_info").structured.toObject()["pieceMethods"].toObject()["Cuff"].toString(), QString("double_bed_machine"));
    r = call("set_knitting_method", {{"method", "round"}});                    // the selected piece
    QVERIFY(!r.isError);
    QVERIFY(window->pieceMethod("Cuff") == KnitMethod::Round);
    QVERIFY(!call("set_knitting_method", {{"piece", "Cuff"}, {"method", "default"}}).isError);
    QVERIFY(!window->hasPieceMethod("Cuff"));
    QVERIFY(call("set_knitting_method", {{"piece", "Cuff"}, {"method", "sideways"}}).isError);
    QVERIFY(call("set_knitting_method", {{"piece", "Nope"}, {"method", "flat"}}).isError);
    QVERIFY(call("set_knitting_method", {{"piece", "Cuff"}}).isError);              // no method given

    // the default method and the needles
    r = call("set_stitch_settings", {{"method", "circular_machine"}, {"machineNeedles", 40}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    QCOMPARE(r.structured.toObject()["method"].toString(), QString("circular_machine"));
    QCOMPARE(r.structured.toObject()["machineNeedles"].toInt(), 40);
    QCOMPARE(find<QComboBox>("methodCombo")->currentData().toInt(), int(KnitMethod::CircularMachine));
    QVERIFY(call("get_instructions", {{"piece", "Cuff"}}).text.contains("Warning: the piece needs 58 needles, the machine has 40."));
    QVERIFY(call("set_stitch_settings", {{"method", "sideways"}}).isError);
    QVERIFY(call("set_stitch_settings", {{"machineNeedles", -1}}).isError);
    QVERIFY(call("set_stitch_settings", {{"machineNeedles", 2001}}).isError);
    QCOMPARE(window->settings().shaping.machineNeedles, 40);
    // Claude can take it back
    QVERIFY(call("undo").text.contains("Claude: Change stitch settings"));
    QVERIFY(window->settings().shaping.method == KnitMethod::Flat);
}

namespace
{
Yarn makeYarn(const QString &name, qreal stockMeters, bool withGauge)
{
    Yarn y;
    y.id = Yarn::newId();
    y.brand = "Woolly"; y.name = name; y.colorName = "red"; y.colorHex = "#c03030";
    y.metersPerSkein = stockMeters; y.skeinsInStock = 1;
    if (withGauge)
    {
        y.gauges.append(YarnGauge{"Stockinette", StitchSize::fromPer10cm(25, 30), 0});
        y.gauges.append(YarnGauge{"No such stitch", StitchSize::fromPer10cm(20, 30), 0});
    }
    return y;
}
}

void TstMainWindow::yarnsChosenAndReported()
{
    YarnInventory inv;
    const QString small = inv.set(makeYarn("Small", 100, false));
    const QString big = inv.set(makeYarn("Big", 100000, false));
    window->setYarnInventory(inv);
    // a stripe pattern on the cuff makes the colors A and B yarn slotNames of their own
    select("Cuff");
    find<QPushButton>("addOverlayButton")->click();
    auto *pattern = qobject_cast<QComboBox *>(find<QTableWidget>("overlayTable")->cellWidget(0, 0));
    pattern->setCurrentIndex(pattern->findData("Stripes 2 rows"));

    QVector<MainWindow::YarnLine> report = window->yarnReport();
    QStringList slotNames;
    for (const auto &line : report) slotNames << line.slot;
    QCOMPARE(slotNames.first(), QString("main"));
    QVERIFY(slotNames.contains("A") && slotNames.contains("B"));
    for (const auto &line : report)
    {
        QVERIFY(line.stitches > 0);
        QCOMPARE(line.neededMeters, 0.0);                    // no yarn chosen yet
    }

    QString error;
    QVERIFY(!window->setYarn("main", "no-such-id", &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(window->setYarn("main", small));
    report = window->yarnReport();
    QCOMPARE(report.first().yarnName, QString("Woolly Small (red)"));
    QVERIFY(report.first().neededMeters > 100);              // a whole shirt from 100 m
    QVERIFY(!report.first().enough);
    QVERIFY(window->setYarn("main", big));
    QVERIFY(window->setYarn("A", big));                      // the same yarn in two slots has to last for both
    report = window->yarnReport();
    QVERIFY(report.first().enough);
    QCOMPARE(report.first().stockMeters, 100000.0);
    QVERIFY(window->isModified());
    QVERIFY(window->undo().contains("Choose yarn"));         // undo takes "A" back off
    QVERIFY(window->yarnRefs().contains("main"));
    QVERIFY(!window->yarnRefs().contains("A"));
    QVERIFY(window->setYarn("main", QString()));             // no yarn again
    QVERIFY(window->yarnRefs().isEmpty());
}

void TstMainWindow::yarnsInProjects()
{
    YarnInventory inv;
    const QString id = inv.set(makeYarn("Big", 100000, false));
    window->setYarnInventory(inv);
    QVERIFY(window->setYarn("main", id));
    QTemporaryDir dir;
    const QString file = dir.filePath("y.stitchly");
    QString error;
    QVERIFY2(window->saveProject(file, &error), qPrintable(error));
    QVERIFY(window->setYarn("main", QString()));
    QVERIFY2(window->openProject(file, &error, MainWindow::CatalogConflict::KeepMine), qPrintable(error));
    QCOMPARE(window->yarnRefs().value("main").id, id);
    QVERIFY(!window->isModified());
    // a project from a computer that does not have the yarn: the stored name is shown, nothing is estimated
    window->setYarnInventory(YarnInventory());
    QVector<MainWindow::YarnLine> report = window->yarnReport();
    QCOMPARE(report.first().yarnId, id);
    QCOMPARE(report.first().yarnName, QString("Woolly Big (red)"));
    QCOMPARE(report.first().neededMeters, 0.0);
    // opening another pattern starts a new project without yarns
    QVERIFY(window->openPattern(QStringLiteral(TEST_DATA_DIR "/patterns/male_shirt.sm2d"), &error));
    QVERIFY(window->yarnRefs().isEmpty());
}

void TstMainWindow::yarnGaugeSetsStitchSizes()
{
    YarnInventory inv;
    const QString id = inv.set(makeYarn("Gauged", 100000, true));
    const QString plain = inv.set(makeYarn("Plain", 100000, false));
    window->setYarnInventory(inv);
    QString error;
    QCOMPARE(window->useYarnGauge(&error), 0);               // no main yarn yet
    QVERIFY(!error.isEmpty());
    QVERIFY(window->setYarn("main", plain));
    QCOMPARE(window->useYarnGauge(&error), 0);               // this yarn has no gauge
    QVERIFY(window->setYarn("main", id));
    select("Cuff");
    const QString before = window->instructionsText();
    QCOMPARE(window->useYarnGauge(&error), 1);               // "No such stitch" is not in the catalog
    const StitchType t = window->catalog().basics().find("Stockinette");
    QVERIFY(qAbs(t.size.stitchWidthMm - 4.0) < 1e-9);
    QVERIFY(qAbs(t.size.rowHeightMm - 100.0 / 30) < 1e-9);
    QVERIFY(window->instructionsText() != before);           // 4 mm stitches give more stitches
    QVERIFY(window->undo().contains("Use gauge"));
    QCOMPARE(window->instructionsText(), before);
}

void TstMainWindow::yarnPanel()
{
    YarnInventory inv;
    const QString id = inv.set(makeYarn("Big", 100000, false));
    window->setYarnInventory(inv);
    QDockWidget *dock = find<QDockWidget>("yarnsDock");
    QVERIFY(dock);
    dock->raise();
    QTest::qWait(20);
    auto *combo = find<QComboBox>("yarnCombo_main");
    QVERIFY(combo);
    QCOMPARE(combo->count(), 2);                             // no yarn + the one yarn
    combo->setCurrentIndex(combo->findData(id));
    QCOMPARE(window->yarnRefs().value("main").id, id);
    QTest::qWait(20);                                        // the panel is rebuilt
    auto *info = find<QLabel>("yarnInfo_main");
    QVERIFY(info);
    QVERIFY2(info->text().contains("100000 m in stock"), qPrintable(info->text()));
    QVERIFY(find<QPushButton>("useYarnGaugeButton"));
    QVERIFY(find<QPushButton>("editYarnsButton"));
}

void TstMainWindow::yarnInventoryEditor()
{
    yarnui::YarnInventoryWidget w(YarnInventory(), {"Stockinette", "Garter"});
    auto child = [&](const char *name) { return w.findChild<QWidget *>(name); };
    QVERIFY(!child("yarnName")->isEnabled());                 // nothing selected
    qobject_cast<QPushButton *>(child("addYarnButton"))->click();
    QCOMPARE(w.inventory().size(), 1);
    auto *name = qobject_cast<QLineEdit *>(child("yarnName"));
    QVERIFY(name->isEnabled());
    name->setText(QString());
    QTest::keyClicks(name, "Soft");
    qobject_cast<QDoubleSpinBox *>(child("yarnMeters"))->setValue(200);
    qobject_cast<QSpinBox *>(child("yarnSkeins"))->setValue(2);
    qobject_cast<QDoubleSpinBox *>(child("yarnPartial"))->setValue(40);
    auto *hex = qobject_cast<QLineEdit *>(child("yarnColorHex"));
    QTest::keyClicks(hex, "#12");                              // half typed: the yarn keeps its old value
    QVERIFY(w.inventory().yarns().first().colorHex.isEmpty());
    QTest::keyClicks(hex, "34ab");
    qobject_cast<QPushButton *>(child("addGaugeButton"))->click();
    qobject_cast<QPushButton *>(child("addGaugeButton"))->click();
    qobject_cast<QPushButton *>(child("addGaugeButton"))->click();   // only two stitches to give a gauge for
    const Yarn y = w.inventory().yarns().first();
    QCOMPARE(y.name, QString("Soft"));
    QCOMPARE(y.stockMeters(), 440.0);
    QCOMPARE(y.colorHex, QString("#1234ab"));
    QCOMPARE(y.gauges.size(), 2);
    QVERIFY(y.gaugeFor("Stockinette") && y.gaugeFor("Garter"));
    auto *rows = qobject_cast<QDoubleSpinBox *>(qobject_cast<QTableWidget *>(child("yarnGauges"))->cellWidget(0, 2));
    rows->setValue(40);
    QVERIFY(qAbs(w.inventory().yarns().first().gauges.first().size.rowsPer10cm() - 40) < 1e-6);
    qobject_cast<QPushButton *>(child("duplicateYarnButton"))->click();
    QCOMPARE(w.inventory().size(), 2);
    QVERIFY(w.inventory().yarns().last().name.contains("copy"));
    QVERIFY(w.validate());
    name->setText("x");
    QTest::keyClick(name, Qt::Key_Backspace);                  // the copy loses its name
    QString error;
    QVERIFY(w.inventory().yarns().last().name.isEmpty());
    QVERIFY(!w.validate(&error));
    QVERIFY(!error.isEmpty());
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
    // The tests must not touch the user's settings, catalog or the list of running windows.
    QTemporaryDir isolated;
    qputenv("XDG_CONFIG_HOME", (isolated.path() + QStringLiteral("/config")).toUtf8());
    qputenv("STITCHLY_MCP_INSTANCE_DIR", (isolated.path() + QStringLiteral("/instances")).toUtf8());
    qputenv("STITCHLY_MCP_TOKEN", "test-token");
    qputenv("STITCHLY_MCP_ASSUME_YES", "1");        // no dialog can be clicked in a test
    Application2D app(argc, argv);
    TstMainWindow test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_mainwindow.moc"

void TstMainWindow::toolsYarns()
{
    McpToolResult r = call("set_yarn", {{"brand", "Test"}, {"name", "Fine"}, {"colorHex", "#336699"},
                                        {"metersPerSkein", 200}, {"skeinsInStock", 3},
                                        {"gauges", QJsonArray{QJsonObject{{"stitchType", "Garter"}, {"stitchWidthMm", 3.5}, {"rowHeightMm", 2.5}}}}});
    QVERIFY2(!r.isError, qPrintable(r.text));
    const QString id = r.structured.toObject().value("id").toString();
    QVERIFY(!id.isEmpty());
    QCOMPARE(window->yarnInventory().size(), 1);
    QCOMPARE(window->yarnInventory().find(id)->stockMeters(), 600.0);

    // change by name keeps the rest
    QVERIFY(!call("set_yarn", {{"name", "fine"}, {"skeinsInStock", 5}}).isError);
    QCOMPARE(window->yarnInventory().size(), 1);
    QCOMPARE(window->yarnInventory().find(id)->skeinsInStock, 5);
    QCOMPARE(window->yarnInventory().find(id)->gauges.size(), 1);

    QVERIFY(call("set_yarn", {{"name", "Bad"}, {"colorHex", "nope"}}).isError);
    QVERIFY(call("set_yarn", {{"notes", "no name"}}).isError);
    QVERIFY(call("set_yarn", {{"id", "missing"}, {"name", "X"}}).isError);
    QVERIFY(call("set_yarn", {{"name", "Neg"}, {"skeinsInStock", -1}}).isError);
    QCOMPARE(window->yarnInventory().size(), 1);

    QVERIFY(call("choose_yarn", {{"yarnId", "missing"}}).isError);
    QVERIFY(!call("choose_yarn", {{"yarnId", id}}).isError);
    QCOMPARE(window->yarnRefs().value("main").id, id);
    const QJsonObject info = call("get_yarns").structured.toObject();
    QCOMPARE(info.value("yarns").toArray().size(), 1);
    QCOMPARE(info.value("slots").toArray().first().toObject().value("yarnId").toString(), id);

    QVERIFY(!call("choose_yarn", {{"yarnId", ""}}).isError);
    QVERIFY(call("use_yarn_gauge").isError);                                  // no main yarn

    QVERIFY(!call("delete_yarn", {{"id", id}}).isError);
    QVERIFY(call("delete_yarn", {{"id", id}}).isError);
}
