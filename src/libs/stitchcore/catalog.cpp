#include "catalog.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

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

QJsonObject PatternDef::toJson() const
{
    QJsonArray rows;
    for (int y = height - 1; y >= 0; --y)      // top row first
    {
        QJsonArray row;
        for (int x = 0; x < width; ++x)
        {
            row.append(cells.value(y * width + x, -1));
        }
        rows.append(row);
    }
    return QJsonObject{{"name", name},
                       {"kind", kind == Color ? "color" : "special"},
                       {"width", width},
                       {"height", height},
                       {"legend", QJsonArray::fromStringList(legend)},
                       {"rows", rows}};
}

bool PatternDef::fromJson(const QJsonObject &o, PatternDef &out, QString *error)
{
    auto fail = [error](const QString &msg) {
        if (error) *error = msg;
        return false;
    };
    PatternDef p;
    p.name = o.value("name").toString();
    if (p.name.isEmpty())
    {
        return fail(QCoreApplication::translate("StitchCore", "pattern without a name"));
    }
    const QString kind = o.value("kind").toString();
    if (kind != QLatin1String("color") && kind != QLatin1String("special"))
    {
        return fail(QCoreApplication::translate("StitchCore", "%1: kind must be \"special\" or \"color\"").arg(p.name));
    }
    p.kind = kind == QLatin1String("color") ? Color : Special;
    p.width = o.value("width").toInt();
    p.height = o.value("height").toInt();
    for (const QJsonValue &l : o.value("legend").toArray())
    {
        if (!l.isString())
        {
            return fail(QCoreApplication::translate("StitchCore", "%1: legend entries must be strings").arg(p.name));
        }
        p.legend << l.toString();
    }
    const QJsonArray rows = o.value("rows").toArray();
    if (p.width < 1 || p.height < 1 || rows.size() != p.height)
    {
        return fail(QCoreApplication::translate("StitchCore", "%1: size does not match the rows").arg(p.name));
    }
    p.cells.fill(-1, p.width * p.height);
    for (int i = 0; i < rows.size(); ++i)
    {
        const QJsonArray row = rows.at(i).toArray();
        if (row.size() != p.width)
        {
            return fail(QCoreApplication::translate("StitchCore", "%1: row %2 has %3 cells, expected %4")
                            .arg(p.name).arg(i + 1).arg(row.size()).arg(p.width));
        }
        const int y = p.height - 1 - i;        // first row in the file is the top row
        for (int x = 0; x < p.width; ++x)
        {
            p.cells[y * p.width + x] = row.at(x).toInt(-2);
        }
    }
    if (!p.isValid())
    {
        return fail(QCoreApplication::translate("StitchCore", "%1: a cell refers to a legend entry that does not exist").arg(p.name));
    }
    out = p;
    return true;
}

QVector<QStringList> PatternDef::toChart() const
{
    QVector<QStringList> rows;
    for (int y = height - 1; y >= 0; --y)      // top row first
    {
        QStringList row;
        for (int x = 0; x < width; ++x)
        {
            const int idx = cells.value(y * width + x, -1);
            row << (idx >= 0 && idx < legend.size() ? legend.at(idx) : QString());
        }
        rows.append(row);
    }
    return rows;
}

bool PatternDef::fromChart(const QString &name, Kind kind, const QVector<QStringList> &rowsTopFirst, PatternDef &out,
                           QString *error)
{
    auto fail = [error](const QString &msg) {
        if (error) *error = msg;
        return false;
    };
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty())
    {
        return fail(QCoreApplication::translate("StitchCore", "the pattern has no name"));
    }
    if (rowsTopFirst.isEmpty() || rowsTopFirst.first().isEmpty())
    {
        return fail(QCoreApplication::translate("StitchCore", "%1: the chart has no cells").arg(trimmedName));
    }
    PatternDef p;
    p.name = trimmedName;
    p.kind = kind;
    p.height = rowsTopFirst.size();
    p.width = rowsTopFirst.first().size();
    p.cells.fill(-1, p.width * p.height);
    for (int i = 0; i < p.height; ++i)
    {
        if (rowsTopFirst.at(i).size() != p.width)
        {
            return fail(QCoreApplication::translate("StitchCore", "%1: row %2 has %3 cells, expected %4")
                            .arg(trimmedName).arg(i + 1).arg(rowsTopFirst.at(i).size()).arg(p.width));
        }
        const int y = p.height - 1 - i;
        for (int x = 0; x < p.width; ++x)
        {
            const QString text = rowsTopFirst.at(i).at(x).trimmed();
            if (text.isEmpty())
            {
                continue;
            }
            int idx = p.legend.indexOf(text);
            if (idx < 0)
            {
                p.legend << text;
                idx = p.legend.size() - 1;
            }
            p.cells[y * p.width + x] = idx;
        }
    }
    if (p.legend.isEmpty())
    {
        return fail(QCoreApplication::translate("StitchCore", "%1: the chart has no stitches or colors, fill in at least one cell").arg(trimmedName));
    }
    out = p;
    return true;
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
        patterns.append(p.toJson());
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
        PatternDef p;
        if (!PatternDef::fromJson(v.toObject(), p) || !c.set(p))
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

QString catalogFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/catalog.json");
}

}
