#include "step_view.h"

#include <QFont>

#include "guide_theme.h"
#include <QMouseEvent>
#include <QPainter>
#include <QTextOption>
#include <QtMath>

namespace
{
// Sizes relative to the height of the widget, so they follow the screen.
qreal currentPx(const QWidget *w) { return qMax<qreal>(12, w->height() / 14.0); }
qreal otherPx(const QWidget *w)   { return qMax<qreal>(10, w->height() / 22.0); }
}

StepView::StepView(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    m_anim.setDuration(250);
    m_anim.setEasingCurve(QEasingCurve::OutCubic);
    connect(GuideThemeNotifier::instance(), &GuideThemeNotifier::changed, this, [this]() { update(); });
    connect(&m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_offset = v.toReal();
        update();
    });
}

void StepView::setSteps(const QStringList &steps)
{
    m_steps = steps;
    m_current.clear();
    layoutItems();
    m_offset = targetOffset();
    update();
}

void StepView::setCurrent(const QVector<int> &current, bool animate)
{
    m_current = current;
    layoutItems();
    const qreal target = targetOffset();
    m_anim.stop();
    if (animate && isVisible())
    {
        m_anim.setStartValue(m_offset);
        m_anim.setEndValue(target);
        m_anim.start();
    }
    else
    {
        m_offset = target;
    }
    update();
}

void StepView::layoutItems()
{
    m_items.clear();
    const qreal margin = width() * 0.04;
    const qreal gap = height() / 40.0;
    qreal y = 0;
    for (int i = 0; i < m_steps.size(); ++i)
    {
        Item it;
        it.text = m_steps.at(i);
        it.current = m_current.contains(i);
        QFont f = font();
        f.setPixelSize(qRound(it.current ? currentPx(this) : otherPx(this)));
        f.setBold(it.current);
        const QRectF bounds = QFontMetricsF(f).boundingRect(QRectF(0, 0, width() - 2 * margin, 1e6),
                                                           Qt::TextWordWrap, it.text);
        it.box = QRectF(margin, y, width() - 2 * margin, bounds.height());
        y += bounds.height() + gap;
        m_items << it;
    }
    m_contentHeight = y;
}

qreal StepView::targetOffset() const
{
    // Roll so that the middle of the current steps is in the middle of the view; at the ends there is padding.
    qreal top = -1, bottom = -1;
    for (int i : m_current)
    {
        if (i >= 0 && i < m_items.size())
        {
            if (top < 0) top = m_items.at(i).box.top();
            bottom = m_items.at(i).box.bottom();
        }
    }
    if (top < 0) return 0;
    // The middle of the current steps goes to the middle of the view, but the text never rolls past its ends.
    const qreal wanted = (top + bottom) / 2 - height() / 2.0;
    return qBound<qreal>(0, wanted, qMax<qreal>(0, m_contentHeight - height()));
}

void StepView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutItems();
    m_anim.stop();
    m_offset = targetOffset();
}

void StepView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    const GuideTheme theme = guideTheme();
    p.fillRect(rect(), theme.view);
    p.setRenderHint(QPainter::TextAntialiasing);
    QTextOption option;
    option.setWrapMode(QTextOption::WordWrap);
    for (const Item &it : qAsConst(m_items))
    {
        const QRectF box = it.box.translated(0, -m_offset);
        if (box.bottom() < 0 || box.top() > height())
        {
            continue;
        }
        QFont f = font();
        f.setPixelSize(qRound(it.current ? currentPx(this) : otherPx(this)));
        f.setBold(it.current);
        p.setFont(f);
        if (it.current)
        {
            p.fillRect(box.adjusted(-width() * 0.02, -height() / 80.0, width() * 0.02, height() / 80.0),
                       theme.stepBackground);
        }
        // Fade with distance from the middle.
        const qreal distance = qAbs(box.center().y() - height() / 2.0) / (height() / 2.0);
        // The steps that are not current fade towards the background the further they are from the middle.
        p.setPen(it.current ? theme.text : blendColors(theme.text, theme.view, qBound<qreal>(0.3, 0.3 + 0.4 * distance, 0.7)));
        p.drawText(box, it.text, option);
    }
}

void StepView::mousePressEvent(QMouseEvent *event)
{
    m_dragging = true;
    m_moved = false;
    m_pressPos = event->position();
    m_pressOffset = m_offset;
    m_anim.stop();
}

void StepView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging)
    {
        return;
    }
    const qreal dy = event->position().y() - m_pressPos.y();
    if (qAbs(dy) > height() / 40.0)
    {
        m_moved = true;
    }
    if (m_moved)
    {
        m_offset = m_pressOffset - dy;
        update();
    }
}

void StepView::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    if (!m_moved)
    {
        // A tap in the upper or lower quarter steps back / forward.
        if (event->position().y() < height() * 0.25) emit tappedAbove();
        else if (event->position().y() > height() * 0.75) emit tappedBelow();
    }
    // Roll back to the current step.
    m_anim.setStartValue(m_offset);
    m_anim.setEndValue(targetOffset());
    m_anim.start();
}
