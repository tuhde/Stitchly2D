// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef SIMULATED_ROW_COUNTER_H
#define SIMULATED_ROW_COUNTER_H

#include <QTimer>

#include "row_counter.h"

/** Sends a pulse every period (or only when injectPulse() is called): to try the guide without a machine, and for tests. */
class SimulatedRowCounter : public RowCounterSource
{
    Q_OBJECT
public:
    explicit SimulatedRowCounter(int periodMs = 1000, int pulsesPerPass = 1, QObject *parent = nullptr);

    bool    start(QString *error = nullptr) override;
    void    stop() override;
    QString description() const override { return tr("simulated"); }

    /** One pulse as a sensor would give it (direction -1 = backwards). Only while running. */
    void    injectPulse(int direction = 1);

private:
    QTimer m_timer;
    bool   m_automatic = true;
};

#endif
