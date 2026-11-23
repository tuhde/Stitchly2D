#include "simulated_row_counter.h"

SimulatedRowCounter::SimulatedRowCounter(int periodMs, int pulsesPerPass, QObject *parent)
    : RowCounterSource(pulsesPerPass, parent)
{
    m_timer.setInterval(qMax(1, periodMs));
    m_automatic = periodMs > 0;
    connect(&m_timer, &QTimer::timeout, this, [this]() { pulse(1); });
}

bool SimulatedRowCounter::start(QString *)
{
    if (m_automatic)
    {
        m_timer.start();
    }
    setState(State::Running, description());
    return true;
}

void SimulatedRowCounter::stop()
{
    m_timer.stop();
    setState(State::Stopped);
}

void SimulatedRowCounter::injectPulse(int direction)
{
    if (state() == State::Running)
    {
        pulse(direction);
    }
}
