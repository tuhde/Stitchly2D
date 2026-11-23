// Drives the guider window on the offscreen platform: loads a sample pattern, steps through rows and pieces,
// checks the saved state and that the layout fits screens of different sizes.
#include <QtTest>

#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>

#include "../../app/guider/guide_scale.h"
#include "../../app/guider/settings_dialog.h"
#include "../../app/guider/gpio_row_counter.h"
#include "../../app/guider/guide_theme.h"
#include "../../app/guider/guide_window.h"
#include "../../app/guider/open_dialog.h"
#include "../../app/guider/row_counter.h"
#include "../../app/guider/simulated_row_counter.h"
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
    void pageKeysJumpByBlock();
    void piecesKeepTheirRow();
    void rowIsRememberedPerFile();
    void badFileReportsError();
    void layoutFitsScreens_data();
    void layoutFitsScreens();
    void startsOnFirstScreenByDefault();
    void savedScreenIsUsed();
    void pulsesBecomeRows();
    void pulsesBecomeRowsBackwards();
    void configIsChecked_data();
    void configIsChecked();
    void configIsSaved();
    void simulatedCounterEmitsRows();
    void windowFollowsCounter();
    void gpioFailsCleanly();
    void stepperClamps();
    void choiceAddsUnknownValue();
    void settingsDialogRoundTrip();
    void settingsDialogTestCountsPulses();
    void settingsDialogFitsSmallScreen();
    void applyConfigReplacesCounter();
    void themeNamesRoundTrip();
    void themeFollowsSystem();
    void themeChoiceIsSavedAndApplied();
    void buttonsStandOutInEveryTheme_data();
    void buttonsStandOutInEveryTheme();
    void settingsDialogChoosesTheme();
    void looksOfDialogs();
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

void TstGuider::pageKeysJumpByBlock()
{
    GuideWindow w;
    QVERIFY(w.openFile(shirt()));
    w.show();
    const auto &steps = w.job().pieces.at(w.piece()).steps;
    // Page Down visits the start of every block in turn, up to the bind-off line
    QVector<int> starts;
    for (const auto &s : steps) if (!starts.contains(s.firstRow)) starts << s.firstRow;
    std::sort(starts.begin(), starts.end());
    QVERIFY(starts.size() > 3);
    QCOMPARE(w.row(), 0);
    for (int i = 1; i < starts.size(); ++i)
    {
        QTest::keyClick(&w, Qt::Key_PageDown);
        QCOMPARE(w.row(), starts.at(i));
    }
    QCOMPARE(w.row(), w.rowCount() + 1);
    QTest::keyClick(&w, Qt::Key_PageDown);            // stays at the end
    QCOMPARE(w.row(), w.rowCount() + 1);
    // Page Up goes back the same way
    for (int i = starts.size() - 2; i >= 0; --i)
    {
        QTest::keyClick(&w, Qt::Key_PageUp);
        QCOMPARE(w.row(), starts.at(i));
    }
    QTest::keyClick(&w, Qt::Key_PageUp);
    QCOMPARE(w.row(), 0);
    // From the middle of a block ("Rows 1-2: work even" on the sample): Page Down to the next block, Page Up to the
    // start of this one
    w.setRow(2);
    QTest::keyClick(&w, Qt::Key_PageUp);
    QCOMPARE(w.row(), 1);
    w.setRow(2);
    QTest::keyClick(&w, Qt::Key_PageDown);
    QVERIFY(w.row() > 2);
    // End jumps to the bind-off block, Home back to the cast-on
    w.setRow(5);
    QTest::keyClick(&w, Qt::Key_End);
    QCOMPARE(w.row(), w.rowCount() + 1);
    QVERIFY(stitchcore::stepsForRow(steps, w.row()).contains(steps.size() - 1));
    QTest::keyClick(&w, Qt::Key_End);                 // already there
    QCOMPARE(w.row(), w.rowCount() + 1);
    QTest::keyClick(&w, Qt::Key_Home);
    QCOMPARE(w.row(), 0);
    // The arrow keys still move by one row
    w.setRow(0);
    QTest::keyClick(&w, Qt::Key_Down);
    QCOMPARE(w.row(), 1);
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
            highlight = image.pixelColor(x, y) == guideTheme().stepBackground;
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

void TstGuider::pulsesBecomeRows()
{
    PulseCounter c(3);
    QCOMPARE(c.add(), 0);
    QCOMPARE(c.add(), 0);
    QCOMPARE(c.add(), 1);
    QCOMPARE(c.pending(), 0);
    QCOMPARE(c.add(7), 2);                 // 7 pulses = 2 rows and 1 left over
    QCOMPARE(c.pending(), 1);
    PulseCounter one(0);                   // nonsense is treated as 1
    QCOMPARE(one.pulsesPerRow(), 1);
    QCOMPARE(one.add(), 1);
}

void TstGuider::pulsesBecomeRowsBackwards()
{
    PulseCounter c(4);
    c.add(2);
    QCOMPARE(c.add(-2), 0);                // turning round takes back what was counted
    QCOMPARE(c.pending(), 0);
    QCOMPARE(c.add(-4), -1);
    QCOMPARE(c.add(-3), 0);
    QCOMPARE(c.add(-1), -1);
}

void TstGuider::configIsChecked_data()
{
    QTest::addColumn<QString>("kind");
    QTest::addColumn<int>("line");
    QTest::addColumn<QString>("edge");
    QTest::addColumn<int>("pulses");
    QTest::addColumn<bool>("valid");
    QTest::newRow("buttons") << "manual" << -1 << "falling" << 1 << true;
    QTest::newRow("gpio ok") << "gpio" << 17 << "falling" << 1 << true;
    QTest::newRow("gpio both edges") << "gpio" << 4 << "both" << 2 << true;
    QTest::newRow("gpio without line") << "gpio" << -1 << "falling" << 1 << false;
    QTest::newRow("gpio bad edge") << "gpio" << 17 << "sideways" << 1 << false;
    QTest::newRow("no pulses per row") << "gpio" << 17 << "rising" << 0 << false;
    QTest::newRow("unknown kind") << "usb" << 17 << "rising" << 1 << false;
    QTest::newRow("simulated") << "simulated" << -1 << "falling" << 1 << true;
}

void TstGuider::configIsChecked()
{
    QFETCH(QString, kind); QFETCH(int, line); QFETCH(QString, edge); QFETCH(int, pulses); QFETCH(bool, valid);
    RowCounterConfig c;
    c.kind = kind; c.line = line; c.edge = edge; c.pulsesPerRow = pulses;
    QCOMPARE(c.problem().isEmpty(), valid);
    QString error;
    RowCounterSource *source = createRowCounter(c, &error);
    QCOMPARE(error.isEmpty(), valid);
    QCOMPARE(source != nullptr, valid && kind != QLatin1String("manual"));
    delete source;
}

void TstGuider::configIsSaved()
{
    RowCounterConfig c;
    c.kind = QStringLiteral("gpio"); c.chip = QStringLiteral("gpiochip4"); c.line = 23; c.edge = QStringLiteral("both");
    c.pull = QStringLiteral("down"); c.debounceMs = 12; c.pulsesPerRow = 6;
    QSettings s(QStringLiteral("Stitchly2D"), QStringLiteral("guider"));
    c.save(s);
    RowCounterConfig back;
    back.load(s);
    QCOMPARE(back.kind, c.kind);
    QCOMPARE(back.chip, c.chip);
    QCOMPARE(back.line, 23);
    QCOMPARE(back.edge, c.edge);
    QCOMPARE(back.pull, c.pull);
    QCOMPARE(back.debounceMs, 12);
    QCOMPARE(back.pulsesPerRow, 6);
    RowCounterConfig fresh;
    s.clear();
    fresh.load(s);
    QCOMPARE(fresh.kind, QStringLiteral("manual"));        // nothing saved: buttons only
}

void TstGuider::simulatedCounterEmitsRows()
{
    SimulatedRowCounter c(0, 2);
    QSignalSpy rows(&c, &RowCounterSource::rowsStepped);
    c.injectPulse();                                       // not running yet: ignored
    QVERIFY(c.start());
    QCOMPARE(c.state(), RowCounterSource::State::Running);
    c.injectPulse();
    QCOMPARE(rows.count(), 0);
    c.injectPulse();
    QCOMPARE(rows.count(), 1);
    QCOMPARE(rows.first().first().toInt(), 1);
    c.injectPulse(-1);
    c.injectPulse(-1);
    QCOMPARE(rows.last().first().toInt(), -1);
    c.stop();
    QCOMPARE(c.state(), RowCounterSource::State::Stopped);

    SimulatedRowCounter timed(5, 1);                       // by itself, every 5 ms
    QSignalSpy timedRows(&timed, &RowCounterSource::rowsStepped);
    QVERIFY(timed.start());
    QTRY_VERIFY_WITH_TIMEOUT(timedRows.count() >= 3, 2000);
}

void TstGuider::windowFollowsCounter()
{
    GuideWindow w;
    auto *counter = new SimulatedRowCounter(0, 1);
    w.setRowCounter(counter);                              // before a pattern is open: nothing to move
    QVERIFY(counter->start());
    counter->injectPulse();
    QCOMPARE(w.row(), 0);
    QVERIFY(w.openFile(shirt()));
    counter->injectPulse();
    counter->injectPulse();
    QCOMPARE(w.row(), 2);
    counter->injectPulse(-1);
    QCOMPARE(w.row(), 1);
    w.nextRow();                                           // the buttons still work and the counter carries on from there
    counter->injectPulse();
    QCOMPARE(w.row(), 3);
    for (int i = 0; i < 5; ++i) counter->injectPulse(-1);  // never below the cast-on
    QCOMPARE(w.row(), 0);
    w.setRowCounter(nullptr);
    QVERIFY(!w.rowCounter());
}

void TstGuider::gpioFailsCleanly()
{
    RowCounterConfig c;
    c.kind = QStringLiteral("gpio");
    c.chip = QStringLiteral("/nonexistent/gpiochip99");
    c.line = 17;
    QString error;
    RowCounterSource *source = createRowCounter(c, &error);
    QVERIFY(source);
    QSignalSpy states(source, &RowCounterSource::stateChanged);
    QVERIFY(!source->start(&error));
    QVERIFY(!error.isEmpty());
    QCOMPARE(source->state(), RowCounterSource::State::Error);
    QCOMPARE(states.count(), 1);
    // The window shows the problem and keeps working
    GuideWindow w;
    w.setRowCounter(source);
    QVERIFY(w.openFile(shirt()));
    w.nextRow();
    QCOMPARE(w.row(), 1);
    qInfo() << "GPIO support in this build:" << GpioRowCounter::available();
}

void TstGuider::stepperClamps()
{
    StepperRow r(QStringLiteral("x"), 1, 10, 3);
    QSignalSpy changed(&r, &StepperRow::changed);
    QCOMPARE(r.value(), 1);
    r.setValue(100);
    QCOMPARE(r.value(), 10);
    r.setValue(-4);
    QCOMPARE(r.value(), 1);
    QCOMPARE(changed.count(), 2);
    r.setValue(1);                         // no change, no signal
    QCOMPARE(changed.count(), 2);
    const auto buttons = r.findChildren<QPushButton *>();
    QCOMPARE(buttons.size(), 2);
    buttons.last()->click();               // plus = one step
    QCOMPARE(r.value(), 4);
    buttons.first()->click();
    buttons.first()->click();
    QCOMPARE(r.value(), 1);                // never below the minimum
}

void TstGuider::choiceAddsUnknownValue()
{
    ChoiceRow c(QStringLiteral("x"));
    c.addChoice(QStringLiteral("A"), QStringLiteral("a"));
    c.addChoice(QStringLiteral("B"), QStringLiteral("b"));
    QCOMPARE(c.value(), QStringLiteral("a"));
    c.setValue(QStringLiteral("b"));
    QCOMPARE(c.value(), QStringLiteral("b"));
    c.setValue(QStringLiteral("gpiochip9"));       // e.g. a chip that is not plugged in now
    QCOMPARE(c.value(), QStringLiteral("gpiochip9"));
    QCOMPARE(c.findChildren<QPushButton *>().size(), 3);
}

void TstGuider::settingsDialogRoundTrip()
{
    RowCounterConfig in;
    in.kind = GpioRowCounter::available() ? QStringLiteral("gpio") : QStringLiteral("simulated");
    in.chip = QStringLiteral("gpiochip4"); in.line = 23; in.edge = QStringLiteral("both"); in.pull = QStringLiteral("down");
    in.debounceMs = 12; in.pulsesPerRow = 6; in.simulatedPeriodMs = 300;
    SettingsDialog dialog(in);
    const RowCounterConfig out = dialog.config();
    QCOMPARE(out.kind, in.kind);
    QCOMPARE(out.chip, in.chip);
    QCOMPARE(out.line, 23);
    QCOMPARE(out.edge, in.edge);
    QCOMPARE(out.pull, in.pull);
    QCOMPARE(out.debounceMs, 12);
    QCOMPARE(out.pulsesPerRow, 6);
    QCOMPARE(out.simulatedPeriodMs, 300);
    QVERIFY(out.problem().isEmpty());
    // Defaults of a fresh config give a usable dialog: buttons only, and a GPIO pin is preset
    SettingsDialog fresh{RowCounterConfig()};
    QCOMPARE(fresh.config().kind, QStringLiteral("manual"));
    QVERIFY(fresh.config().line >= 0);
}

void TstGuider::settingsDialogTestCountsPulses()
{
    RowCounterConfig c;
    c.kind = QStringLiteral("simulated"); c.simulatedPeriodMs = 5; c.pulsesPerRow = 2;
    SettingsDialog dialog(c);
    dialog.show();
    QPushButton *test = nullptr;
    for (QPushButton *b : dialog.findChildren<QPushButton *>()) if (b->text() == QLatin1String("Test")) test = b;
    QVERIFY(test);
    QVERIFY(test->isVisible());
    test->click();
    QCOMPARE(test->text(), QStringLiteral("Stop test"));
    bool counted = false;
    QTRY_VERIFY_WITH_TIMEOUT(([&] {
        for (QLabel *l : dialog.findChildren<QLabel *>())
            if (l->text().startsWith(QLatin1String("Listening:"))) {
                const QStringList w = l->text().split(' ');         // "Listening: N pulses, M rows"
                counted = w.value(1).toInt() >= 4 && w.value(3).toInt() >= 2;
            }
        return counted;
    }()), 3000);
    test->click();
    QCOMPARE(test->text(), QStringLiteral("Test"));
    // A test that cannot start shows why and stays stopped
    RowCounterConfig bad;
    bad.kind = QStringLiteral("gpio"); bad.chip = QStringLiteral("/nonexistent/gpiochip99"); bad.line = 5;
    if (GpioRowCounter::available())
    {
        SettingsDialog d2(bad);
        d2.show();
        QPushButton *t2 = nullptr;
        for (QPushButton *b : d2.findChildren<QPushButton *>()) if (b->text() == QLatin1String("Test")) t2 = b;
        QVERIFY(t2);
        t2->click();
        QCOMPARE(t2->text(), QStringLiteral("Test"));
        bool shown = false;
        for (QLabel *l : d2.findChildren<QLabel *>()) shown = shown || l->text().contains(QLatin1String("/nonexistent/gpiochip99"));
        QVERIFY(shown);
    }
}

void TstGuider::settingsDialogFitsSmallScreen()
{
    qputenv("GUIDER_UNIT", "9");
    applyGuideStyle(*qApp);
    RowCounterConfig c;
    c.kind = GpioRowCounter::available() ? QStringLiteral("gpio") : QStringLiteral("simulated");
    SettingsDialog dialog(c);
    dialog.resize(800, 480);
    dialog.show();
    QVERIFY(QTest::qWaitForWindowExposed(&dialog));
    // OK and Cancel are always reachable, whatever the page holds
    int reachable = 0;
    for (QPushButton *b : dialog.findChildren<QPushButton *>())
    {
        if ((b->text() == QLatin1String("OK") || b->text() == QLatin1String("Cancel")))
        {
            const QRect r(b->mapTo(&dialog, QPoint()), b->size());
            QVERIFY(dialog.rect().contains(r));
            QVERIFY(r.height() >= 9 * 2);
            ++reachable;
        }
    }
    QCOMPARE(reachable, 2);
    // GUIDER_TEST_SHOTS=dir saves a picture of the dialog, for a look by eye
    if (qEnvironmentVariableIsSet("GUIDER_TEST_SHOTS"))
        dialog.grab().save(qEnvironmentVariable("GUIDER_TEST_SHOTS") + QStringLiteral("/settings_800x480.png"));
    qunsetenv("GUIDER_UNIT");
}

void TstGuider::applyConfigReplacesCounter()
{
    GuideWindow w;
    QVERIFY(w.openFile(shirt()));
    QVERIFY(!w.rowCounter());
    RowCounterConfig sim;
    sim.kind = QStringLiteral("simulated"); sim.simulatedPeriodMs = 0;
    QString error;
    QVERIFY2(w.applyCounterConfig(sim, true, &error), qPrintable(error));
    auto *counter = qobject_cast<SimulatedRowCounter *>(w.rowCounter());
    QVERIFY(counter);
    QCOMPARE(counter->state(), RowCounterSource::State::Running);
    counter->injectPulse();
    QCOMPARE(w.row(), 1);
    // Saved for the next start
    RowCounterConfig back;
    QSettings s(QStringLiteral("Stitchly2D"), QStringLiteral("guider"));
    back.load(s);
    QCOMPARE(back.kind, QStringLiteral("simulated"));
    // A wrong config is refused and changes nothing
    RowCounterConfig bad;
    bad.kind = QStringLiteral("gpio");          // no line
    QVERIFY(!w.applyCounterConfig(bad, true, &error));
    QVERIFY(!error.isEmpty());
    QVERIFY(w.rowCounter() == counter);
    back.load(s);
    QCOMPARE(back.kind, QStringLiteral("simulated"));
    // Buttons only removes the counter
    QVERIFY(w.applyCounterConfig(RowCounterConfig(), false, &error));
    QVERIFY(!w.rowCounter());
    QCOMPARE(w.counterConfig().kind, QStringLiteral("manual"));
}

void TstGuider::themeNamesRoundTrip()
{
    for (ThemeChoice c : {ThemeChoice::System, ThemeChoice::Light, ThemeChoice::Dark})
        QCOMPARE(themeChoiceFromName(themeChoiceName(c)), c);
    QCOMPARE(themeChoiceFromName(QStringLiteral("nonsense")), ThemeChoice::System);
    QCOMPARE(themeChoiceFromName(QString()), ThemeChoice::System);
    const QColor mid = blendColors(Qt::black, Qt::white, 0.5);
    QVERIFY(qAbs(mid.red() - 128) <= 1);
    QCOMPARE(blendColors(Qt::red, Qt::blue, 0.0), QColor(Qt::red));
    QCOMPARE(blendColors(Qt::red, Qt::blue, 1.0), QColor(Qt::blue));
}

void TstGuider::themeFollowsSystem()
{
    QCOMPARE(guideThemeChoice(), ThemeChoice::System);         // the default
    QVERIFY(guideThemeIsDark());                               // the system says nothing: dark, the look guider always had
    QSignalSpy changed(GuideThemeNotifier::instance(), &GuideThemeNotifier::changed);
    pretendSystemColorScheme(Qt::ColorScheme::Light);
    QVERIFY(!guideThemeIsDark());
    QCOMPARE(guideTheme().window, lightGuideTheme().window);
    QVERIFY(changed.count() >= 1);                              // following: restyled when the system switched
    changed.clear();
    pretendSystemColorScheme(Qt::ColorScheme::Dark);
    QVERIFY(guideThemeIsDark());
    QCOMPARE(guideTheme().window, darkGuideTheme().window);
    QVERIFY(changed.count() >= 1);
    // A fixed choice ignores the system and is not restyled by it
    setGuideThemeChoice(ThemeChoice::Light);
    changed.clear();
    pretendSystemColorScheme(Qt::ColorScheme::Dark);
    QVERIFY(!guideThemeIsDark());
    QCOMPARE(changed.count(), 0);
    setGuideThemeChoice(ThemeChoice::System);
    QVERIFY(guideThemeIsDark());                                // back to following: the pretended dark
    pretendSystemColorScheme(Qt::ColorScheme::Unknown, false);
}

void TstGuider::themeChoiceIsSavedAndApplied()
{
    qputenv("GUIDER_UNIT", "20");
    setGuideThemeChoice(ThemeChoice::Dark);
    QCOMPARE(guideThemeChoice(), ThemeChoice::Dark);
    QCOMPARE(QSettings(QStringLiteral("Stitchly2D"), QStringLiteral("guider")).value(QStringLiteral("theme")).toString(), QStringLiteral("dark"));
    QVERIFY(qApp->styleSheet().contains(darkGuideTheme().window.name()));
    setGuideThemeChoice(ThemeChoice::Light);
    QVERIFY(qApp->styleSheet().contains(lightGuideTheme().window.name()));
    QVERIFY(!qApp->styleSheet().contains(darkGuideTheme().window.name()));
    // The rolling text repaints in the new colors
    GuideWindow w;
    QVERIFY(w.openFile(shirt()));
    w.resize(1000, 600);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    auto *view = w.findChild<StepView *>();
    QCOMPARE(view->grab().toImage().pixelColor(2, view->height() - 2), lightGuideTheme().view);
    setGuideThemeChoice(ThemeChoice::Dark);
    QCOMPARE(view->grab().toImage().pixelColor(2, view->height() - 2), darkGuideTheme().view);
    setGuideThemeChoice(ThemeChoice::System);
    qunsetenv("GUIDER_UNIT");
}

void TstGuider::buttonsStandOutInEveryTheme_data()
{
    QTest::addColumn<int>("choice");
    QTest::newRow("dark") << int(ThemeChoice::Dark);
    QTest::newRow("light") << int(ThemeChoice::Light);
}

void TstGuider::buttonsStandOutInEveryTheme()
{
    QFETCH(int, choice);
    qputenv("GUIDER_UNIT", "20");
    setGuideThemeChoice(ThemeChoice(choice));
    // A button must be easy to tell from the window around it: contrast of the button and its border against the dialog
    auto luminance = [](const QColor &c) { return 0.2126 * c.redF() + 0.7152 * c.greenF() + 0.0722 * c.blueF(); };
    const GuideTheme t = guideTheme();
    QVERIFY2(qAbs(luminance(t.button) - luminance(t.window)) >= 0.06, "button color too close to the window");
    QVERIFY2(qAbs(luminance(t.border) - luminance(t.window)) >= 0.3, "button border too faint");
    QVERIFY2(qAbs(luminance(t.text) - luminance(t.window)) >= 0.6, "text contrast");
    QVERIFY2(qAbs(luminance(t.text) - luminance(t.button)) >= 0.5, "button text contrast");
    QVERIFY2(qAbs(luminance(t.accentText) - luminance(t.accent)) >= 0.4, "chosen choice contrast");
    // and it is really painted that way: the Browse button in the open dialog differs from the dialog background
    OpenDialog open({}, QString());
    open.show();
    QVERIFY(QTest::qWaitForWindowExposed(&open));
    QPushButton *browse = nullptr;
    for (QPushButton *b : open.findChildren<QPushButton *>()) if (b->text().startsWith(QLatin1String("Browse"))) browse = b;
    QVERIFY(browse);
    const QImage image = open.grab().toImage();
    const QPoint inside = browse->mapTo(&open, QPoint(browse->width() / 2, browse->height() - 6));
    QCOMPARE(image.pixelColor(inside), t.button);
    QCOMPARE(image.pixelColor(QPoint(2, inside.y())), t.window);                // the margin left of the button
    qunsetenv("GUIDER_UNIT");
    setGuideThemeChoice(ThemeChoice::System);
}

void TstGuider::settingsDialogChoosesTheme()
{
    setGuideThemeChoice(ThemeChoice::Light);
    SettingsDialog dialog{RowCounterConfig()};
    QCOMPARE(dialog.themeChoice(), ThemeChoice::Light);         // shows what is set now
    dialog.show();
    QPushButton *dark = nullptr;
    for (QPushButton *b : dialog.findChildren<QPushButton *>()) if (b->text() == QLatin1String("Dark")) dark = b;
    QVERIFY(dark);
    dark->click();
    QCOMPARE(dialog.themeChoice(), ThemeChoice::Dark);
    QCOMPARE(guideThemeChoice(), ThemeChoice::Light);           // only OK applies it
    setGuideThemeChoice(ThemeChoice::System);
}

void TstGuider::looksOfDialogs()
{
    // Pictures for a look by eye: GUIDER_TEST_SHOTS=dir
    if (!qEnvironmentVariableIsSet("GUIDER_TEST_SHOTS"))
        QSKIP("set GUIDER_TEST_SHOTS to a directory to save pictures");
    const QString dir = qEnvironmentVariable("GUIDER_TEST_SHOTS");
    qputenv("GUIDER_UNIT", "20");
    QFile::copy(shirt(), dir + QStringLiteral("/recent.sm2d"));
    for (ThemeChoice choice : {ThemeChoice::Dark, ThemeChoice::Light})
    {
        setGuideThemeChoice(choice);
        const QString name = themeChoiceName(choice);
        OpenDialog open({dir + QStringLiteral("/recent.sm2d"), shirt()}, dir);
        open.show();
        QVERIFY(QTest::qWaitForWindowExposed(&open));
        open.grab().save(dir + QStringLiteral("/open_") + name + QStringLiteral(".png"));
        GuideWindow w;
        QVERIFY(w.openFile(shirt()));
        RowCounterConfig sim;
        sim.kind = QStringLiteral("simulated"); sim.simulatedPeriodMs = 0;
        w.applyCounterConfig(sim, false);
        w.setRow(3);
        w.resize(1280, 720);
        w.show();
        QVERIFY(QTest::qWaitForWindowExposed(&w));
        w.grab().save(dir + QStringLiteral("/window_") + name + QStringLiteral(".png"));
        QFileDialog files(nullptr, QStringLiteral("Open"), dir);
        files.setOption(QFileDialog::DontUseNativeDialog);
        files.resize(900, 600);
        files.show();
        QVERIFY(QTest::qWaitForWindowExposed(&files));
        files.grab().save(dir + QStringLiteral("/files_") + name + QStringLiteral(".png"));
        files.close();
        SettingsDialog settings{RowCounterConfig()};
        settings.show();
        QVERIFY(QTest::qWaitForWindowExposed(&settings));
        settings.grab().save(dir + QStringLiteral("/settings_") + name + QStringLiteral(".png"));
    }
    setGuideThemeChoice(ThemeChoice::System);
    qunsetenv("GUIDER_UNIT");
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
