#include "display.h"

#include <QPainter>
#include <QWidget>
#include <mutex>
#include <atomic>

#ifndef QT_DISPLAY_H
#define QT_DISPLAY_H

class QtDisplay : public Display, public QWidget {
	int width;
	int height;
	int scale;

	std::mutex framebuf_mutex;
	uint8_t* actual_framebuf;
	std::atomic<bool> needs_repaint;

	void convert_framebufs();

protected:
	void paintEvent(QPaintEvent* e) override;
	void timerEvent(QTimerEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;

public:
	QtDisplay(int width, int height, int scale=4);
	~QtDisplay();

	void swap_buf() override;
};

#endif
