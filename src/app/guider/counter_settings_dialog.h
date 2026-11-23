// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef COUNTER_SETTINGS_DIALOG_H
#define COUNTER_SETTINGS_DIALOG_H

#include <QButtonGroup>
#include <QDialog>

#include "row_counter.h"

class QLabel;
class QPushButton;
class QToolButton;

/** One row of a settings page: a label and a number with big minus / plus buttons (spin boxes are too small for a finger). */
class StepperRow : public QWidget
{
    Q_OBJECT
public:
    StepperRow(const QString &label, int min, int max, int step, QWidget *parent = nullptr);
    int  value() const { return m_value; }
    void setValue(int value);

signals:
    void changed(int value);

private:
    QLabel      *m_label = nullptr;
    QLabel      *m_number = nullptr;
    int          m_value = 0, m_min, m_max, m_step;
};

/** One row of a settings page: a label and a few big buttons, exactly one of them chosen. */
class ChoiceRow : public QWidget
{
    Q_OBJECT
public:
    ChoiceRow(const QString &label, QWidget *parent = nullptr);
    void    addChoice(const QString &text, const QString &value, bool enabled = true);
    QString value() const;
    /** Selects the choice with this value, adding it first if it is not there (e.g. a chip that is not plugged in now). */
    void    setValue(const QString &value);

signals:
    void changed(const QString &value);

private:
    QLabel       *m_label = nullptr;
    QWidget      *m_box = nullptr;
    QButtonGroup  m_group;
};

/**
 * Touch-friendly settings of the row counter: which one, and for GPIO the chip, pin, edge, pull, debounce and
 * pulses per row. "Test" listens with the values shown and counts pulses, to check the wiring.
 */
class CounterSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    CounterSettingsDialog(const RowCounterConfig &config, QWidget *parent = nullptr);
    ~CounterSettingsDialog() override;

    RowCounterConfig config() const;

private:
    void updateVisibility();
    void toggleTest();
    void stopTest();

    ChoiceRow   *m_kind = nullptr;
    ChoiceRow   *m_chip = nullptr;
    StepperRow  *m_line = nullptr;
    ChoiceRow   *m_edge = nullptr;
    ChoiceRow   *m_pull = nullptr;
    StepperRow  *m_debounce = nullptr;
    StepperRow  *m_pulses = nullptr;
    StepperRow  *m_period = nullptr;
    QWidget     *m_gpioBox = nullptr;
    QWidget     *m_simBox = nullptr;
    QPushButton *m_testButton = nullptr;
    QLabel      *m_testLabel = nullptr;
    RowCounterSource *m_test = nullptr;
    int          m_testPulses = 0;
    int          m_testRows = 0;
};

#endif
