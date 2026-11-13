#include "application_2d.h"

#include <QDir>
#include <QStandardPaths>
#include <QUndoStack>

#include "../vmisc/vsettings.h"
#include "../vpatterndb/vtranslatevars.h"

Application2D::Application2D(int &argc, char **argv)
    : VAbstractApplication(argc, argv)
{
    setApplicationName(QStringLiteral("Stitchly2D"));
    setOrganizationName(QStringLiteral("Stitchly2D"));
    undoStack = new QUndoStack(this);
    openSettings();
    initTranslateVariables();
    // Pieces are measured from their seam line; labels need a document and are not used.
    Settings()->setShowGrainlines(true);
    Settings()->setShowLabels(false);
}

const VTranslateVars *Application2D::translateVariables()
{
    return m_trVars;
}

void Application2D::openSettings()
{
    // Own settings file; never touch the settings of an installed Seamly2D.
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(dir);
    m_settings = new VSettings(dir + QStringLiteral("/stitchly2d.ini"), QSettings::IniFormat, this);
}

bool Application2D::isAppInGUIMode() const
{
    return false;
}

VSettings *Application2D::Seamly2DSettings()
{
    return qobject_cast<VSettings *>(m_settings);
}

bool Application2D::isGUIMode()
{
    return false;
}

QStringList Application2D::pointNameLanguages()
{
    return QStringList() << "de" << "en" << "fr" << "ru" << "uk" << "hr" << "sr" << "bs";
}

void Application2D::initTranslateVariables()
{
    if (m_trVars == nullptr)
    {
        m_trVars = new VTranslateVars();
    }
}
