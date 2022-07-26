#include "qt_display.h"

#include <QKeyEvent>
#include <stdio.h>

#include "atari.h"
#include "registers.h"
#include "input.h"
#include "sound.h"

// NTSC color palette
// Stored in BGRA format
const uint8_t color_palette[] = {
	0, 0, 0, 255,
	0, 68, 68, 255,
	0, 40, 112, 255,
	0, 24, 132, 255,
	0, 0, 136, 255,
	92, 0, 120, 255,
	120, 0, 72, 255,
	132, 0, 20, 255,
	136, 0, 0, 255,
	124, 24, 0, 255,
	92, 44, 0, 255,
	44, 64, 0, 255,
	0, 60, 0, 255,
	0, 56, 20, 255,
	0, 48, 44, 255,
	0, 40, 68, 255,
	64, 64, 64, 255,
	16, 100, 100, 255,
	20, 68, 132, 255,
	24, 52, 152, 255,
	32, 32, 156, 255,
	116, 32, 140, 255,
	144, 32, 96, 255,
	152, 32, 48, 255,
	156, 32, 28, 255,
	144, 56, 28, 255,
	120, 76, 28, 255,
	72, 92, 28, 255,
	32, 92, 32, 255,
	28, 92, 52, 255,
	28, 80, 76, 255,
	24, 72, 100, 255,
	108, 108, 108, 255,
	36, 132, 132, 255,
	40, 92, 152, 255,
	48, 80, 172, 255,
	60, 60, 176, 255,
	136, 60, 160, 255,
	164, 60, 120, 255,
	172, 60, 76, 255,
	176, 64, 56, 255,
	168, 84, 56, 255,
	144, 104, 56, 255,
	100, 124, 56, 255,
	64, 124, 64, 255,
	56, 124, 80, 255,
	52, 112, 104, 255,
	48, 104, 132, 255,
	144, 144, 144, 255,
	52, 160, 160, 255,
	60, 120, 172, 255,
	72, 104, 192, 255,
	88, 88, 192, 255,
	156, 88, 176, 255,
	184, 88, 140, 255,
	192, 88, 104, 255,
	192, 92, 80, 255,
	188, 112, 80, 255,
	172, 132, 80, 255,
	128, 156, 80, 255,
	92, 156, 92, 255,
	80, 152, 108, 255,
	76, 140, 132, 255,
	68, 132, 160, 255,
	176, 176, 176, 255,
	64, 184, 184, 255,
	76, 140, 188, 255,
	92, 128, 208, 255,
	112, 112, 208, 255,
	176, 112, 192, 255,
	204, 112, 160, 255,
	208, 112, 124, 255,
	208, 116, 104, 255,
	204, 136, 104, 255,
	192, 156, 104, 255,
	148, 180, 104, 255,
	116, 180, 116, 255,
	104, 180, 132, 255,
	100, 168, 156, 255,
	88, 156, 184, 255,
	200, 200, 200, 255,
	80, 208, 208, 255,
	92, 160, 204, 255,
	112, 148, 224, 255,
	136, 136, 224, 255,
	192, 132, 208, 255,
	220, 132, 180, 255,
	224, 136, 148, 255,
	224, 140, 124, 255,
	220, 156, 124, 255,
	212, 180, 124, 255,
	172, 208, 124, 255,
	140, 208, 140, 255,
	124, 204, 156, 255,
	120, 192, 180, 255,
	108, 180, 208, 255,
	220, 220, 220, 255,
	92, 232, 232, 255,
	104, 180, 220, 255,
	128, 168, 236, 255,
	160, 160, 236, 255,
	208, 156, 220, 255,
	236, 156, 196, 255,
	236, 160, 168, 255,
	236, 164, 144, 255,
	236, 180, 144, 255,
	232, 204, 144, 255,
	192, 228, 144, 255,
	164, 228, 164, 255,
	144, 228, 180, 255,
	136, 212, 204, 255,
	124, 204, 232, 255,
	236, 236, 236, 255,
	104, 252, 252, 255,
	148, 188, 252, 255,
	180, 180, 252, 255,
	224, 176, 236, 255,
	252, 176, 212, 255,
	252, 180, 188, 255,
	252, 184, 164, 255,
	252, 200, 164, 255,
	252, 224, 164, 255,
	212, 252, 164, 255,
	184, 252, 184, 255,
	164, 252, 200, 255,
	156, 236, 224, 255,
	140, 224, 252, 255,
	255, 255, 255, 255,
};

void QtDisplay::convert_framebufs() {
	for (int i = 0; i < width*height; i++) {
		uint8_t val = framebuf[i];
		val = ((val & 0x0E) << 3) | ((val & 0xF0) >> 4);
		((uint32_t*)actual_framebuf)[i] = ((uint32_t*)color_palette)[val];
	}
}

void QtDisplay::handle_sound_channel_update(std::shared_ptr<QSoundEffect>& channel, 
					    int& channel_index,
					    int& prev_volume,
					    int volume,
					    int freq,
					    int noise_control) {
	int new_channel_index = (int)freq << 4 | noise_control;
	if (new_channel_index != channel_index ||
	    prev_volume != volume) {
		if (!volume || !noise_control) {
			channel->setVolume(0.0);
		} else {
			if (new_channel_index != channel_index || !prev_volume) {
				if (channel)
					channel->setVolume(0.0);
				char filename[256];
				freq = 30000/(freq+1);
				snprintf(filename, 256, "sounds/%dhz_waveform%d.wav", freq, noise_control);
				channel->setSource(QUrl::fromLocalFile(filename));
				channel->play();
			}

			channel->setVolume(0.0625 * volume);
		}

		prev_volume = volume;
		channel_index = new_channel_index;
	}
}

void QtDisplay::handle_sound_updates() {
	handle_sound_channel_update(channel0, channel0_index, prev_volume0, volume0, freq0, noise_control0);
	handle_sound_channel_update(channel1, channel1_index, prev_volume1, volume1, freq1, noise_control1);
}

QtDisplay::QtDisplay(int width, int height, int scale) : QWidget(nullptr) {
	this->width = width;
	this->height = height;
	this->scale = scale;

	framebuf = (uint8_t*)malloc(width*height);
	actual_framebuf = (uint8_t*)malloc(4*width*height);

	setFixedSize(scale*width, scale*height);

	setWindowTitle("test");
	show();

	needs_repaint = false;

	startTimer(5);

	channel0 = std::make_shared<QSoundEffect>();
	channel1 = std::make_shared<QSoundEffect>();
}

QtDisplay::~QtDisplay() {
	free(framebuf);
	free(actual_framebuf);
}

void QtDisplay::swap_buf() {
	framebuf_mutex.lock();
	convert_framebufs();
	framebuf_mutex.unlock();

	needs_repaint = true;
}

void QtDisplay::paintEvent(QPaintEvent* e) {
	Q_UNUSED(e);
	QPainter qp(this);

	framebuf_mutex.lock();
	QImage image(actual_framebuf, width, height, width*4, QImage::Format_RGB32);
	qp.drawPixmap(0, 0, width*scale, height*scale, QPixmap::fromImage(image));
	framebuf_mutex.unlock();

	needs_repaint = false;
}

void QtDisplay::timerEvent(QTimerEvent* e) {
	Q_UNUSED(e);

	handle_sound_updates();

	if (needs_repaint)
		this->repaint();
}

void QtDisplay::keyPressEvent(QKeyEvent* e) {
	if (!e->isAutoRepeat()) {
		int key = e->key();

		switch (key) {
			case Qt::Key_Left:
				player0_left = true;
				break;
			case Qt::Key_Right:
				player0_right = true;
				break;
			case Qt::Key_Up:
				player0_up = true;
				break;
			case Qt::Key_Down:
				player0_down = true;
				break;
			case Qt::Key_Space:
				player0_fire = true;
				break;
			default:
				break;
		}
	}

	QWidget::keyPressEvent(e);
}	

void QtDisplay::keyReleaseEvent(QKeyEvent* e) {
	if (!e->isAutoRepeat()) {
		int key = e->key();

		switch (key) {
			case Qt::Key_Left:
				player0_left = false;
				break;
			case Qt::Key_Right:
				player0_right = false;
				break;
			case Qt::Key_Up:
				player0_up = false;
				break;
			case Qt::Key_Down:
				player0_down = false;
				break;
			case Qt::Key_Space:
				player0_fire = false;
				break;
			default:
				break;
		}
	}

	QWidget::keyReleaseEvent(e);
}
