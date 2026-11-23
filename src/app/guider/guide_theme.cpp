#include "guide_theme.h"

#include <QApplication>
#include <QGuiApplication>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>
#include <QWidget>

#include "guide_scale.h"

namespace
{
Qt::ColorScheme g_testScheme = Qt::ColorScheme::Unknown;   // set by tests only
bool            g_testSchemeSet = false;

bool systemIsDark()
{
    const Qt::ColorScheme scheme = g_testSchemeSet ? g_testScheme : QGuiApplication::styleHints()->colorScheme();
    // Without information from the system (no desktop portal, e.g. a bare X11 session) the dark colors are used:
    // they are easier on the eyes at a knitting machine and were always the look of guider.
    return scheme != Qt::ColorScheme::Light;
}
}

QString themeChoiceName(ThemeChoice choice)
{
    switch (choice)
    {
        case ThemeChoice::Light: return QStringLiteral("light");
        case ThemeChoice::Dark:  return QStringLiteral("dark");
        case ThemeChoice::System: break;
    }
    return QStringLiteral("system");
}

ThemeChoice themeChoiceFromName(const QString &name)
{
    if (name == QLatin1String("light")) return ThemeChoice::Light;
    if (name == QLatin1String("dark"))  return ThemeChoice::Dark;
    return ThemeChoice::System;
}

ThemeChoice guideThemeChoice()
{
    return themeChoiceFromName(QSettings(QStringLiteral("Stitchly2D"), QStringLiteral("guider")).value(QStringLiteral("theme")).toString());
}

bool guideThemeIsDark()
{
    switch (guideThemeChoice())
    {
        case ThemeChoice::Light: return false;
        case ThemeChoice::Dark:  return true;
        case ThemeChoice::System: break;
    }
    return systemIsDark();
}

QColor blendColors(const QColor &a, const QColor &b, qreal t)
{
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t);
}

GuideTheme darkGuideTheme()
{
    GuideTheme t;
    t.window = QColor(0x20, 0x20, 0x20);
    t.view = QColor(0x10, 0x10, 0x10);
    t.text = QColor(0xf0, 0xf0, 0xf0);
    t.textDisabled = QColor(0x80, 0x80, 0x80);
    t.button = QColor(0x44, 0x44, 0x44);           // clearly lighter than the window
    t.buttonPressed = QColor(0x4a, 0x6f, 0xa5);
    t.border = QColor(0x9a, 0x9a, 0x9a);
    t.accent = QColor(0x4a, 0x6f, 0xa5);
    t.accentText = QColor(0xff, 0xff, 0xff);
    t.stepBackground = QColor(0x1d, 0x2c, 0x44);
    t.ok = QColor(0x7f, 0xbf, 0x7f);
    t.warning = QColor(0xff, 0xb3, 0x6b);
    t.error = QColor(0xff, 0x6b, 0x6b);
    return t;
}

GuideTheme lightGuideTheme()
{
    GuideTheme t;
    t.window = QColor(0xee, 0xee, 0xee);
    t.view = QColor(0xff, 0xff, 0xff);
    t.text = QColor(0x1a, 0x1a, 0x1a);
    t.textDisabled = QColor(0x90, 0x90, 0x90);
    t.button = QColor(0xd4, 0xd4, 0xd4);
    t.buttonPressed = QColor(0x9b, 0xb8, 0xe0);
    t.border = QColor(0x70, 0x70, 0x70);
    t.accent = QColor(0x2f, 0x6d, 0xb5);
    t.accentText = QColor(0xff, 0xff, 0xff);
    t.stepBackground = QColor(0xd6, 0xe6, 0xfb);
    t.ok = QColor(0x1e, 0x7a, 0x2e);
    t.warning = QColor(0xa8, 0x5a, 0x00);
    t.error = QColor(0xc0, 0x1f, 0x1f);
    return t;
}

GuideTheme guideTheme()
{
    return guideThemeIsDark() ? darkGuideTheme() : lightGuideTheme();
}

GuideThemeNotifier *GuideThemeNotifier::instance()
{
    static GuideThemeNotifier notifier;
    return &notifier;
}

void setGuideThemeChoice(ThemeChoice choice)
{
    QSettings(QStringLiteral("Stitchly2D"), QStringLiteral("guider")).setValue(QStringLiteral("theme"), themeChoiceName(choice));
    if (auto *app = qobject_cast<QApplication *>(QCoreApplication::instance()))
    {
        applyGuideStyle(*app);
    }
}

void setStatusStyle(QWidget *label, const QString &status)
{
    label->setProperty("status", status);
    label->style()->unpolish(label);
    label->style()->polish(label);
}

namespace
{
void systemSchemeChanged()
{
    if (guideThemeChoice() == ThemeChoice::System)
    {
        if (auto *app = qobject_cast<QApplication *>(QCoreApplication::instance()))
        {
            applyGuideStyle(*app);
        }
    }
}
}

void startFollowingSystemTheme()
{
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, qApp, []() { systemSchemeChanged(); });
}

void pretendSystemColorScheme(Qt::ColorScheme scheme, bool pretend)
{
    g_testScheme = scheme;
    g_testSchemeSet = pretend;
    systemSchemeChanged();      // as when the system really switches
}
