#include "display.h"

#include <QPainter>
#include <QWidget>

#ifndef QT_DISPLAY_H
#define QT_DISPLAY_H

class QtDisplay : public Display, public QWidget {
	int width;
	int height;

	uint8_t* actual_framebuf;

	void convert_framebufs();

protected:
	void paintEvent(QPaintEvent* e);
	void timerEvent(QTimerEvent* e);
	void keyPressEvent(QKeyEvent* e);

public:
	QtDisplay(int width, int height);
	~QtDisplay();

	void swap_buf() override;
};

#endif
