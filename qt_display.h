#include "display.h"

#include <QPainter>
#include <QSoundEffect>
#include <QWidget>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#ifndef QT_DISPLAY_H
#define QT_DISPLAY_H

class QtDisplay : public Display, public QWidget {
  int width;
  int height;

  // The original Atari was designed for 160x192. This is positively tiny on a
  // modern monitor, so we upscale.
  int scale;

  // This is the framebuffer in actual BGRA format, which QT5 can interpret as a
  // bitmap and display to the screen. Note that we need mutexes here because
  // QT5 runs in a separate thread from the main emulator.
  std::mutex framebuf_mutex;
  uint8_t *actual_framebuf;

  // This signals to the QT5 thread that we need a repaint. This is just poll'd
  // by the QT5 thread every few milliseconds and is set by the emulation
  // thread.
  std::atomic<bool> needs_repaint;

  // Sound effects for each channel
  std::shared_ptr<QSoundEffect> channel0 = nullptr;
  std::shared_ptr<QSoundEffect> channel1 = nullptr;
  int prev_volume0 = 0;
  int prev_volume1 = 0;

  // This is a convenient hash value that combines frequency and waveform
  // values. A change in these values means we need to update the sound
  // channels.
  int channel0_index = 0;
  int channel1_index = 0;

  // Convert from Atari NTSC to proper BGRA.
  void convert_framebufs();

  // Update all the sound effects.
  void handle_sound_channel_update(std::shared_ptr<QSoundEffect> &channel,
                                   int &channel_index, int &prev_volume,
                                   int volume, int freq, int noise_control);
  void handle_sound_updates();

protected:
  void paintEvent(QPaintEvent *e) override;
  void timerEvent(QTimerEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;
  void keyReleaseEvent(QKeyEvent *e) override;

public:
  QtDisplay(int width, int height, int scale = 4);
  ~QtDisplay();

  void swap_buf() override;
};

#endif
