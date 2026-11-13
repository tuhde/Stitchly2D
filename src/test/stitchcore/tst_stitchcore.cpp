#include <QtTest>

#include "stitchsize.h"
#include "stitchgrid.h"
#include "stitchtype.h"
#include "shaping.h"
#include "instructions.h"

using namespace stitchcore;

class TstStitchCore : public QObject
{
    Q_OBJECT
private slots:
    void derivedGauges();
    void fromGauge();
    void rectangle();
    void trapezoid();
    void invalidInput();
    void instructionsText();
    void csv();
    void library();
    void bandsRibThenStockinette();
    void bandsDifferentRowHeights();
    void bandsUnknownType();
};

static StitchSize g20x30() { StitchSize s; s.stitchWidthMm = 5.0; s.rowHeightMm = 100.0 / 30; return s; }

void TstStitchCore::derivedGauges()
{
    const StitchSize s = g20x30();
    QCOMPARE(s.stitchesPer10cm(), 20.0);
    QVERIFY(qAbs(s.rowsPer10cm() - 30.0) < 1e-9);
    QVERIFY(qAbs(s.stitchesPerInch() - 5.08) < 1e-9);
    QVERIFY(qAbs(s.rowsPerInch() - 7.62) < 1e-9);
    StitchSize bad; bad.stitchWidthMm = 0;
    QVERIFY(!bad.isValid());
    QCOMPARE(bad.stitchesPer10cm(), 0.0);
}

void TstStitchCore::fromGauge()
{
    const StitchSize a = StitchSize::fromPer10cm(20, 25);
    QCOMPARE(a.stitchWidthMm, 5.0);
    QCOMPARE(a.rowHeightMm, 4.0);
    const StitchSize b = StitchSize::fromPerInch(5, 7);
    QVERIFY(qAbs(b.stitchWidthMm - 5.08) < 1e-9);
    QVERIFY(qAbs(b.stitchesPer10cm() - 19.685) < 0.01);
    QVERIFY(!StitchSize::fromPer10cm(0, 10).isValid());
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
    StitchSize bad; bad.rowHeightMm = 0;
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
    QVERIFY(csv.startsWith(QLatin1String("row;band;stitch type;first stitch;count")));
    QCOMPARE(csv.count('\n'), 10); // header + 9 rows
}

void TstStitchCore::library()
{
    StitchLibrary lib = StitchLibrary::withDefaults();
    QVERIFY(lib.contains("Stockinette"));
    QVERIFY(!lib.find("2x2 rib").name.isEmpty());
    QVERIFY(lib.find("nope").name.isEmpty());
    const int n = lib.types().size();
    lib.set({"Stockinette", StitchSize{4.0, 3.0}});   // replace, not add
    QCOMPARE(lib.types().size(), n);
    QCOMPARE(lib.find("Stockinette").size.stitchWidthMm, 4.0);
    QVERIFY(lib.remove("Garter"));
    QVERIFY(!lib.remove("Garter"));
}

void TstStitchCore::bandsRibThenStockinette()
{
    // 200 x 300 mm rectangle: 60 mm of rib (4 mm wide, 3 mm high), rest stockinette (5 mm wide, 3 mm high)
    StitchLibrary lib;
    lib.set({"rib", StitchSize{4.0, 3.0}});
    lib.set({"st", StitchSize{5.0, 3.0}});
    const QPolygonF rect({{0, 0}, {200, 0}, {200, 300}, {0, 300}});
    const StitchGrid grid = StitchGrid::build(rect, {{"rib", 60}, {"st", 0}}, lib);
    QCOMPARE(grid.bands().size(), 2);
    QCOMPARE(grid.bands().at(0).rowCount, 20);
    QCOMPARE(grid.bands().at(1).rowCount, 80);
    QCOMPARE(grid.castOn(), 50);          // 200 / 4
    QCOMPARE(grid.bindOff(), 40);         // 200 / 5
    QCOMPARE(grid.rows().at(19).count, 50);
    QCOMPARE(grid.rows().at(20).count, 40);
    QCOMPARE(grid.rows().at(20).band, 1);

    // Only one shaping entry: the band change, not an edge change.
    const QVector<Shaping> sh = planShaping(grid);
    QCOMPARE(sh.size(), 1);
    QVERIFY(sh.first().bandChange);
    QCOMPARE(sh.first().row, 20);
    QCOMPARE(sh.first().stitchesBefore, 50);
    QCOMPARE(sh.first().stitchesAfter, 40);

    const QString text = writeInstructions(grid).join('\n');
    QVERIFY(text.contains("change to st"));
    QVERIFY(text.contains("decrease 10 stitches evenly"));
    QVERIFY(text.contains("(rib)"));
    QVERIFY(rowTableCsv(grid).contains(";rib;"));
}

void TstStitchCore::bandsDifferentRowHeights()
{
    StitchLibrary lib;
    lib.set({"a", StitchSize{5.0, 2.0}});
    lib.set({"b", StitchSize{5.0, 6.0}});
    const QPolygonF rect({{0, 0}, {100, 0}, {100, 120}, {0, 120}});
    const StitchGrid grid = StitchGrid::build(rect, {{"a", 60}, {"b", 60}}, lib);
    QCOMPARE(grid.bands().at(0).rowCount, 30);
    QCOMPARE(grid.bands().at(1).rowCount, 10);
    QCOMPARE(grid.rowCount(), 40);
    QVERIFY(planShaping(grid).first().bandChange);
    QCOMPARE(planShaping(grid).first().stitchesBefore, planShaping(grid).first().stitchesAfter);
}

void TstStitchCore::bandsUnknownType()
{
    const QPolygonF rect({{0, 0}, {100, 0}, {100, 100}, {0, 100}});
    QVERIFY(StitchGrid::build(rect, {{"missing", 0}}, StitchLibrary::withDefaults()).isEmpty());
    QVERIFY(StitchGrid::build(rect, QVector<BandSpec>(), StitchLibrary::withDefaults()).isEmpty());
}

QTEST_APPLESS_MAIN(TstStitchCore)
#include "tst_stitchcore.moc"
