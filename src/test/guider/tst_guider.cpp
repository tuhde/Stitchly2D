// Drives the guider window on the offscreen platform: loads a sample pattern, steps through rows and pieces,
// checks the saved state and that the layout fits screens of different sizes.
#include <QtTest>

#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>

#include "../../app/guider/guide_scale.h"
#include "../../app/guider/guide_window.h"
#include "../../app/guider/step_view.h"
#include "../../libs/stitchload/core/application_2d.h"

#undef qApp
#define qApp (static_cast<QApplication *>(QCoreApplication::instance()))

using namespace stitchcore;

namespace
{
QString shirt() { return QStringLiteral(TEST_DATA_DIR "/patterns/male_shirt.sm2d"); }
}

class TstGuider : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void opensPattern();
    void rowsAreClamped();
    void keysStepThroughRows();
    void stepsFollowTheRow();
    void piecesKeepTheirRow();
    void rowIsRememberedPerFile();
    void badFileReportsError();
    void layoutFitsScreens_data();
    void layoutFitsScreens();
    void startsOnFirstScreenByDefault();
    void savedScreenIsUsed();
};

void TstGuider::init()
{
    QSettings(QStringLiteral("Stitchly2D"), QStringLiteral("guider")).clear();
}

void TstGuider::opensPattern()
{
    GuideWindow w;
    QString error;
    QVERIFY2(w.openFile(shirt(), &error), qPrintable(error));
    QCOMPARE(w.job().pieces.size(), 17);
    QVERIFY(w.rowCount() > 0);
    QCOMPARE(w.row(), 0);
    QVERIFY(w.recentFiles().contains(w.job().input));
}

void TstGuider::rowsAreClamped()
{
    GuideWindow w;
    QVERIFY(w.openFile(shirt()));
    w.setRow(-5);
    QCOMPARE(w.row(), 0);
    w.setRow(1000000);
    QCOMPARE(w.row(), w.rowCount() + 1);          // after the last row: bind off
    w.nextRow();
    QCOMPARE(w.row(), w.rowCount() + 1);
    w.setRow(0);
    w.previousRow();
    QCOMPARE(w.row(), 0);
}

void TstGuider::keysStepThroughRows()
{
    GuideWindow w;
    QVERIFY(w.openFile(shirt()));
    w.show();
    QTest::keyClick(&w, Qt::Key_Right);
    QTest::keyClick(&w, Qt::Key_Space);
    QCOMPARE(w.row(), 2);
    QTest::keyClick(&w, Qt::Key_Left);
    QCOMPARE(w.row(), 1);
    QTest::keyClick(&w, Qt::Key_Home);
    QCOMPARE(w.row(), 0);
    // The buttons do the same
    const auto buttons = w.findChildren<QPushButton *>();
    QPushButton *next = nullptr;
    for (QPushButton *b : buttons) if (b->text().startsWith(QLatin1String("Next"))) next = b;
    QVERIFY(next);
    next->click();
    QCOMPARE(w.row(), 1);
}

void TstGuider::stepsFollowTheRow()
{
    GuideWindow w;
    QVERIFY(w.openFile(shirt()));
    const KnitPiece &p = w.job().pieces.at(w.piece());
    for (int row : {0, 1, w.rowCount() / 2, w.rowCount(), w.rowCount() + 1})
    {
        w.setRow(row);
        QVERIFY2(!stepsForRow(p.steps, row).isEmpty(), qPrintable(QStringLiteral("no step for row %1").arg(row)));
    }
}

void TstGuider::piecesKeepTheirRow()
{
    GuideWindow w;
    QVERIFY(w.openFile(shirt()));
    const int first = w.piece();
    w.setRow(7);
    int other = -1;
    for (int i = 0; i < w.job().pieces.size() && other < 0; ++i)
        if (i != first && w.job().pieces.at(i).rowCount > 8) other = i;
    QVERIFY(other >= 0);
    w.setPiece(other);
    QCOMPARE(w.row(), 0);
    w.setRow(3);
    w.setPiece(first);
    QCOMPARE(w.row(), 7);
    w.setPiece(other);
    QCOMPARE(w.row(), 3);
}

void TstGuider::rowIsRememberedPerFile()
{
    int piece = 0;
    {
        GuideWindow w;
        QVERIFY(w.openFile(shirt()));
        w.setRow(5);
        piece = w.piece();
    }
    GuideWindow again;
    QVERIFY(again.openFile(shirt()));
    QCOMPARE(again.piece(), piece);
    QCOMPARE(again.row(), 5);
}

void TstGuider::badFileReportsError()
{
    GuideWindow w;
    QString error;
    QVERIFY(!w.openFile(QStringLiteral("/nonexistent/none.sm2d"), &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(w.job().pieces.isEmpty());
    // A failed open leaves a working window
    w.nextRow();
    QCOMPARE(w.row(), 0);
}

void TstGuider::layoutFitsScreens_data()
{
    QTest::addColumn<QSize>("size");
    QTest::addColumn<int>("unit");
    QTest::newRow("Full HD") << QSize(1920, 1080) << 20;
    QTest::newRow("HD ready") << QSize(1280, 720) << 13;
    QTest::newRow("small 800x480") << QSize(800, 480) << 9;
    QTest::newRow("portrait") << QSize(600, 1024) << 19;
}

void TstGuider::layoutFitsScreens()
{
    QFETCH(QSize, size);
    QFETCH(int, unit);
    qputenv("GUIDER_UNIT", QByteArray::number(unit));
    applyGuideStyle(*qApp);          // the touch sizes, as main() sets them
    GuideWindow w;
    QVERIFY(w.openFile(shirt()));
    w.resize(size);
    w.show();
    w.setRow(3);
    QVERIFY(QTest::qWaitForWindowExposed(&w));

    // Every button is completely inside the window and a finger can hit it
    const auto buttons = w.findChildren<QPushButton *>();
    QVERIFY(buttons.size() >= 5);
    for (QPushButton *b : buttons)
    {
        const QRect r(b->mapTo(&w, QPoint()), b->size());
        QVERIFY2(w.rect().contains(r), qPrintable(b->text()));
        QVERIFY2(r.height() >= unit * 2, qPrintable(b->text()));
    }
    auto *view = w.findChild<StepView *>();
    QVERIFY(view);
    QVERIFY(view->height() > size.height() / 3);
    QVERIFY(view->width() > size.width() * 0.8);
    // The current step is painted in the view: its highlight color is somewhere in the picture
    const QImage image = view->grab().toImage();
    bool highlight = false;
    for (int y = 0; y < image.height() && !highlight; y += 4)
        for (int x = 0; x < image.width() && !highlight; x += 4)
            highlight = image.pixelColor(x, y) == QColor(0x1d, 0x2c, 0x44);
    QVERIFY(highlight);
    qunsetenv("GUIDER_UNIT");
}

void TstGuider::startsOnFirstScreenByDefault()
{
    QVERIFY(guideScreen() == QGuiApplication::screens().first());
}

void TstGuider::savedScreenIsUsed()
{
    QScreen *screen = QGuiApplication::screens().first();
    saveGuideScreen(screen);
    QVERIFY(guideScreen() == screen);
    QSettings(QStringLiteral("Stitchly2D"), QStringLiteral("guider")).setValue(QStringLiteral("screen"), QStringLiteral("gone"));
    QVERIFY(guideScreen() == QGuiApplication::screens().first());     // an unplugged monitor falls back to monitor 0
    GuideWindow w;
    w.showOnSavedScreen(false);
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    QVERIFY(w.screen() == screen);
    w.close();
    QCOMPARE(QSettings(QStringLiteral("Stitchly2D"), QStringLiteral("guider")).value(QStringLiteral("screen")).toString(),
             screen->name());
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
    // The tests must not touch the user's settings or catalog.
    QTemporaryDir isolated;
    qputenv("XDG_CONFIG_HOME", (isolated.path() + QStringLiteral("/config")).toUtf8());
    Application2D app(argc, argv);
    TstGuider test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_guider.moc"
