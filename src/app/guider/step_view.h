// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef STEP_VIEW_H
#define STEP_VIEW_H

#include <QStringList>
#include <QVariantAnimation>
#include <QWidget>

/**
 * Rolling instruction text: all steps of a piece in one column. The current steps are big and bright and rolled to
 * the middle, the others are smaller and dimmer. Sizes follow the widget's height, so it fits any screen.
 */
class StepView : public QWidget
{
    Q_OBJECT
public:
    explicit StepView(QWidget *parent = nullptr);

    /** All steps of the piece, in order. */
    void setSteps(const QStringList &steps);
    /** The steps that are current now (indexes into the list given to setSteps()). */
    void setCurrent(const QVector<int> &current, bool animate = true);

    /** Vertical position of the middle of the first current step as of the last layout; for tests. */
    int  scrollOffset() const { return qRound(m_offset); }

signals:
    /** Drag up/down to look ahead; the view rolls back to the current step when released. A tap in the top or bottom quarter: */
    void tappedAbove();
    void tappedBelow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    struct Item { QString text; QRectF box; bool current = false; };
    void  layoutItems();
    qreal targetOffset() const;

    QStringList         m_steps;
    QVector<int>        m_current;
    QVector<Item>       m_items;
    qreal               m_contentHeight = 0;
    qreal               m_offset = 0;       //!< scroll position of the content top, in pixels (>= 0)
    QVariantAnimation   m_anim;
    bool                m_dragging = false;
    QPointF             m_pressPos;
    qreal               m_pressOffset = 0;
    bool                m_moved = false;
};

#endif
