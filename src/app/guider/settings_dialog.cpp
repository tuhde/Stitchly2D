#include "settings_dialog.h"

#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "gpio_row_counter.h"
#include "guide_scale.h"
#include "guide_theme.h"

StepperRow::StepperRow(const QString &label, int min, int max, int step, QWidget *parent)
    : QWidget(parent), m_min(min), m_max(max), m_step(step)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_label = new QLabel(label, this);
    auto *minus = new QPushButton(QStringLiteral("-"), this);
    auto *plus = new QPushButton(QStringLiteral("+"), this);
    m_number = new QLabel(this);
    m_number->setAlignment(Qt::AlignCenter);
    const int u = guideUnit(this);
    m_number->setMinimumWidth(u * 6);
    for (QPushButton *b : {minus, plus})
    {
        b->setAutoRepeat(true);                 // hold to count on
        b->setAutoRepeatDelay(400);
        b->setAutoRepeatInterval(80);
        b->setFocusPolicy(Qt::NoFocus);
        b->setMinimumWidth(u * 4);
    }
    layout->addWidget(m_label, 1);
    layout->addWidget(minus);
    layout->addWidget(m_number);
    layout->addWidget(plus);
    connect(minus, &QPushButton::clicked, this, [this]() { setValue(m_value - m_step); });
    connect(plus, &QPushButton::clicked, this, [this]() { setValue(m_value + m_step); });
    setValue(min);
}

void StepperRow::setValue(int value)
{
    value = qBound(m_min, value, m_max);
    m_number->setText(QString::number(value));
    if (value != m_value)
    {
        m_value = value;
        emit changed(value);
    }
}

ChoiceRow::ChoiceRow(const QString &label, QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    m_label = new QLabel(label, this);
    m_box = new QWidget(this);
    auto *boxLayout = new QHBoxLayout(m_box);
    boxLayout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_label, 1);
    layout->addWidget(m_box);
    m_group.setExclusive(true);
    connect(&m_group, &QButtonGroup::buttonClicked, this, [this](QAbstractButton *b) {
        emit changed(b->property("value").toString());
    });
}

void ChoiceRow::addChoice(const QString &text, const QString &value, bool enabled)
{
    auto *button = new QPushButton(text, m_box);
    button->setCheckable(true);
    button->setEnabled(enabled);
    button->setFocusPolicy(Qt::NoFocus);
    button->setProperty("value", value);
    m_box->layout()->addWidget(button);
    m_group.addButton(button);
    if (m_group.buttons().size() == 1 && enabled)
    {
        button->setChecked(true);
    }
}

QString ChoiceRow::value() const
{
    const QAbstractButton *b = m_group.checkedButton();
    return b ? b->property("value").toString() : QString();
}

void ChoiceRow::setValue(const QString &value)
{
    for (QAbstractButton *b : m_group.buttons())
    {
        if (b->property("value").toString() == value)
        {
            b->setChecked(true);
            return;
        }
    }
    addChoice(value, value);
    m_group.buttons().last()->setChecked(true);
}

SettingsDialog::SettingsDialog(const RowCounterConfig &config, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    const int u = guideUnit(this);
    auto *outer = new QVBoxLayout(this);

    // Everything scrolls, so the buttons stay reachable on a small screen.
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName(QStringLiteral("settingsScroll"));
    auto *page = new QWidget(scroll);
    auto *layout = new QVBoxLayout(page);
    layout->setSpacing(u / 2);
    scroll->setWidget(page);
    outer->addWidget(scroll, 1);

    m_theme = new ChoiceRow(tr("Colors"), page);
    m_theme->addChoice(tr("System"), QStringLiteral("system"));
    m_theme->addChoice(tr("Light"), QStringLiteral("light"));
    m_theme->addChoice(tr("Dark"), QStringLiteral("dark"));
    m_theme->setValue(themeChoiceName(guideThemeChoice()));
    layout->addWidget(m_theme);

    m_kind = new ChoiceRow(tr("Row counter"), page);
    m_kind->addChoice(tr("Buttons only"), QStringLiteral("manual"));
    m_kind->addChoice(tr("GPIO sensor"), QStringLiteral("gpio"), GpioRowCounter::available());
    m_kind->addChoice(tr("Simulated"), QStringLiteral("simulated"));
    layout->addWidget(m_kind);
    if (!GpioRowCounter::available())
    {
        auto *note = new QLabel(tr("This build has no GPIO support (libgpiod 2.x was not found)."), page);
        setStatusStyle(note, QStringLiteral("warning"));
        layout->addWidget(note);
    }

    m_gpioBox = new QWidget(page);
    auto *gpio = new QVBoxLayout(m_gpioBox);
    gpio->setContentsMargins(0, 0, 0, 0);
    gpio->setSpacing(u / 2);
    m_chip = new ChoiceRow(tr("GPIO chip"), m_gpioBox);
    const QStringList chips = QDir(QStringLiteral("/dev")).entryList({QStringLiteral("gpiochip*")}, QDir::System | QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &chip : chips) m_chip->addChoice(chip, chip);
    m_chip->setValue(config.chip);           // adds it if it is not plugged in now
    m_line = new StepperRow(tr("Pin (BCM number)"), 0, 63, 1, m_gpioBox);
    m_edge = new ChoiceRow(tr("One pulse is the"), m_gpioBox);
    m_edge->addChoice(tr("falling edge"), QStringLiteral("falling"));
    m_edge->addChoice(tr("rising edge"), QStringLiteral("rising"));
    m_edge->addChoice(tr("both edges"), QStringLiteral("both"));
    m_pull = new ChoiceRow(tr("Pull resistor"), m_gpioBox);
    m_pull->addChoice(tr("up"), QStringLiteral("up"));
    m_pull->addChoice(tr("down"), QStringLiteral("down"));
    m_pull->addChoice(tr("none"), QStringLiteral("none"));
    m_debounce = new StepperRow(tr("Debounce (ms)"), 0, 200, 1, m_gpioBox);
    for (QWidget *w : {static_cast<QWidget *>(m_chip), static_cast<QWidget *>(m_line), static_cast<QWidget *>(m_edge),
                       static_cast<QWidget *>(m_pull), static_cast<QWidget *>(m_debounce)})
    {
        gpio->addWidget(w);
    }
    layout->addWidget(m_gpioBox);

    m_simBox = new QWidget(page);
    auto *sim = new QVBoxLayout(m_simBox);
    sim->setContentsMargins(0, 0, 0, 0);
    m_period = new StepperRow(tr("One pulse every (ms, 0 = never)"), 0, 10000, 100, m_simBox);
    sim->addWidget(m_period);
    layout->addWidget(m_simBox);

    m_pulses = new StepperRow(tr("Pulses per pass"), 1, 200, 1, page);
    layout->addWidget(m_pulses);

    m_testButton = new QPushButton(tr("Test"), page);
    m_testLabel = new QLabel(page);
    auto *test = new QHBoxLayout;
    test->addWidget(m_testButton);
    test->addWidget(m_testLabel, 1);
    layout->addLayout(test);
    layout->addStretch(1);

    auto *buttons = new QHBoxLayout;
    auto *ok = new QPushButton(tr("OK"), this);
    auto *cancel = new QPushButton(tr("Cancel"), this);
    buttons->addWidget(cancel);
    buttons->addStretch(1);
    buttons->addWidget(ok);
    outer->addLayout(buttons);
    connect(ok, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    m_kind->setValue(config.kind == QLatin1String("gpio") && !GpioRowCounter::available() ? QStringLiteral("manual") : config.kind);
    m_line->setValue(config.line < 0 ? 17 : config.line);
    m_edge->setValue(config.edge);
    m_pull->setValue(config.pull);
    m_debounce->setValue(config.debounceMs);
    m_pulses->setValue(config.pulsesPerPass);
    m_period->setValue(config.simulatedPeriodMs);

    connect(m_kind, &ChoiceRow::changed, this, [this]() { stopTest(); updateVisibility(); });
    connect(m_testButton, &QPushButton::clicked, this, &SettingsDialog::toggleTest);
    updateVisibility();
    resize(u * 46, u * 34);
}

SettingsDialog::~SettingsDialog()
{
    stopTest();       // gives the pin back before the guide starts its own counter
}

ThemeChoice SettingsDialog::themeChoice() const
{
    return themeChoiceFromName(m_theme->value());
}

RowCounterConfig SettingsDialog::config() const
{
    RowCounterConfig c;
    c.kind = m_kind->value();
    c.chip = m_chip->value();
    c.line = m_line->value();
    c.edge = m_edge->value();
    c.pull = m_pull->value();
    c.debounceMs = m_debounce->value();
    c.pulsesPerPass = m_pulses->value();
    c.simulatedPeriodMs = m_period->value();
    return c;
}

void SettingsDialog::updateVisibility()
{
    const QString kind = m_kind->value();
    m_gpioBox->setVisible(kind == QLatin1String("gpio"));
    m_simBox->setVisible(kind == QLatin1String("simulated"));
    m_pulses->setVisible(kind != QLatin1String("manual"));
    m_testButton->setVisible(kind != QLatin1String("manual"));
    m_testLabel->setVisible(kind != QLatin1String("manual"));
}

void SettingsDialog::toggleTest()
{
    if (m_test)
    {
        stopTest();
        return;
    }
    QString error;
    m_test = createRowCounter(config(), &error, this);
    if (!m_test || !m_test->start(&error))
    {
        setStatusStyle(m_testLabel, QStringLiteral("error"));
        m_testLabel->setText(error);
        delete m_test;
        m_test = nullptr;
        return;
    }
    m_testPulses = m_testPasses = 0;
    connect(m_test, &RowCounterSource::pulseCounted, this, [this]() { ++m_testPulses; });
    connect(m_test, &RowCounterSource::passStepped, this, [this](int d) { m_testPasses += d; });
    auto refresh = [this]() {
        setStatusStyle(m_testLabel, QStringLiteral("ok"));
        m_testLabel->setText(tr("Listening: %1 pulses, %2 passes").arg(m_testPulses).arg(m_testPasses));
    };
    connect(m_test, &RowCounterSource::pulseCounted, this, refresh);
    connect(m_test, &RowCounterSource::passStepped, this, refresh);
    refresh();
    m_testButton->setText(tr("Stop test"));
}

void SettingsDialog::stopTest()
{
    if (m_test)
    {
        m_test->stop();
        delete m_test;
        m_test = nullptr;
    }
    if (m_testButton)
    {
        m_testButton->setText(tr("Test"));
    }
    if (m_testLabel)
    {
        m_testLabel->clear();
    }
}
