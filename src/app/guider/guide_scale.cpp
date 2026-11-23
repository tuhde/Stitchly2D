#include "guide_scale.h"

#include "guide_theme.h"

#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QWidget>
#include <QPair>
#include <QList>
#include <QtGlobal>

QScreen *guideScreen()
{
    const QString saved = QSettings(QStringLiteral("Stitchly2D"), QStringLiteral("guider")).value(QStringLiteral("screen")).toString();
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens)
    {
        if (!saved.isEmpty() && screen->name() == saved)
        {
            return screen;
        }
    }
    return screens.isEmpty() ? QGuiApplication::primaryScreen() : screens.first();
}

void saveGuideScreen(const QScreen *screen)
{
    if (screen)
    {
        QSettings(QStringLiteral("Stitchly2D"), QStringLiteral("guider")).setValue(QStringLiteral("screen"), screen->name());
    }
}

int guideUnit(const QWidget *widget)
{
    // GUIDER_UNIT overrides the unit (tests on the offscreen platform, whose screen is always 800x600).
    bool ok = false;
    const int forced = qEnvironmentVariableIntValue("GUIDER_UNIT", &ok);
    if (ok && forced > 0)
    {
        return forced;
    }
    const QScreen *screen = widget ? widget->screen() : guideScreen();
    const int height = screen ? screen->size().height() : 1080;
    return qMax(9, height / 54);
}

void applyGuideStyle(QApplication &app)
{
    const int u = guideUnit();
    QFont font = app.font();
    font.setPixelSize(u);
    app.setFont(font);

    const GuideTheme t = guideTheme();
    auto c = [](const QColor &color) { return color.name(); };
    const QString sheet = QStringLiteral(
        // Touch targets: at least 2.4 units high; borders strong enough to see where a button is.
        "QWidget { color: %c_text; }"
        "QDialog, QFileDialog, QInputDialog, QMessageBox, GuideWindow { background: %c_window; }"
        "QLabel { color: %c_text; background: transparent; }"
        "QLabel[status=\"ok\"] { color: %c_ok; }"
        "QLabel[status=\"warning\"] { color: %c_warning; }"
        "QLabel[status=\"error\"] { color: %c_error; }"
        "QPushButton { min-height: %h_button; padding: 0 %pad; border: 2px solid %c_border;"
        "  border-radius: %radius; background: %c_button; color: %c_text; }"
        "QPushButton:pressed { background: %c_pressed; }"
        "QPushButton:checked { background: %c_accent; color: %c_accentText; border-color: %c_text; }"
        "QPushButton:disabled { color: %c_disabled; border-color: %c_disabled; }"
        "QListWidget, QListView, QTreeView, QTableView { background: %c_view; color: %c_text; border: 1px solid %c_border; }"
        "QListWidget::item { min-height: %h_button; padding: %pad3; border-bottom: 1px solid %c_disabled; }"
        "QListWidget::item:selected, QListView::item:selected, QTreeView::item:selected { background: %c_accent; color: %c_accentText; }"
        "QLineEdit, QSpinBox, QComboBox { background: %c_view; color: %c_text; border: 1px solid %c_border; min-height: %h_field; }"
        "QHeaderView::section { background: %c_button; color: %c_text; border: 1px solid %c_border; padding: 2px; }"
        "QScrollBar:vertical { width: %w_bar; background: %c_window; }"
        "QScrollBar:horizontal { height: %w_bar; background: %c_window; }"
        "QScrollBar::handle:vertical { background: %c_border; min-height: %h_field; border-radius: %radius; }"
        "QScrollBar::handle:horizontal { background: %c_border; min-width: %h_field; border-radius: %radius; }"
        "QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }"
        "#settingsScroll, #settingsScroll > QWidget > QWidget { background: %c_window; }");
    QString out = sheet;
    const QList<QPair<QString, QString>> values = {
        {QStringLiteral("%c_text"), c(t.text)}, {QStringLiteral("%c_window"), c(t.window)},
        {QStringLiteral("%c_ok"), c(t.ok)}, {QStringLiteral("%c_warning"), c(t.warning)}, {QStringLiteral("%c_error"), c(t.error)},
        {QStringLiteral("%c_border"), c(t.border)}, {QStringLiteral("%c_button"), c(t.button)},
        {QStringLiteral("%c_pressed"), c(t.buttonPressed)}, {QStringLiteral("%c_accentText"), c(t.accentText)},
        {QStringLiteral("%c_accent"), c(t.accent)}, {QStringLiteral("%c_disabled"), c(t.textDisabled)},
        {QStringLiteral("%c_view"), c(t.view)},
        {QStringLiteral("%h_button"), QString::number(u * 24 / 10) + QStringLiteral("px")}, {QStringLiteral("%h_field"), QString::number(u * 2) + QStringLiteral("px")},
        {QStringLiteral("%pad3"), QString::number(u / 3) + QStringLiteral("px")}, {QStringLiteral("%pad"), QString::number(u) + QStringLiteral("px")},
        {QStringLiteral("%radius"), QString::number(u / 2) + QStringLiteral("px")}, {QStringLiteral("%w_bar"), QString::number(u * 3 / 2) + QStringLiteral("px")}};
    for (const auto &v : values) out.replace(v.first, v.second);
    app.setStyleSheet(out);
    emit GuideThemeNotifier::instance()->changed();
}
