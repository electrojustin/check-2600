#include "ntsc.h"

#include <string.h>

NTSC::NTSC() {
	display = create_display(visible_columns, visible_scanlines);
	memset(display->framebuf, 0, visible_columns*visible_scanlines);
	vsync();
}

void NTSC::vsync() {
	gun_x = 0;
	gun_y = 0;
}

void NTSC::write_pixel(uint8_t pixel) {
	if (pixel) {
		int x = gun_x - hblank;
		int y = gun_y - (vsync_lines + vblank);
		if (x >= 0 && x < visible_columns &&
		    y >= 0 && y < visible_scanlines)
			display->framebuf[y*visible_columns + x] = pixel;
	}
	gun_x++;
	if (gun_x >= columns) {
		gun_x = 0;
		gun_y++;
	}
	if (gun_y >= scanlines) {
		gun_y = 0;
		display->swap_buf();
	}
}
