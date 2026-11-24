#include "yarn.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>

namespace stitchcore
{

namespace
{
const char *const formatId = "stitchly-yarns";

bool fail(QString *error, const QString &msg)
{
    if (error) *error = msg;
    return false;
}

bool validColor(const QString &c)
{
    static const QRegularExpression re(QStringLiteral("^#[0-9a-fA-F]{6}$"));
    return c.isEmpty() || re.match(c).hasMatch();
}
}

QString Yarn::newId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString Yarn::displayName() const
{
    QString text = (brand + QLatin1Char(' ') + name).trimmed();
    if (!colorName.isEmpty())
    {
        text += QStringLiteral(" (%1)").arg(colorName);
    }
    return text;
}

const YarnGauge *Yarn::gaugeFor(const QString &stitchType) const
{
    for (const YarnGauge &g : gauges)
    {
        if (g.stitchType == stitchType) return &g;
    }
    return nullptr;
}

QJsonObject Yarn::toJson() const
{
    QJsonArray gaugeArray;
    for (const YarnGauge &g : gauges)
    {
        gaugeArray.append(QJsonObject{{"stitchType", g.stitchType},
                                      {"stitchWidthMm", g.size.stitchWidthMm},
                                      {"rowHeightMm", g.size.rowHeightMm},
                                      {"yarnPerStitchMm", g.yarnPerStitchMm}});
    }
    return QJsonObject{{"id", id},           {"brand", brand},       {"name", name},
                       {"fiber", fiber},     {"weightClass", weightClass},
                       {"colorName", colorName}, {"colorHex", colorHex}, {"imagePath", imagePath},
                       {"dyeLot", dyeLot},   {"notes", notes},
                       {"metersPerSkein", metersPerSkein}, {"gramsPerSkein", gramsPerSkein},
                       {"skeinsInStock", skeinsInStock},   {"partialMeters", partialMeters},
                       {"gauges", gaugeArray}};
}

bool Yarn::fromJson(const QJsonObject &o, Yarn &out)
{
    Yarn y;
    y.id = o.value("id").toString();
    y.brand = o.value("brand").toString();
    y.name = o.value("name").toString();
    y.fiber = o.value("fiber").toString();
    y.weightClass = o.value("weightClass").toString();
    y.colorName = o.value("colorName").toString();
    y.colorHex = o.value("colorHex").toString();
    y.imagePath = o.value("imagePath").toString();
    y.dyeLot = o.value("dyeLot").toString();
    y.notes = o.value("notes").toString();
    y.metersPerSkein = o.value("metersPerSkein").toDouble();
    y.gramsPerSkein = o.value("gramsPerSkein").toDouble();
    y.skeinsInStock = o.value("skeinsInStock").toInt();
    y.partialMeters = o.value("partialMeters").toDouble();
    if (y.id.isEmpty() || (y.brand.isEmpty() && y.name.isEmpty()) || !validColor(y.colorHex)
        || y.metersPerSkein < 0 || y.gramsPerSkein < 0 || y.skeinsInStock < 0 || y.partialMeters < 0)
    {
        return false;
    }
    for (const QJsonValue &v : o.value("gauges").toArray())
    {
        const QJsonObject g = v.toObject();
        YarnGauge yg;
        yg.stitchType = g.value("stitchType").toString();
        yg.size.stitchWidthMm = g.value("stitchWidthMm").toDouble();
        yg.size.rowHeightMm = g.value("rowHeightMm").toDouble();
        yg.yarnPerStitchMm = g.value("yarnPerStitchMm").toDouble();
        if (yg.stitchType.isEmpty() || !yg.size.isValid() || yg.yarnPerStitchMm < 0 || y.gaugeFor(yg.stitchType))
        {
            return false;
        }
        y.gauges.append(yg);
    }
    out = y;
    return true;
}

bool operator==(const YarnGauge &a, const YarnGauge &b)
{
    return a.stitchType == b.stitchType && qFuzzyCompare(a.size.stitchWidthMm, b.size.stitchWidthMm)
           && qFuzzyCompare(a.size.rowHeightMm, b.size.rowHeightMm)
           && qFuzzyCompare(a.yarnPerStitchMm + 1, b.yarnPerStitchMm + 1);
}

bool operator==(const Yarn &a, const Yarn &b)
{
    return a.id == b.id && a.brand == b.brand && a.name == b.name && a.fiber == b.fiber
           && a.weightClass == b.weightClass && a.colorName == b.colorName && a.colorHex == b.colorHex
           && a.imagePath == b.imagePath && a.dyeLot == b.dyeLot && a.notes == b.notes
           && qFuzzyCompare(a.metersPerSkein + 1, b.metersPerSkein + 1)
           && qFuzzyCompare(a.gramsPerSkein + 1, b.gramsPerSkein + 1)
           && a.skeinsInStock == b.skeinsInStock && qFuzzyCompare(a.partialMeters + 1, b.partialMeters + 1)
           && a.gauges == b.gauges;
}

bool consumeYarn(Yarn &yarn, qreal meters)
{
    if (meters < 0 || meters > yarn.stockMeters() + 1e-9)
    {
        return false;
    }
    qreal left = meters;
    const qreal fromPartial = qMin(left, yarn.partialMeters);
    yarn.partialMeters -= fromPartial;
    left -= fromPartial;
    while (left > 1e-9 && yarn.skeinsInStock > 0)
    {
        --yarn.skeinsInStock;                       // open a skein
        yarn.partialMeters = yarn.metersPerSkein;
        const qreal take = qMin(left, yarn.partialMeters);
        yarn.partialMeters -= take;
        left -= take;
    }
    if (yarn.partialMeters < 1e-9) yarn.partialMeters = 0;
    return true;
}

const Yarn *YarnInventory::find(const QString &id) const
{
    for (const Yarn &y : m_yarns)
    {
        if (y.id == id) return &y;
    }
    return nullptr;
}

Yarn *YarnInventory::find(const QString &id)
{
    for (Yarn &y : m_yarns)
    {
        if (y.id == id) return &y;
    }
    return nullptr;
}

QString YarnInventory::set(Yarn yarn)
{
    if (yarn.id.isEmpty()) yarn.id = Yarn::newId();
    if (Yarn *existing = find(yarn.id))
    {
        *existing = yarn;
    }
    else
    {
        m_yarns.append(yarn);
    }
    return yarn.id;
}

bool YarnInventory::remove(const QString &id)
{
    for (int i = 0; i < m_yarns.size(); ++i)
    {
        if (m_yarns.at(i).id == id)
        {
            m_yarns.removeAt(i);
            return true;
        }
    }
    return false;
}

QJsonObject YarnInventory::toJson() const
{
    QJsonArray list;
    for (const Yarn &y : m_yarns) list.append(y.toJson());
    return QJsonObject{{"format", formatId}, {"version", kFileVersion}, {"yarns", list}};
}

bool YarnInventory::fromJson(const QJsonObject &json, YarnInventory &out, QString *error)
{
    if (json.value("format").toString() != QLatin1String(formatId))
    {
        return fail(error, QCoreApplication::translate("Yarn", "not a Stitchly2D yarn file"));
    }
    const int version = json.value("version").toInt();
    if (version > kFileVersion)
    {
        return fail(error, QCoreApplication::translate("Yarn", "yarn file version %1 is newer than this program understands (%2)")
                               .arg(version).arg(kFileVersion));
    }
    if (version < 1)
    {
        return fail(error, QCoreApplication::translate("Yarn", "missing or invalid version"));
    }
    YarnInventory inv;
    int n = 0;
    for (const QJsonValue &v : json.value("yarns").toArray())
    {
        ++n;
        Yarn y;
        if (!Yarn::fromJson(v.toObject(), y) || inv.find(y.id))
        {
            return fail(error, QCoreApplication::translate("Yarn", "yarn number %1 is not valid").arg(n));
        }
        inv.m_yarns.append(y);
    }
    out = inv;
    return true;
}

bool YarnInventory::save(const QString &path, QString *error) const
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
    {
        return fail(error, f.errorString());
    }
    f.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    return f.commit() ? true : fail(error, f.errorString());
}

bool YarnInventory::load(const QString &path, YarnInventory &out, QString *error)
{
    QFile f(path);
    if (!f.exists())
    {
        out = YarnInventory();
        return true;
    }
    if (!f.open(QIODevice::ReadOnly))
    {
        return fail(error, f.errorString());
    }
    QJsonParseError perr;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject())
    {
        return fail(error, QCoreApplication::translate("Yarn", "not a yarn file (%1)").arg(perr.errorString()));
    }
    return fromJson(doc.object(), out, error);
}

QString userYarnPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/yarns.json");
}

}
