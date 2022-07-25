#include <stdint.h>
#include <memory>

#ifndef DISPLAY_H
#define DISPLAY_H

// Note that a display should also handle input and sound
class Display {
public:
	virtual ~Display() = default;

	uint8_t* framebuf;

	virtual void swap_buf() = 0;
};

std::unique_ptr<Display> create_display(int width, int height);

#endif
