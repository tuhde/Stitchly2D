#include <QtTest>

#include "stitchsize.h"
#include "stitchgrid.h"
#include "stitchtype.h"
#include "catalog.h"
#include "patternfile.h"
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
    void patternRepeat();
    void catalogDefaults();
    void catalogJson();
    void overlayColorStripes();
    void overlaySpecialAndColor();
    void overlayRegionAndOffset();
    void overlayUnknownPattern();
    void patternRowsTopFirst();
    void fileRoundTrip();
    void fileErrors();
    void importPolicies();
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

static QPolygonF rect(qreal w, qreal h) { return QPolygonF({{0, 0}, {w, 0}, {w, h}, {0, h}}); }

void TstStitchCore::patternRepeat()
{
    PatternDef p;
    p.name = "x"; p.width = 2; p.height = 2; p.legend = {"k", "p"}; p.cells = {0, 1, 1, 0};
    QVERIFY(p.isValid());
    QCOMPARE(p.cellAt(0, 0), 0);
    QCOMPARE(p.cellAt(1, 0), 1);
    QCOMPARE(p.cellAt(2, 0), 0);     // wraps
    QCOMPARE(p.cellAt(-1, 0), 1);    // negative wraps too
    QCOMPARE(p.cellAt(0, 3), 1);
    p.cells = {0, 1, 2, 0};          // index outside the legend
    QVERIFY(!p.isValid());
    p.cells = {0, 1};                // wrong size
    QVERIFY(!p.isValid());
    QCOMPARE(p.cellAt(0, 0), -1);
}

void TstStitchCore::catalogDefaults()
{
    StitchCatalog c = StitchCatalog::withDefaults();
    QVERIFY(c.basics().contains("1:1 rib"));
    QVERIFY(c.basics().contains("Patent"));
    QVERIFY(c.contains("Seed stitch"));
    QCOMPARE(c.patterns(PatternDef::Color).size(), 2);
    QCOMPARE(c.patterns(PatternDef::Special).size(), 1);
    PatternDef bad; bad.name = "bad";
    QVERIFY(!c.set(bad));            // empty cells
    QVERIFY(c.remove("Checks 2x2"));
    QVERIFY(!c.contains("Checks 2x2"));
}

void TstStitchCore::catalogJson()
{
    const StitchCatalog c = StitchCatalog::withDefaults();
    StitchCatalog back;
    QVERIFY(StitchCatalog::fromJson(c.toJson(), back));
    QCOMPARE(back.basics().types().size(), c.basics().types().size());
    QCOMPARE(back.patterns().size(), c.patterns().size());
    QCOMPARE(back.find("Checks 2x2").cells, c.find("Checks 2x2").cells);
    QCOMPARE(back.basics().find("Patent").size.rowHeightMm, 4.5);

    QTemporaryDir dir;
    const QString path = dir.filePath("catalog.json");
    QVERIFY(c.save(path));
    StitchCatalog loaded;
    QVERIFY(StitchCatalog::load(path, loaded));
    QCOMPARE(loaded.patterns().size(), c.patterns().size());
    QVERIFY(!StitchCatalog::load(dir.filePath("missing.json"), loaded));

    QJsonObject broken = c.toJson();
    broken["version"] = 2;
    QVERIFY(!StitchCatalog::fromJson(broken, loaded));
    QJsonObject badCell = c.toJson();
    QJsonArray pats = badCell["patterns"].toArray();
    QJsonObject p0 = pats[0].toObject();
    p0["rows"] = QJsonArray{QJsonArray{9, 9}, QJsonArray{9, 9}};
    pats[0] = p0;
    badCell["patterns"] = pats;
    QVERIFY(!StitchCatalog::fromJson(badCell, loaded));
}

void TstStitchCore::overlayColorStripes()
{
    const StitchCatalog cat = StitchCatalog::withDefaults();
    BandSpec band; band.stitchType = "Stockinette";
    band.overlays.append(Overlay{"Stripes 2 rows"});
    const StitchGrid grid = StitchGrid::build(rect(100, 70), {band}, cat);   // 20 sts x 20 rows
    QCOMPARE(grid.rowCount(), 20);
    QCOMPARE(grid.cell(0, 0).color, QString("A"));
    QCOMPARE(grid.cell(1, 5).color, QString("A"));
    QCOMPARE(grid.cell(2, 0).color, QString("B"));
    QCOMPARE(grid.cell(4, 0).color, QString("A"));
    QCOMPARE(grid.cell(0, 0).basic, QString("Stockinette"));
    QVERIFY(grid.cell(0, 0).special.isEmpty());
    const QVector<Run> runs = grid.overlayRuns(2);
    QCOMPARE(runs.size(), 1);
    QCOMPARE(runs.first().count, 20);
    QCOMPARE(runs.first().color, QString("B"));
    const QString text = writeInstructions(grid).join('\n');
    QVERIFY(text.contains("Row 1: A x20."));
    QVERIFY(text.contains("Row 3: B x20."));
}

void TstStitchCore::overlaySpecialAndColor()
{
    const StitchCatalog cat = StitchCatalog::withDefaults();
    BandSpec band; band.stitchType = "Stockinette";
    band.overlays.append(Overlay{"Seed stitch"});
    band.overlays.append(Overlay{"Stripes 2 rows"});
    const StitchGrid grid = StitchGrid::build(rect(50, 35), {band}, cat);    // 10 sts x 10 rows
    const Cell c = grid.cell(0, 0);
    QCOMPARE(c.special, QString("k"));
    QCOMPARE(c.color, QString("A"));
    QCOMPARE(grid.cell(0, 1).special, QString("p"));
    QCOMPARE(grid.cell(1, 0).special, QString("p"));
    QVERIFY(writeInstructions(grid).join('\n').contains("A/k x1, A/p x1"));
}

void TstStitchCore::overlayRegionAndOffset()
{
    const StitchCatalog cat = StitchCatalog::withDefaults();
    BandSpec band; band.stitchType = "Stockinette";
    Overlay o{"Seed stitch"};
    o.firstRow = 2; o.rowCount = 3; o.firstStitch = 4; o.stitchCount = 6;
    band.overlays.append(o);
    const StitchGrid grid = StitchGrid::build(rect(100, 70), {band}, cat);   // 20 x 20
    QVERIFY(grid.cell(1, 4).special.isEmpty());      // before the rows
    QVERIFY(grid.cell(5, 4).special.isEmpty());      // after the rows
    QVERIFY(grid.cell(3, 3).special.isEmpty());      // left of the region
    QVERIFY(grid.cell(3, 10).special.isEmpty());     // right of the region
    QCOMPARE(grid.cell(2, 4).special, QString("k")); // repeat starts at the region corner
    QCOMPARE(grid.cell(2, 5).special, QString("p"));
    QVERIFY(grid.overlayRuns(0).isEmpty());          // untouched row
    QVERIFY(!grid.overlayRuns(3).isEmpty());
    // plain rows stay grouped around the overlay rows
    const QString text = writeInstructions(grid).join('\n');
    QVERIFY(text.contains("Rows 1-2: work even."));
    QVERIFY(text.contains("Rows 6-20: work even."));

    Overlay shifted = o;
    shifted.offsetX = 1;
    BandSpec band2; band2.stitchType = "Stockinette"; band2.overlays.append(shifted);
    QCOMPARE(StitchGrid::build(rect(100, 70), {band2}, cat).cell(2, 4).special, QString("p"));
}

void TstStitchCore::overlayUnknownPattern()
{
    BandSpec band; band.stitchType = "Stockinette"; band.overlays.append(Overlay{"nope"});
    QVERIFY(StitchGrid::build(rect(100, 70), {band}, StitchCatalog::withDefaults()).isEmpty());
}

void TstStitchCore::patternRowsTopFirst()
{
    PatternDef p;
    p.name = "t"; p.width = 2; p.height = 3; p.legend = {"a", "b", "c"};
    p.cells = {0, 0,     // bottom row (y = 0)
               1, 1,
               2, -1};   // top row
    QVERIFY(p.isValid());
    const QJsonArray rows = p.toJson()["rows"].toArray();
    QCOMPARE(rows.size(), 3);
    QCOMPARE(rows.at(0).toArray().at(0).toInt(), 2);    // first row in the file = top row
    QCOMPARE(rows.at(0).toArray().at(1).toInt(), -1);
    QCOMPARE(rows.at(2).toArray().at(0).toInt(), 0);
    PatternDef back;
    QVERIFY(PatternDef::fromJson(p.toJson(), back));
    QCOMPARE(back.cells, p.cells);
}

void TstStitchCore::fileRoundTrip()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("set.stpat");
    const StitchCatalog c = StitchCatalog::withDefaults();
    PatternFileInfo info; info.author = "Tester"; info.description = "starter set";
    QVERIFY(exportPatterns(path, c.patterns(), info));

    QVector<PatternDef> back;
    PatternFileInfo backInfo;
    QString err;
    QVERIFY2(importPatterns(path, back, &backInfo, &err), qPrintable(err));
    QCOMPARE(back.size(), c.patterns().size());
    QCOMPARE(backInfo.author, QString("Tester"));
    QCOMPARE(backInfo.description, QString("starter set"));
    for (int i = 0; i < back.size(); ++i)
    {
        QCOMPARE(back.at(i).name, c.patterns().at(i).name);
        QCOMPARE(back.at(i).cells, c.patterns().at(i).cells);
        QCOMPARE(int(back.at(i).kind), int(c.patterns().at(i).kind));
    }
    // a single pattern works too, and the file is plain JSON
    QVERIFY(exportPatterns(dir.filePath("one.stpat"), {c.find("Seed stitch")}));
    QFile f(dir.filePath("one.stpat")); QVERIFY(f.open(QIODevice::ReadOnly));
    QVERIFY(QJsonDocument::fromJson(f.readAll()).isObject());
    QCOMPARE(QString(kPatternFileExtension), QString("stpat"));
}

static QString writeText(const QTemporaryDir &dir, const QString &name, const QByteArray &data)
{
    const QString path = dir.filePath(name);
    QFile f(path); f.open(QIODevice::WriteOnly); f.write(data);
    return path;
}

void TstStitchCore::fileErrors()
{
    QTemporaryDir dir;
    QVector<PatternDef> out; QString err;
    QVERIFY(!importPatterns(dir.filePath("missing.stpat"), out, nullptr, &err));
    QVERIFY(!err.isEmpty());
    QVERIFY(!importPatterns(writeText(dir, "a", "not json"), out, nullptr, &err));
    QVERIFY(!importPatterns(writeText(dir, "b", R"({"format":"other","version":1,"patterns":[]})"), out, nullptr, &err));
    QVERIFY(!importPatterns(writeText(dir, "c", R"({"format":"stitchly-patterns","version":99,"patterns":[{}]})"), out, nullptr, &err));
    QVERIFY(err.contains("newer"));
    QVERIFY(!importPatterns(writeText(dir, "d", R"({"format":"stitchly-patterns","version":1,"patterns":[]})"), out, nullptr, &err));
    const QByteArray ragged = R"({"format":"stitchly-patterns","version":1,"patterns":[
        {"name":"r","kind":"special","width":2,"height":2,"legend":["k","p"],"rows":[[0,1],[0]]}]})";
    QVERIFY(!importPatterns(writeText(dir, "e", ragged), out, nullptr, &err));
    QVERIFY(err.contains("row 2"));
    const QByteArray badIndex = R"({"format":"stitchly-patterns","version":1,"patterns":[
        {"name":"r","kind":"color","width":1,"height":1,"legend":["A"],"rows":[[5]]}]})";
    QVERIFY(!importPatterns(writeText(dir, "f", badIndex), out, nullptr, &err));
    const QByteArray dup = R"({"format":"stitchly-patterns","version":1,"patterns":[
        {"name":"r","kind":"color","width":1,"height":1,"legend":["A"],"rows":[[0]]},
        {"name":"r","kind":"color","width":1,"height":1,"legend":["A"],"rows":[[0]]}]})";
    QVERIFY(!importPatterns(writeText(dir, "g", dup), out, nullptr, &err));
    QVERIFY(err.contains("two patterns"));
    QVERIFY(out.isEmpty());      // failed imports leave the output alone
    // export refuses empty, invalid and duplicate input
    QVERIFY(!exportPatterns(dir.filePath("x.stpat"), {}));
    PatternDef bad; bad.name = "bad";
    QVERIFY(!exportPatterns(dir.filePath("x.stpat"), {bad}));
    const PatternDef ok = StitchCatalog::withDefaults().find("Seed stitch");
    QVERIFY(!exportPatterns(dir.filePath("x.stpat"), {ok, ok}));
    QVERIFY(!QFile::exists(dir.filePath("x.stpat")));
}

void TstStitchCore::importPolicies()
{
    const StitchCatalog base = StitchCatalog::withDefaults();
    PatternDef seed = base.find("Seed stitch");
    seed.legend = {"K", "P"};                         // same name, different content
    PatternDef fresh = seed; fresh.name = "Fresh";

    StitchCatalog c = base;
    ImportResult r = addToCatalog(c, {seed, fresh}, ConflictPolicy::Skip);
    QCOMPARE(r.added, 1); QCOMPARE(r.skipped, 1);
    QCOMPARE(c.find("Seed stitch").legend.first(), QString("k"));

    c = base;
    r = addToCatalog(c, {seed, fresh}, ConflictPolicy::Replace);
    QCOMPARE(r.replaced, 1); QCOMPARE(r.added, 1);
    QCOMPARE(c.find("Seed stitch").legend.first(), QString("K"));
    QCOMPARE(c.patterns().size(), base.patterns().size() + 1);

    c = base;
    r = addToCatalog(c, {seed}, ConflictPolicy::Rename);
    QCOMPARE(r.renamed, 1); QCOMPARE(r.added, 1);
    QVERIFY(c.contains("Seed stitch (2)"));
    r = addToCatalog(c, {seed}, ConflictPolicy::Rename);
    QVERIFY(c.contains("Seed stitch (3)"));            // no clash with the first rename
    QCOMPARE(c.find("Seed stitch").legend.first(), QString("k"));
}

QTEST_APPLESS_MAIN(TstStitchCore)
#include "tst_stitchcore.moc"
