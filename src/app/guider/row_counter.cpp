#include "row_counter.h"

#include <QSettings>

#include "gpio_row_counter.h"
#include "simulated_row_counter.h"

RowCounterSource::RowCounterSource(int pulsesPerRow, QObject *parent)
    : QObject(parent), m_pulses(pulsesPerRow)
{
}

void RowCounterSource::pulse(int direction)
{
    const int rows = m_pulses.add(direction < 0 ? -1 : 1);
    if (rows != 0)
    {
        emit rowsStepped(rows);
    }
}

void RowCounterSource::setState(State state, const QString &message)
{
    if (state == m_state && message == m_message)
    {
        return;
    }
    m_state = state;
    m_message = message;
    emit stateChanged(state, message);
}

QString RowCounterConfig::problem() const
{
    if (kind != QLatin1String("manual") && kind != QLatin1String("gpio") && kind != QLatin1String("simulated"))
    {
        return QObject::tr("Unknown row counter \"%1\" (use manual, gpio or simulated).").arg(kind);
    }
    if (pulsesPerRow < 1)
    {
        return QObject::tr("Pulses per row must be 1 or more.");
    }
    if (kind == QLatin1String("gpio"))
    {
        if (line < 0)
        {
            return QObject::tr("The GPIO counter needs a line (pin number).");
        }
        if (edge != QLatin1String("rising") && edge != QLatin1String("falling") && edge != QLatin1String("both"))
        {
            return QObject::tr("GPIO edge must be rising, falling or both.");
        }
        if (pull != QLatin1String("up") && pull != QLatin1String("down") && pull != QLatin1String("none"))
        {
            return QObject::tr("GPIO pull must be up, down or none.");
        }
        if (debounceMs < 0)
        {
            return QObject::tr("GPIO debounce must be 0 or more.");
        }
    }
    if (kind == QLatin1String("simulated") && simulatedPeriodMs < 0)
    {
        return QObject::tr("The simulated period must be 0 or more.");
    }
    return QString();
}

void RowCounterConfig::load(QSettings &s)
{
    s.beginGroup(QStringLiteral("counter"));
    kind = s.value(QStringLiteral("kind"), kind).toString();
    chip = s.value(QStringLiteral("chip"), chip).toString();
    line = s.value(QStringLiteral("line"), line).toInt();
    edge = s.value(QStringLiteral("edge"), edge).toString();
    pull = s.value(QStringLiteral("pull"), pull).toString();
    debounceMs = s.value(QStringLiteral("debounceMs"), debounceMs).toInt();
    pulsesPerRow = s.value(QStringLiteral("pulsesPerRow"), pulsesPerRow).toInt();
    simulatedPeriodMs = s.value(QStringLiteral("simulatedPeriodMs"), simulatedPeriodMs).toInt();
    s.endGroup();
}

void RowCounterConfig::save(QSettings &s) const
{
    s.beginGroup(QStringLiteral("counter"));
    s.setValue(QStringLiteral("kind"), kind);
    s.setValue(QStringLiteral("chip"), chip);
    s.setValue(QStringLiteral("line"), line);
    s.setValue(QStringLiteral("edge"), edge);
    s.setValue(QStringLiteral("pull"), pull);
    s.setValue(QStringLiteral("debounceMs"), debounceMs);
    s.setValue(QStringLiteral("pulsesPerRow"), pulsesPerRow);
    s.setValue(QStringLiteral("simulatedPeriodMs"), simulatedPeriodMs);
    s.endGroup();
}

QString RowCounterConfig::describe() const
{
    if (kind == QLatin1String("gpio"))
    {
        return QObject::tr("GPIO %1 line %2").arg(chip).arg(line);
    }
    if (kind == QLatin1String("simulated"))
    {
        return QObject::tr("simulated");
    }
    return QObject::tr("buttons");
}

RowCounterSource *createRowCounter(const RowCounterConfig &config, QString *error, QObject *parent)
{
    const QString problem = config.problem();
    if (!problem.isEmpty())
    {
        if (error) *error = problem;
        return nullptr;
    }
    if (config.kind == QLatin1String("gpio"))
    {
        return new GpioRowCounter(config, parent);
    }
    if (config.kind == QLatin1String("simulated"))
    {
        return new SimulatedRowCounter(config.simulatedPeriodMs, config.pulsesPerRow, parent);
    }
    return nullptr;
}
