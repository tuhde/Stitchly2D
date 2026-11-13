#include "project.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

namespace stitchcore
{

const char *const kProjectFileExtension = "stitchly";
const char *const kDefaultStitchName = "Default stitch";

namespace
{
const char *const formatId = "stitchly-project";

bool fail(QString *error, const QString &msg)
{
    if (error) *error = msg;
    return false;
}

QString toRelative(const QString &projectPath, const QString &file)
{
    if (file.isEmpty())
    {
        return QString();
    }
    return QDir(QFileInfo(projectPath).absolutePath()).relativeFilePath(file);
}

QString toAbsolute(const QString &projectPath, const QString &file)
{
    if (file.isEmpty())
    {
        return QString();
    }
    return QDir::cleanPath(QDir(QFileInfo(projectPath).absolutePath()).absoluteFilePath(file));
}

QString increaseName(IncreaseMethod m)
{
    switch (m)
    {
        case IncreaseMethod::M1:       return QStringLiteral("m1");
        case IncreaseMethod::YarnOver: return QStringLiteral("yarnover");
        case IncreaseMethod::KFB:      return QStringLiteral("kfb");
    }
    return QStringLiteral("m1");
}

bool increaseFromName(const QString &s, IncreaseMethod &m)
{
    if (s == QLatin1String("m1")) m = IncreaseMethod::M1;
    else if (s == QLatin1String("yarnover")) m = IncreaseMethod::YarnOver;
    else if (s == QLatin1String("kfb")) m = IncreaseMethod::KFB;
    else return false;
    return true;
}

QString decreaseName(DecreaseMethod m)
{
    return m == DecreaseMethod::K2tog ? QStringLiteral("k2tog") : QStringLiteral("k2tog_ssk");
}

bool decreaseFromName(const QString &s, DecreaseMethod &m)
{
    if (s == QLatin1String("k2tog")) m = DecreaseMethod::K2tog;
    else if (s == QLatin1String("k2tog_ssk")) m = DecreaseMethod::K2tog_Ssk;
    else return false;
    return true;
}

QJsonObject overlayToJson(const Overlay &o)
{
    return QJsonObject{{"pattern", o.pattern},         {"firstRow", o.firstRow},     {"rowCount", o.rowCount},
                       {"firstStitch", o.firstStitch}, {"stitchCount", o.stitchCount}, {"offsetX", o.offsetX},
                       {"offsetY", o.offsetY}};
}

bool overlayFromJson(const QJsonObject &j, Overlay &o, QString *error)
{
    o.pattern = j.value("pattern").toString();
    o.firstRow = j.value("firstRow").toInt();
    o.rowCount = j.value("rowCount").toInt();
    o.firstStitch = j.value("firstStitch").toInt();
    o.stitchCount = j.value("stitchCount").toInt();
    o.offsetX = j.value("offsetX").toInt();
    o.offsetY = j.value("offsetY").toInt();
    if (o.pattern.isEmpty() || o.firstRow < 0 || o.rowCount < 0 || o.firstStitch < 0 || o.stitchCount < 0)
    {
        return fail(error, QStringLiteral("a pattern placed on a band is not valid"));
    }
    return true;
}

bool samePattern(const PatternDef &a, const PatternDef &b)
{
    return a.name == b.name && a.kind == b.kind && a.width == b.width && a.height == b.height
           && a.legend == b.legend && a.cells == b.cells;
}

bool sameSize(const StitchSize &a, const StitchSize &b)
{
    return qFuzzyCompare(a.stitchWidthMm, b.stitchWidthMm) && qFuzzyCompare(a.rowHeightMm, b.rowHeightMm);
}
}

bool saveProject(const QString &path, const Project &p, QString *error)
{
    QJsonObject settings{{"stitchType", p.settings.stitchType},
                         {"stitchWidthMm", p.settings.size.stitchWidthMm},
                         {"rowHeightMm", p.settings.size.rowHeightMm},
                         {"includeSeamAllowance", p.settings.includeSeamAllowance},
                         {"edgeStitches", p.settings.shaping.edgeStitches},
                         {"bulkFrom", p.settings.shaping.bulkFrom},
                         {"shapeOn", p.settings.shaping.shapeOn == ShapingRows::EveryRow ? "every" : "outside"},
                         {"increase", increaseName(p.settings.shaping.increase)},
                         {"decrease", decreaseName(p.settings.shaping.decrease)}};

    QJsonObject pieces;
    for (auto it = p.bands.cbegin(); it != p.bands.cend(); ++it)
    {
        QJsonArray bands;
        for (const BandSpec &b : it.value())
        {
            QJsonArray overlays;
            for (const Overlay &o : b.overlays) overlays.append(overlayToJson(o));
            bands.append(QJsonObject{{"stitchType", b.stitchType}, {"heightMm", b.heightMm}, {"overlays", overlays}});
        }
        pieces.insert(it.key(), QJsonObject{{"bands", bands}});
    }

    QJsonObject root{{"format", formatId},
                     {"version", kProjectFileVersion},
                     {"pattern", toRelative(path, p.patternFile)},
                     {"settings", settings},
                     {"pieces", pieces},
                     {"catalog", p.catalog.toJson()}};
    if (!p.measurementsFile.isEmpty())
    {
        root["measurements"] = toRelative(path, p.measurementsFile);
    }

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
    {
        return fail(error, f.errorString());
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit() ? true : fail(error, f.errorString());
}

bool loadProject(const QString &path, Project &project, QString *error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        return fail(error, f.errorString());
    }
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject())
    {
        return fail(error, QStringLiteral("not a project file (%1)").arg(perr.errorString()));
    }
    const QJsonObject root = doc.object();
    if (root.value("format").toString() != QLatin1String(formatId))
    {
        return fail(error, QStringLiteral("not a Stitchly2D project file"));
    }
    const int version = root.value("version").toInt();
    if (version > kProjectFileVersion)
    {
        return fail(error, QStringLiteral("project version %1 is newer than this program understands (%2)")
                               .arg(version).arg(kProjectFileVersion));
    }
    if (version < 1)
    {
        return fail(error, QStringLiteral("missing or invalid version"));
    }

    Project out;
    out.patternFile = toAbsolute(path, root.value("pattern").toString());
    if (out.patternFile.isEmpty())
    {
        return fail(error, QStringLiteral("the project does not name a pattern file"));
    }
    out.measurementsFile = toAbsolute(path, root.value("measurements").toString());

    const QJsonObject s = root.value("settings").toObject();
    out.settings.stitchType = s.value("stitchType").toString();
    out.settings.size.stitchWidthMm = s.value("stitchWidthMm").toDouble();
    out.settings.size.rowHeightMm = s.value("rowHeightMm").toDouble();
    if (!out.settings.size.isValid())
    {
        return fail(error, QStringLiteral("the stitch width and row height must be greater than 0"));
    }
    out.settings.includeSeamAllowance = s.value("includeSeamAllowance").toBool();
    out.settings.shaping.edgeStitches = s.value("edgeStitches").toInt(1);
    if (out.settings.shaping.edgeStitches < 0)
    {
        return fail(error, QStringLiteral("the number of edge stitches must not be negative"));
    }
    // Files from before this setting existed were made with shaping on every row.
    const QString shapeOn = s.value("shapeOn").toString(QStringLiteral("every"));
    if (shapeOn != QLatin1String("every") && shapeOn != QLatin1String("outside"))
    {
        return fail(error, QStringLiteral("shapeOn must be \"outside\" or \"every\""));
    }
    out.settings.shaping.shapeOn = shapeOn == QLatin1String("outside") ? ShapingRows::OutsideRows : ShapingRows::EveryRow;
    out.settings.shaping.bulkFrom = s.value("bulkFrom").toInt(4);      // files from before this setting: the default
    if (out.settings.shaping.bulkFrom < 0)
    {
        return fail(error, QStringLiteral("the number of stitches from which edges are cast on or bound off must not be negative"));
    }
    if (!increaseFromName(s.value("increase").toString(), out.settings.shaping.increase)
        || !decreaseFromName(s.value("decrease").toString(), out.settings.shaping.decrease))
    {
        return fail(error, QStringLiteral("unknown increase or decrease method"));
    }

    const QJsonObject pieces = root.value("pieces").toObject();
    for (auto it = pieces.constBegin(); it != pieces.constEnd(); ++it)
    {
        QVector<BandSpec> bands;
        for (const QJsonValue &bv : it.value().toObject().value("bands").toArray())
        {
            const QJsonObject bj = bv.toObject();
            BandSpec b;
            b.stitchType = bj.value("stitchType").toString();
            b.heightMm = bj.value("heightMm").toDouble();
            if (b.stitchType.isEmpty() || b.heightMm < 0)
            {
                return fail(error, QStringLiteral("piece \"%1\": a band is not valid").arg(it.key()));
            }
            for (const QJsonValue &ov : bj.value("overlays").toArray())
            {
                Overlay o;
                QString msg;
                if (!overlayFromJson(ov.toObject(), o, &msg))
                {
                    return fail(error, QStringLiteral("piece \"%1\": %2").arg(it.key(), msg));
                }
                b.overlays.append(o);
            }
            bands.append(b);
        }
        if (bands.isEmpty())
        {
            return fail(error, QStringLiteral("piece \"%1\" has no bands").arg(it.key()));
        }
        out.bands.insert(it.key(), bands);
    }

    if (!StitchCatalog::fromJson(root.value("catalog").toObject(), out.catalog))
    {
        return fail(error, QStringLiteral("the catalog inside the project is not valid"));
    }
    project = out;
    return true;
}

StitchCatalog usedCatalog(const StitchCatalog &full, const QMap<QString, QVector<BandSpec>> &bands)
{
    StitchCatalog used;
    for (const QVector<BandSpec> &list : bands)
    {
        for (const BandSpec &b : list)
        {
            if (b.stitchType != QLatin1String(kDefaultStitchName) && full.basics().contains(b.stitchType))
            {
                used.basics().set(full.basics().find(b.stitchType));
            }
            for (const Overlay &o : b.overlays)
            {
                if (full.contains(o.pattern))
                {
                    used.set(full.find(o.pattern));
                }
            }
        }
    }
    return used;
}

bool operator==(const Overlay &a, const Overlay &b)
{
    return a.pattern == b.pattern && a.firstRow == b.firstRow && a.rowCount == b.rowCount
           && a.firstStitch == b.firstStitch && a.stitchCount == b.stitchCount && a.offsetX == b.offsetX
           && a.offsetY == b.offsetY;
}

bool operator==(const BandSpec &a, const BandSpec &b)
{
    return a.stitchType == b.stitchType && qFuzzyCompare(a.heightMm + 1.0, b.heightMm + 1.0) && a.overlays == b.overlays;
}

bool operator==(const ProjectSettings &a, const ProjectSettings &b)
{
    return a.stitchType == b.stitchType && sameSize(a.size, b.size)
           && a.includeSeamAllowance == b.includeSeamAllowance && a.shaping.edgeStitches == b.shaping.edgeStitches
           && a.shaping.increase == b.shaping.increase && a.shaping.decrease == b.shaping.decrease
           && a.shaping.bulkFrom == b.shaping.bulkFrom && a.shaping.shapeOn == b.shaping.shapeOn;
}

bool sameCatalog(const StitchCatalog &a, const StitchCatalog &b)
{
    if (a.basics().types().size() != b.basics().types().size() || a.patterns().size() != b.patterns().size())
    {
        return false;
    }
    const CatalogDiff d = diffCatalog(a, b);     // names in b that a lacks or defines differently
    return d.added.isEmpty() && d.differing.isEmpty();
}

CatalogDiff diffCatalog(const StitchCatalog &target, const StitchCatalog &incoming)
{
    CatalogDiff d;
    for (const StitchType &t : incoming.basics().types())
    {
        if (!target.basics().contains(t.name))
            d.added << t.name;
        else if (!sameSize(target.basics().find(t.name).size, t.size))
            d.differing << t.name;
    }
    for (const PatternDef &p : incoming.patterns())
    {
        if (!target.contains(p.name))
            d.added << p.name;
        else if (!samePattern(target.find(p.name), p))
            d.differing << p.name;
    }
    return d;
}

void mergeCatalog(StitchCatalog &target, const StitchCatalog &incoming, bool replaceDifferent)
{
    for (const StitchType &t : incoming.basics().types())
    {
        if (!target.basics().contains(t.name) || (replaceDifferent && !sameSize(target.basics().find(t.name).size, t.size)))
        {
            target.basics().set(t);
        }
    }
    for (const PatternDef &p : incoming.patterns())
    {
        if (!target.contains(p.name) || (replaceDifferent && !samePattern(target.find(p.name), p)))
        {
            target.set(p);
        }
    }
}

}
