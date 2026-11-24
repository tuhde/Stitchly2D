// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef STITCHCORE_YARN_H
#define STITCHCORE_YARN_H

#include <QJsonObject>
#include <QString>
#include <QVector>

#include "stitchsize.h"

namespace stitchcore
{

/** What a yarn knits to with one basic stitch (swatch result), plus optionally how much yarn one stitch takes. */
struct YarnGauge
{
    QString    stitchType;            //!< name of a basic stitch of the catalog
    StitchSize size;                  //!< stitch width and row height of the swatch
    qreal      yarnPerStitchMm = 0;   //!< measured yarn length per stitch; 0 = use the default estimate
};

/** One entry of the yarn inventory. */
struct Yarn
{
    QString id;                       //!< UUID, never shown; projects refer to yarns by it
    QString brand;
    QString name;
    QString fiber;                    //!< e.g. "merino 80%, nylon 20%"
    QString weightClass;              //!< e.g. "DK", "fingering"
    QString colorName;
    QString colorHex;                 //!< "#rrggbb", empty = unknown
    QString imagePath;
    QString dyeLot;
    QString notes;
    qreal   metersPerSkein = 0;
    qreal   gramsPerSkein = 0;
    int     skeinsInStock = 0;
    qreal   partialMeters = 0;        //!< meters left on an opened skein, on top of skeinsInStock
    QVector<YarnGauge> gauges;

    /** All the yarn that is left, in meters. */
    qreal   stockMeters() const { return skeinsInStock * metersPerSkein + partialMeters; }
    /** Name for lists: "Brand Name (color)". */
    QString displayName() const;
    /** The gauge for a basic stitch; nullptr if none was entered. */
    const YarnGauge *gaugeFor(const QString &stitchType) const;

    QJsonObject toJson() const;
    static bool fromJson(const QJsonObject &json, Yarn &out);
    static QString newId();
};

bool operator==(const YarnGauge &a, const YarnGauge &b);
bool operator==(const Yarn &a, const Yarn &b);

/** Takes meters from the stock: whole skeins first count as opened. Returns false (and changes nothing) if not enough is left. */
bool consumeYarn(Yarn &yarn, qreal meters);

/** The user's yarns, saved as JSON ("yarns.json") next to the catalog. */
class YarnInventory
{
public:
    static constexpr int kFileVersion = 1;

    const QVector<Yarn> &yarns() const { return m_yarns; }
    int  size() const { return m_yarns.size(); }
    const Yarn *find(const QString &id) const;
    Yarn *find(const QString &id);
    /** Add; a yarn without id gets one. Returns the id. An existing id is replaced. */
    QString set(Yarn yarn);
    bool remove(const QString &id);

    QJsonObject toJson() const;
    /** Error says why a file was refused; nothing is changed then. */
    static bool fromJson(const QJsonObject &json, YarnInventory &out, QString *error = nullptr);
    bool save(const QString &path, QString *error = nullptr) const;
    /** A missing file is not an error: out is then empty. */
    static bool load(const QString &path, YarnInventory &out, QString *error = nullptr);

    friend bool operator==(const YarnInventory &a, const YarnInventory &b) { return a.m_yarns == b.m_yarns; }

private:
    QVector<Yarn> m_yarns;
};

/** Where the inventory lives: AppConfigLocation + "/yarns.json". */
QString userYarnPath();

}

#endif
