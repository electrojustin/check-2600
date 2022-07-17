#include "ntsc.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>

NTSC::NTSC() {
	display = create_display(visible_columns, visible_scanlines);
	memset(display->framebuf, 0, visible_columns*visible_scanlines);
	vsync();

	last_buf_swap = std::chrono::high_resolution_clock::now();
}

void NTSC::vsync() {
	gun_x = 0;
	gun_y = 0;
}

void NTSC::write_pixel(uint8_t pixel) {
	int x = gun_x - hblank;
	int y = gun_y - (vsync_lines + vblank);
	if (x >= 0 && x < visible_columns &&
	    y >= 0 && y < visible_scanlines)
		display->framebuf[y*visible_columns + x] = pixel;

	gun_x++;
	if (gun_x >= columns) {
		gun_x = 0;
		gun_y++;
	}
	if (gun_y >= scanlines) {
		gun_y = 0;

		display->swap_buf();

		auto curr_time = std::chrono::high_resolution_clock::now();
		auto time_in_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(curr_time - last_buf_swap);
		last_buf_swap = curr_time;
		if (time_in_microseconds.count() < refresh_period_us)
			usleep(refresh_period_us - time_in_microseconds.count());
	}
}
