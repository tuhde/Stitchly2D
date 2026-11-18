// Stitchly2D - pattern preparation for knitting. GPLv3, see LICENSE.
//
// The class keeps the name Application2D on purpose: xml/vpattern.cpp is copied from Seamly2D and refers to it
// (Seamly2DSettings(), isGUIMode(), pointNameLanguages()). Keeping the name keeps the copy close to upstream.
#ifndef APPLICATION_2D_H
#define APPLICATION_2D_H

#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vsysexits.h"

class VSettings;
class VTranslateVars;

class Application2D;
#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<Application2D*>(VAbstractApplication::instance()))

/**
 * @brief Stitchly2D application object.
 *
 * Runs without a visible main window (use QT_QPA_PLATFORM=offscreen for command-line use), so
 * isAppInGUIMode() is false for now and tools never open dialogs while a pattern is loaded.
 */
class Application2D : public VAbstractApplication
{
    Q_OBJECT
public:
    Application2D(int &argc, char **argv);
    ~Application2D() override = default;

    const VTranslateVars *translateVariables() override;
    void                  openSettings() override;
    bool                  isAppInGUIMode() const override;
    QString               appTranslationPrefix() const override;

    VSettings            *Seamly2DSettings();

    static bool           isGUIMode();
    static QStringList    pointNameLanguages();

protected:
    void                  initTranslateVariables() override;

private:
    Q_DISABLE_COPY(Application2D)
    VTranslateVars *m_trVars = nullptr;
};

#endif
