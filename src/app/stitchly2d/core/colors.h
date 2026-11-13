// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
#ifndef COLORS_H
#define COLORS_H

#include <QColor>
#include <QString>

/**
 * Color to show for a yarn name from a color pattern. Names Qt knows ("red", "#c03030") are used as written;
 * other names ("A", "MC", "CC1") get a stable color from a fixed palette, so the same name always looks the same.
 */
QColor yarnColor(const QString &name);

#endif
