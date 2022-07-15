#include <stdint.h>
#include <memory>

#ifndef DISPLAY_H
#define DISPLAY_H

class Display {
public:
	uint8_t* framebuf;

	virtual void swap_buf() = 0;
};

std::unique_ptr<Display> create_display(int width, int height);

#endif
