// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef GPIO_ROW_COUNTER_H
#define GPIO_ROW_COUNTER_H

#include "row_counter.h"

class QSocketNotifier;
struct gpiod_line_request;
struct gpiod_edge_event_buffer;

/**
 * Counts pulses on one GPIO line (a reed switch or optical sensor on the machine's carriage) with libgpiod 2.x.
 * Edge events arrive on a file descriptor that the event loop watches, so no extra thread is needed.
 * Built without libgpiod 2.x, start() just fails with a message.
 */
class GpioRowCounter : public RowCounterSource
{
    Q_OBJECT
public:
    explicit GpioRowCounter(const RowCounterConfig &config, QObject *parent = nullptr);
    ~GpioRowCounter() override;

    bool    start(QString *error = nullptr) override;
    void    stop() override;
    QString description() const override;

    /** True if this build has libgpiod support. */
    static bool available();

private:
    void readEvents();

    RowCounterConfig          m_config;
    gpiod_line_request       *m_request = nullptr;
    gpiod_edge_event_buffer  *m_buffer = nullptr;
    QSocketNotifier          *m_notifier = nullptr;
};

#endif
