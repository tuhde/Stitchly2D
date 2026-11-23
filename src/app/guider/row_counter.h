// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef ROW_COUNTER_H
#define ROW_COUNTER_H

#include <QObject>
#include <QString>

class QSettings;

/**
 * Turns sensor pulses into rows: every `pulsesPerRow` pulses make one row. A pulse in the other direction takes
 * back what is pending first, so a carriage that turns round half way does not lose or gain a row.
 */
class PulseCounter
{
public:
    explicit PulseCounter(int pulsesPerRow = 1) { setPulsesPerRow(pulsesPerRow); }
    void setPulsesPerRow(int n) { m_perRow = n < 1 ? 1 : n; }
    int  pulsesPerRow() const { return m_perRow; }
    /** Adds pulses (negative = backwards); returns how many whole rows that completed (negative = back). */
    int  add(int pulses = 1)
    {
        m_pending += pulses;
        const int rows = m_pending / m_perRow;
        m_pending -= rows * m_perRow;
        return rows;
    }
    int  pending() const { return m_pending; }
    void reset() { m_pending = 0; }

private:
    int m_perRow = 1;
    int m_pending = 0;
};

/**
 * Where row changes come from besides the touch buttons: a sensor on the machine. A source only says how many rows
 * the work moved (+1 forward, -1 back); the guide applies it to its current row, so the user can always correct
 * the row by hand.
 */
class RowCounterSource : public QObject
{
    Q_OBJECT
public:
    enum class State { Stopped, Running, Error };
    Q_ENUM(State)

    explicit RowCounterSource(int pulsesPerRow = 1, QObject *parent = nullptr);

    /** Starts listening. Returns false (and the state becomes Error) with the reason in error. */
    virtual bool    start(QString *error = nullptr) = 0;
    virtual void    stop() = 0;
    /** Short text for the status line, e.g. "GPIO gpiochip0 line 17". */
    virtual QString description() const = 0;

    State           state() const { return m_state; }
    QString         message() const { return m_message; }
    int             pulsesPerRow() const { return m_pulses.pulsesPerRow(); }

signals:
    void rowsStepped(int delta);                        //!< rows moved, never 0
    void pulseCounted();                                //!< every sensor pulse (also those that do not complete a row)
    void stateChanged(RowCounterSource::State state, const QString &message);

protected:
    /** Call for every sensor pulse; emits rowsStepped() when a row is complete. */
    void pulse(int direction = 1);
    void setState(State state, const QString &message = QString());

private:
    PulseCounter m_pulses;
    State        m_state = State::Stopped;
    QString      m_message;
};

/** Which counter to use and how it is wired; saved in the settings, set by command-line options. */
struct RowCounterConfig
{
    QString kind = QStringLiteral("manual");        //!< "manual" (buttons only), "gpio", "simulated"
    // GPIO (libgpiod 2.x)
    QString chip = QStringLiteral("gpiochip0");     //!< name or path; the Pi 400 header is gpiochip0
    int     line = -1;                              //!< BCM pin number
    QString edge = QStringLiteral("falling");       //!< "rising", "falling" or "both": which edge is one pulse
    QString pull = QStringLiteral("up");            //!< "up", "down" or "none": internal pull resistor
    int     debounceMs = 5;                         //!< ignore changes shorter than this; 0 = off
    int     pulsesPerRow = 1;
    // Simulated
    int     simulatedPeriodMs = 1000;               //!< one pulse every ... ms; 0 = only when told to

    /** Empty if fine, else what is wrong. */
    QString problem() const;
    void    load(QSettings &settings);
    void    save(QSettings &settings) const;
    QString describe() const;
};

/** Creates the source for a config ("manual" gives nullptr without error). The caller still has to start() it. */
RowCounterSource *createRowCounter(const RowCounterConfig &config, QString *error, QObject *parent = nullptr);

#endif
