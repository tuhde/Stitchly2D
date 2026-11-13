#include <QtTest>

#include "gauge.h"
#include "stitchgrid.h"
#include "shaping.h"
#include "instructions.h"

using namespace stitchcore;

class TstStitchCore : public QObject
{
    Q_OBJECT
private slots:
    void gaugeSizes();
    void gaugePerInch();
    void rectangle();
    void trapezoid();
    void invalidInput();
    void instructionsText();
    void csv();
};

static Gauge g20x30() { Gauge g; g.stitchesPer10cm = 20; g.rowsPer10cm = 30; return g; }

void TstStitchCore::gaugeSizes()
{
    QCOMPARE(g20x30().stitchWidthMm(), 5.0);
    QVERIFY(qAbs(g20x30().rowHeightMm() - 100.0 / 30) < 1e-9);
    Gauge bad; bad.stitchesPer10cm = 0;
    QVERIFY(!bad.isValid());
}

void TstStitchCore::gaugePerInch()
{
    const Gauge g = Gauge::fromPerInch(5, 7);
    QVERIFY(qAbs(g.stitchesPer10cm - 19.685) < 0.01);
}

void TstStitchCore::rectangle()
{
    // 200 mm x 300 mm at 20 sts / 30 rows per 10 cm -> 40 sts, 90 rows
    const QPolygonF rect({{0, 0}, {200, 0}, {200, 300}, {0, 300}});
    const StitchGrid grid = StitchGrid::build(rect, g20x30());
    QCOMPARE(grid.rowCount(), 90);
    QCOMPARE(grid.castOn(), 40);
    QCOMPARE(grid.bindOff(), 40);
    QCOMPARE(grid.width(), 40);
    QVERIFY(planShaping(grid).isEmpty());
    const QVector<Segment> segs = planSegments(grid);
    QCOMPARE(segs.size(), 1);
    QVERIFY(!segs.first().shaped);
    QCOMPARE(segs.first().rows, 90);
}

void TstStitchCore::trapezoid()
{
    // 100 mm wide at the bottom, 200 mm at the top, 300 mm high, centred.
    const QPolygonF trap({{50, 0}, {150, 0}, {200, 300}, {0, 300}});
    const StitchGrid grid = StitchGrid::build(trap, g20x30());
    QVERIFY(grid.castOn() >= 19 && grid.castOn() <= 21);
    QVERIFY(grid.bindOff() >= 39 && grid.bindOff() <= 41);
    QVERIFY(grid.bindOff() > grid.castOn());
    int net = 0;
    for (const Shaping &s : planShaping(grid))
    {
        QVERIFY(s.left >= 0 && s.right >= 0); // only increases
        net += s.left + s.right;
    }
    QCOMPARE(net, grid.bindOff() - grid.castOn());
}

void TstStitchCore::invalidInput()
{
    QVERIFY(StitchGrid::build(QPolygonF(), g20x30()).isEmpty());
    Gauge bad; bad.rowsPer10cm = 0;
    QVERIFY(StitchGrid::build(QPolygonF({{0, 0}, {10, 0}, {10, 10}}), bad).isEmpty());
    QVERIFY(writeInstructions(StitchGrid()).isEmpty());
}

void TstStitchCore::instructionsText()
{
    const QPolygonF trap({{50, 0}, {150, 0}, {200, 300}, {0, 300}});
    const QStringList lines = writeInstructions(StitchGrid::build(trap, g20x30()));
    QVERIFY(lines.first().startsWith(QLatin1String("Cast on")));
    QVERIFY(lines.last().startsWith(QLatin1String("Bind off")));
    QVERIFY(lines.join('\n').contains(QLatin1String("increase")));
    QVERIFY(lines.join('\n').contains(QLatin1String("M1")));
}

void TstStitchCore::csv()
{
    const QPolygonF rect({{0, 0}, {100, 0}, {100, 30}, {0, 30}});
    const QString csv = rowTableCsv(StitchGrid::build(rect, g20x30()));
    QVERIFY(csv.startsWith(QLatin1String("row;first stitch;count")));
    QCOMPARE(csv.count('\n'), 10); // header + 9 rows
}

QTEST_APPLESS_MAIN(TstStitchCore)
#include "tst_stitchcore.moc"
