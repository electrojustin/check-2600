#include "display.h"

#include <QSoundEffect>
#include <QPainter>
#include <QWidget>
#include <mutex>
#include <atomic>
#include <memory>
#include <vector>

#ifndef QT_DISPLAY_H
#define QT_DISPLAY_H

class QtDisplay : public Display, public QWidget {
	int width;
	int height;
	int scale;

	std::mutex framebuf_mutex;
	uint8_t* actual_framebuf;
	std::atomic<bool> needs_repaint;

	std::shared_ptr<QSoundEffect> channel0 = nullptr;
	int channel0_index = 0; // Combo of freq and noise control to know when to change sounds.
	int prev_volume0 = 0;
	std::shared_ptr<QSoundEffect> channel1 = nullptr;
	int channel1_index = 0;
	int prev_volume1 = 0;

	void convert_framebufs();

	void handle_sound_channel_update(std::shared_ptr<QSoundEffect>& channel, 
					 int& channel_index,
					 int& prev_volume,
					 int volume,
					 int freq,
					 int noise_control);
	void handle_sound_updates();

protected:
	void paintEvent(QPaintEvent* e) override;
	void timerEvent(QTimerEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;
	void keyReleaseEvent(QKeyEvent* e) override;

public:
	QtDisplay(int width, int height, int scale=4);
	~QtDisplay();

	void swap_buf() override;
};

#endif
