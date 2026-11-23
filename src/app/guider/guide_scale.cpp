#include "guide_scale.h"

#include <QApplication>
#include <QFont>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QtGlobal>

int guideUnit(const QWidget *widget)
{
    // GUIDER_UNIT overrides the unit (tests on the offscreen platform, whose screen is always 800x600).
    bool ok = false;
    const int forced = qEnvironmentVariableIntValue("GUIDER_UNIT", &ok);
    if (ok && forced > 0)
    {
        return forced;
    }
    const QScreen *screen = widget ? widget->screen() : QGuiApplication::primaryScreen();
    const int height = screen ? screen->size().height() : 1080;
    return qMax(9, height / 54);
}

void applyGuideStyle(QApplication &app)
{
    const int u = guideUnit();
    QFont font = app.font();
    font.setPixelSize(u);
    app.setFont(font);
    // Touch targets: at least 2.4 units high; selection and scrollbars big enough for a finger.
    app.setStyleSheet(QStringLiteral(
        "QPushButton { min-height: %1px; padding: 0 %2px; border: 1px solid #555; border-radius: %3px;"
        "  background: #2b2b2b; color: #eee; }"
        "QPushButton:pressed { background: #4a6fa5; }"
        "QPushButton:disabled { color: #777; }"
        "QListWidget { background: #181818; color: #eee; border: 1px solid #444; }"
        "QListWidget::item { min-height: %4px; padding: %5px; }"
        "QListWidget::item:selected { background: #4a6fa5; color: white; }"
        "QDialog, QFileDialog, QInputDialog, QMessageBox { background: #202020; color: #eee; }"
        "QLabel { color: #eee; }"
        "QLineEdit, QSpinBox, QComboBox, QTreeView, QListView { background: #181818; color: #eee; min-height: %6px; }"
        "QScrollBar:vertical { width: %7px; background: #222; }"
        "QScrollBar::handle:vertical { background: #666; min-height: %6px; border-radius: %3px; }")
        .arg(u * 24 / 10).arg(u).arg(u / 2).arg(u * 24 / 10).arg(u / 3).arg(u * 2).arg(u * 3 / 2));
}
