#include <stdint.h>

#ifndef DISPLAY_H
#define DISPLAY_H

class Display {
public:
	uint8_t* framebuf;

	virtual void swap_buf() = 0;
};

#endif
