#include "display.h"

#include <memory>
#include <chrono>

#ifndef NTSC_H
#define NTSC_H

class NTSC {
	std::unique_ptr<Display> display;

	std::chrono::time_point<std::chrono::high_resolution_clock> last_buf_swap;

public:
	const static int columns = 228;
	const static int scanlines = 262;
	const static int vsync_lines = 3;
	const static int vblank = 37;
	const static int overscan = 30;
	const static int hblank = 68;

	const static int visible_columns = 160;
	const static int visible_scanlines = 192;

	// Period in us for 30Hz refresh
	const static int refresh_period_us = 33333;

	// Electron gun position
	int gun_x;
	int gun_y;

	NTSC();

	// Resets gun position
	void vsync();

	// Fires electron gun
	void write_pixel(uint8_t pixel=0);
};

#endif
