#include "stitchtype.h"

namespace stitchcore
{

StitchLibrary StitchLibrary::withDefaults()
{
    StitchLibrary lib;
    lib.set({QStringLiteral("Stockinette"), StitchSize{5.0, 3.5}});
    lib.set({QStringLiteral("Garter"),      StitchSize{5.0, 2.5}});
    lib.set({QStringLiteral("2x2 rib"),     StitchSize{3.5, 3.5}});
    return lib;
}

bool StitchLibrary::contains(const QString &name) const
{
    for (const StitchType &t : m_types)
    {
        if (t.name == name) return true;
    }
    return false;
}

StitchType StitchLibrary::find(const QString &name) const
{
    for (const StitchType &t : m_types)
    {
        if (t.name == name) return t;
    }
    return StitchType();
}

void StitchLibrary::set(const StitchType &type)
{
    for (StitchType &t : m_types)
    {
        if (t.name == type.name)
        {
            t = type;
            return;
        }
    }
    m_types.append(type);
}

bool StitchLibrary::remove(const QString &name)
{
    for (int i = 0; i < m_types.size(); ++i)
    {
        if (m_types.at(i).name == name)
        {
            m_types.removeAt(i);
            return true;
        }
    }
    return false;
}

}
