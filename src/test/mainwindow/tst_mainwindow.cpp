// Drives the real main window: loads a sample pattern, edits bands and patterns, checks the instructions.
#include <QtTest>

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>

#include "../../app/stitchly2d/core/application_2d.h"
#include "../../app/stitchly2d/mainwindow.h"

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
