// guider - fullscreen knitting guide. GPLv3, see LICENSE.
#ifndef GUIDE_SCALE_H
#define GUIDE_SCALE_H

class QWidget;
class QApplication;

/**
 * All sizes in guider come from one unit that follows the screen, so the layout works on any resolution:
 * unit = height of the screen / 54 (20 px at 1080p), at least 9 px. Fonts and touch targets are multiples of it.
 */
int guideUnit(const QWidget *widget = nullptr);

/** Sets the application font and a stylesheet (buttons, lists, dialogs) from guideUnit(). Call again after a screen change. */
void applyGuideStyle(QApplication &app);

#endif
