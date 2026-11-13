#include "catalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace stitchcore
{

bool PatternDef::isValid() const
{
    if (name.isEmpty() || width < 1 || height < 1 || cells.size() != width * height)
    {
        return false;
    }
    for (int c : cells)
    {
        if (c < -1 || c >= legend.size())
        {
            return false;
        }
    }
    return true;
}

int PatternDef::cellAt(int x, int y) const
{
    if (width < 1 || height < 1 || cells.size() != width * height)
    {
        return -1;
    }
    const int cx = ((x % width) + width) % width;
    const int cy = ((y % height) + height) % height;
    return cells.at(cy * width + cx);
}

StitchCatalog StitchCatalog::withDefaults()
{
    StitchCatalog c;
    c.m_basics.set({QStringLiteral("Stockinette"), StitchSize{5.0, 3.5}});
    c.m_basics.set({QStringLiteral("Garter"),      StitchSize{5.0, 2.5}});
    c.m_basics.set({QStringLiteral("1:1 rib"),     StitchSize{3.5, 3.5}});
    c.m_basics.set({QStringLiteral("2:2 rib"),     StitchSize{3.5, 3.5}});
    c.m_basics.set({QStringLiteral("Patent"),      StitchSize{4.0, 4.5}});

    PatternDef seed;
    seed.name = QStringLiteral("Seed stitch");
    seed.kind = PatternDef::Special;
    seed.width = 2;
    seed.height = 2;
    seed.legend = {QStringLiteral("k"), QStringLiteral("p")};
    seed.cells = {0, 1,
                  1, 0};
    c.set(seed);

    PatternDef stripes;
    stripes.name = QStringLiteral("Stripes 2 rows");
    stripes.kind = PatternDef::Color;
    stripes.width = 1;
    stripes.height = 4;
    stripes.legend = {QStringLiteral("A"), QStringLiteral("B")};
    stripes.cells = {0, 0, 1, 1};
    c.set(stripes);

    PatternDef checks;
    checks.name = QStringLiteral("Checks 2x2");
    checks.kind = PatternDef::Color;
    checks.width = 4;
    checks.height = 4;
    checks.legend = {QStringLiteral("A"), QStringLiteral("B")};
    checks.cells = {0, 0, 1, 1,
                    0, 0, 1, 1,
                    1, 1, 0, 0,
                    1, 1, 0, 0};
    c.set(checks);
    return c;
}

QVector<PatternDef> StitchCatalog::patterns(PatternDef::Kind kind) const
{
    QVector<PatternDef> out;
    for (const PatternDef &p : m_patterns)
    {
        if (p.kind == kind) out.append(p);
    }
    return out;
}

bool StitchCatalog::contains(const QString &name) const
{
    return !find(name).name.isEmpty();
}

PatternDef StitchCatalog::find(const QString &name) const
{
    for (const PatternDef &p : m_patterns)
    {
        if (p.name == name) return p;
    }
    return PatternDef();
}

bool StitchCatalog::set(const PatternDef &pattern)
{
    if (!pattern.isValid())
    {
        return false;
    }
    for (PatternDef &p : m_patterns)
    {
        if (p.name == pattern.name)
        {
            p = pattern;
            return true;
        }
    }
    m_patterns.append(pattern);
    return true;
}

bool StitchCatalog::remove(const QString &name)
{
    for (int i = 0; i < m_patterns.size(); ++i)
    {
        if (m_patterns.at(i).name == name)
        {
            m_patterns.removeAt(i);
            return true;
        }
    }
    return false;
}

QJsonObject StitchCatalog::toJson() const
{
    QJsonArray basics;
    for (const StitchType &t : m_basics.types())
    {
        basics.append(QJsonObject{{"name", t.name},
                                  {"stitchWidthMm", t.size.stitchWidthMm},
                                  {"rowHeightMm", t.size.rowHeightMm}});
    }
    QJsonArray patterns;
    for (const PatternDef &p : m_patterns)
    {
        QJsonArray cells;
        for (int c : p.cells) cells.append(c);
        patterns.append(QJsonObject{{"name", p.name},
                                    {"kind", p.kind == PatternDef::Color ? "color" : "special"},
                                    {"width", p.width},
                                    {"height", p.height},
                                    {"legend", QJsonArray::fromStringList(p.legend)},
                                    {"cells", cells}});
    }
    return QJsonObject{{"version", 1}, {"basics", basics}, {"patterns", patterns}};
}

bool StitchCatalog::fromJson(const QJsonObject &json, StitchCatalog &out)
{
    if (json.value("version").toInt() != 1)
    {
        return false;
    }
    StitchCatalog c;
    for (const QJsonValue &v : json.value("basics").toArray())
    {
        const QJsonObject o = v.toObject();
        StitchType t;
        t.name = o.value("name").toString();
        t.size.stitchWidthMm = o.value("stitchWidthMm").toDouble();
        t.size.rowHeightMm = o.value("rowHeightMm").toDouble();
        if (t.name.isEmpty() || !t.size.isValid())
        {
            return false;
        }
        c.m_basics.set(t);
    }
    for (const QJsonValue &v : json.value("patterns").toArray())
    {
        const QJsonObject o = v.toObject();
        PatternDef p;
        p.name = o.value("name").toString();
        const QString kind = o.value("kind").toString();
        if (kind != QLatin1String("color") && kind != QLatin1String("special"))
        {
            return false;
        }
        p.kind = kind == QLatin1String("color") ? PatternDef::Color : PatternDef::Special;
        p.width = o.value("width").toInt();
        p.height = o.value("height").toInt();
        for (const QJsonValue &l : o.value("legend").toArray()) p.legend << l.toString();
        for (const QJsonValue &cv : o.value("cells").toArray()) p.cells.append(cv.toInt(-2));
        if (!c.set(p))
        {
            return false;
        }
    }
    out = c;
    return true;
}

bool StitchCatalog::save(const QString &path) const
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
    {
        return false;
    }
    f.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    return f.commit();
}

bool StitchCatalog::load(const QString &path, StitchCatalog &out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        return false;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
    {
        return false;
    }
    return fromJson(doc.object(), out);
}

}
