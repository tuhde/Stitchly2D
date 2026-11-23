// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef GUIDE_THEME_H
#define GUIDE_THEME_H

#include <QColor>
#include <QObject>
#include <QString>
#include <Qt>

class QWidget;

/** All colors of guider. Dark and light are two sets of these; nothing else in the program names a color. */
struct GuideTheme
{
    QColor window;          //!< dialogs and the window around the text
    QColor view;            //!< background of lists and of the rolling text
    QColor text;
    QColor textDisabled;
    QColor button;          //!< buttons: clearly different from the window
    QColor buttonPressed;
    QColor border;
    QColor accent;          //!< chosen choice, selected list entry
    QColor accentText;
    QColor stepBackground;  //!< highlight behind the current steps
    QColor ok, warning, error;
};

/** Which colors to use. System follows the system's dark/light setting while the program runs. */
enum class ThemeChoice { System, Light, Dark };

QString     themeChoiceName(ThemeChoice choice);                  //!< "system", "light", "dark"
ThemeChoice themeChoiceFromName(const QString &name);             //!< unknown names give System

ThemeChoice guideThemeChoice();                                   //!< saved in the settings
/** Saves the choice and restyles everything. */
void        setGuideThemeChoice(ThemeChoice choice);
/** Whether the colors in use are the dark ones (the choice, or the system's setting). */
bool        guideThemeIsDark();
/** The colors in use. */
GuideTheme  guideTheme();
GuideTheme  darkGuideTheme();
GuideTheme  lightGuideTheme();
/** Colors mixed: 0 = a, 1 = b. */
QColor      blendColors(const QColor &a, const QColor &b, qreal t);

/** Emits changed() when the colors in use change (the choice was changed, or the system switched while following it). */
class GuideThemeNotifier : public QObject
{
    Q_OBJECT
public:
    static GuideThemeNotifier *instance();
signals:
    void changed();
};

/**
 * Styles a label as a status ("ok", "warning", "error" or empty = plain). The colors come from the application's
 * stylesheet, so the label follows a change of the theme.
 */
void setStatusStyle(QWidget *label, const QString &status);

/** Starts following the system's color scheme (for the System choice). Call once after the application exists. */
void startFollowingSystemTheme();

/**
 * For tests: act as if the system's color scheme were this one (pretend = false goes back to the real one). The
 * offscreen platform used by the tests cannot switch its scheme. Restyles as a real change would.
 */
void pretendSystemColorScheme(Qt::ColorScheme scheme, bool pretend = true);

#endif
