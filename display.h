#include <memory>
#include <stdint.h>

#ifndef DISPLAY_H
#define DISPLAY_H

// Abstract display class for handling actual visual output. Note that Displays
// will likely handle input and sound too, since GUIs tend to come with event
// loop frameworks.
class Display {
public:
  virtual ~Display() = default;

  // The framebuf is 8-bit color in the Atari NTSC color palette.
  // TODO: Support PAL and SECAM color palettes.
  uint8_t *framebuf;

  // Display the information in the current framebuffer.
  virtual void swap_buf() = 0;
};

std::unique_ptr<Display> create_display(int width, int height, int scale);

#endif
