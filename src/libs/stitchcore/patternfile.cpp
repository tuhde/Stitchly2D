#include "patternfile.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

namespace stitchcore
{

const char *const kPatternFileExtension = "stpat";

namespace
{
const char *const formatId = "stitchly-patterns";

bool fail(QString *error, const QString &msg)
{
    if (error) *error = msg;
    return false;
}
}

bool exportPatterns(const QString &path, const QVector<PatternDef> &patterns, const PatternFileInfo &info,
                    QString *error)
{
    if (patterns.isEmpty())
    {
        return fail(error, QCoreApplication::translate("StitchCore", "nothing to export"));
    }
    QSet<QString> names;
    QJsonArray arr;
    for (const PatternDef &p : patterns)
    {
        if (!p.isValid())
        {
            return fail(error, QCoreApplication::translate("StitchCore", "pattern '%1' is not valid").arg(p.name));
        }
        if (names.contains(p.name))
        {
            return fail(error, QCoreApplication::translate("StitchCore", "two patterns are named '%1'").arg(p.name));
        }
        names.insert(p.name);
        arr.append(p.toJson());
    }
    QJsonObject root{{"format", formatId}, {"version", kPatternFileVersion}};
    if (!info.author.isEmpty()) root["author"] = info.author;
    if (!info.description.isEmpty()) root["description"] = info.description;
    root["patterns"] = arr;

    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
    {
        return fail(error, f.errorString());
    }
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return f.commit() ? true : fail(error, f.errorString());
}

bool importPatterns(const QString &path, QVector<PatternDef> &patterns, PatternFileInfo *info, QString *error)
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
        return fail(error, QCoreApplication::translate("StitchCore", "not a pattern file (%1)").arg(perr.errorString()));
    }
    const QJsonObject root = doc.object();
    if (root.value("format").toString() != QLatin1String(formatId))
    {
        return fail(error, QCoreApplication::translate("StitchCore", "not a Stitchly2D pattern file"));
    }
    const int version = root.value("version").toInt();
    if (version > kPatternFileVersion)
    {
        return fail(error, QCoreApplication::translate("StitchCore", "file version %1 is newer than this program understands (%2)")
                               .arg(version).arg(kPatternFileVersion));
    }
    if (version < 1)
    {
        return fail(error, QCoreApplication::translate("StitchCore", "missing or invalid version"));
    }
    const QJsonArray arr = root.value("patterns").toArray();
    if (arr.isEmpty())
    {
        return fail(error, QCoreApplication::translate("StitchCore", "the file contains no patterns"));
    }
    QVector<PatternDef> out;
    QSet<QString> names;
    for (const QJsonValue &v : arr)
    {
        PatternDef p;
        QString msg;
        if (!PatternDef::fromJson(v.toObject(), p, &msg))
        {
            return fail(error, msg);
        }
        if (names.contains(p.name))
        {
            return fail(error, QCoreApplication::translate("StitchCore", "two patterns are named '%1'").arg(p.name));
        }
        names.insert(p.name);
        out.append(p);
    }
    patterns = out;
    if (info)
    {
        info->author = root.value("author").toString();
        info->description = root.value("description").toString();
    }
    return true;
}

ImportResult addToCatalog(StitchCatalog &catalog, const QVector<PatternDef> &patterns, ConflictPolicy policy)
{
    ImportResult res;
    for (PatternDef p : patterns)
    {
        if (!p.isValid())
        {
            ++res.skipped;
            continue;
        }
        if (catalog.contains(p.name))
        {
            if (policy == ConflictPolicy::Skip)
            {
                ++res.skipped;
                continue;
            }
            if (policy == ConflictPolicy::Replace)
            {
                catalog.set(p);
                ++res.replaced;
                continue;
            }
            const QString base = p.name;
            int n = 2;
            do
            {
                p.name = QStringLiteral("%1 (%2)").arg(base).arg(n++);
            } while (catalog.contains(p.name));
            ++res.renamed;
        }
        catalog.set(p);
        ++res.added;
    }
    return res;
}

}
