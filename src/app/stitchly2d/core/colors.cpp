#include "colors.h"

#include <QVector>

QColor yarnColor(const QString &name)
{
    const QColor named(name);
    if (named.isValid())
    {
        return named;
    }
    static const QVector<QColor> palette = {QColor("#4e79a7"), QColor("#f28e2b"), QColor("#e15759"), QColor("#76b7b2"),
                                            QColor("#59a14f"), QColor("#edc948"), QColor("#b07aa1"), QColor("#ff9da7")};
    if (name.size() == 1 && name.at(0).isLetter())
    {
        return palette.at((name.at(0).toUpper().unicode() - 'A') % palette.size());
    }
    // Not qHash: it is seeded per run, and the same name must give the same color every time.
    uint h = 5381;
    for (const QChar &c : name)
    {
        h = h * 33 + c.unicode();
    }
    return palette.at(h % palette.size());
}
