#include "gpio_row_counter.h"

#include <QSocketNotifier>

#ifdef HAVE_GPIOD
#include <gpiod.h>
#include <cerrno>
#include <cstring>
#endif

GpioRowCounter::GpioRowCounter(const RowCounterConfig &config, QObject *parent)
    : RowCounterSource(config.pulsesPerPass, parent), m_config(config)
{
}

GpioRowCounter::~GpioRowCounter()
{
    stop();
}

QString GpioRowCounter::description() const
{
    return tr("GPIO %1 line %2").arg(m_config.chip).arg(m_config.line);
}

#ifndef HAVE_GPIOD

bool GpioRowCounter::available() { return false; }

bool GpioRowCounter::start(QString *error)
{
    const QString message = tr("This build has no GPIO support (libgpiod 2.x was not found).");
    setState(State::Error, message);
    if (error) *error = message;
    return false;
}

void GpioRowCounter::stop() {}
void GpioRowCounter::readEvents() {}

#else

bool GpioRowCounter::available() { return true; }

bool GpioRowCounter::start(QString *error)
{
    stop();
    auto fail = [this, error](const QString &what) {
        const QString message = tr("%1: %2").arg(description(), what);
        setState(State::Error, message);
        if (error) *error = message;
        return false;
    };

    const QString path = m_config.chip.startsWith(QLatin1Char('/')) ? m_config.chip
                                                                    : QStringLiteral("/dev/") + m_config.chip;
    gpiod_chip *chip = gpiod_chip_open(path.toLocal8Bit().constData());
    if (!chip)
    {
        const int e = errno;
        return fail(e == EACCES ? tr("no permission for %1 (is the user in the gpio group?)").arg(path)
                                : tr("cannot open %1 (%2)").arg(path, QString::fromLocal8Bit(std::strerror(e))));
    }

    gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_config   *lines = gpiod_line_config_new();
    gpiod_request_config *request = gpiod_request_config_new();
    bool ok = settings && lines && request;
    if (ok)
    {
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
        gpiod_line_settings_set_edge_detection(settings,
            m_config.edge == QLatin1String("rising")  ? GPIOD_LINE_EDGE_RISING
          : m_config.edge == QLatin1String("falling") ? GPIOD_LINE_EDGE_FALLING
                                                      : GPIOD_LINE_EDGE_BOTH);
        gpiod_line_settings_set_bias(settings,
            m_config.pull == QLatin1String("up")   ? GPIOD_LINE_BIAS_PULL_UP
          : m_config.pull == QLatin1String("down") ? GPIOD_LINE_BIAS_PULL_DOWN
                                                   : GPIOD_LINE_BIAS_DISABLED);
        gpiod_line_settings_set_debounce_period_us(settings, static_cast<unsigned long>(m_config.debounceMs) * 1000UL);
        const unsigned int offset = static_cast<unsigned int>(m_config.line);
        ok = gpiod_line_config_add_line_settings(lines, &offset, 1, settings) == 0;
        gpiod_request_config_set_consumer(request, "guider");
    }
    if (ok)
    {
        m_request = gpiod_chip_request_lines(chip, request, lines);
    }
    const int requestErrno = errno;
    gpiod_request_config_free(request);
    gpiod_line_config_free(lines);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
    if (!m_request)
    {
        return fail(requestErrno == EBUSY ? tr("the line is already in use by another program")
                  : requestErrno == EINVAL || requestErrno == ENOENT ? tr("this chip has no such line")
                  : requestErrno == EACCES || requestErrno == EPERM ? tr("no permission (is the user in the gpio group?)")
                                                                     : tr("cannot request the line (%1)").arg(QString::fromLocal8Bit(std::strerror(requestErrno))));
    }

    m_buffer = gpiod_edge_event_buffer_new(16);
    m_notifier = new QSocketNotifier(gpiod_line_request_get_fd(m_request), QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, [this]() { readEvents(); });
    setState(State::Running, description());
    return true;
}

void GpioRowCounter::readEvents()
{
    if (!m_request || !m_buffer)
    {
        return;
    }
    const int n = gpiod_line_request_read_edge_events(m_request, m_buffer, 16);
    if (n < 0)
    {
        const QString message = tr("%1: reading failed (%2)").arg(description(), QString::fromLocal8Bit(std::strerror(errno)));
        stop();
        setState(State::Error, message);
        return;
    }
    for (int i = 0; i < n; ++i)
    {
        pulse(1);           // one sensor: every counted edge is a forward pulse
    }
}

void GpioRowCounter::stop()
{
    delete m_notifier;
    m_notifier = nullptr;
    if (m_buffer)
    {
        gpiod_edge_event_buffer_free(m_buffer);
        m_buffer = nullptr;
    }
    if (m_request)
    {
        gpiod_line_request_release(m_request);
        m_request = nullptr;
    }
    if (state() == State::Running)
    {
        setState(State::Stopped);
    }
}

#endif
