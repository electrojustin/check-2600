#include "ntsc.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

NTSC::NTSC() {
  display = create_display(visible_columns, visible_scanlines);
  memset(display->framebuf, 0, visible_columns * visible_scanlines);

  gun_x = 0;
  gun_y = 0;

  last_buf_swap = std::chrono::high_resolution_clock::now();
}

void NTSC::vsync() {
  gun_y = 0;

  display->swap_buf();

  auto curr_time = std::chrono::high_resolution_clock::now();
  auto time_in_microseconds =
      std::chrono::duration_cast<std::chrono::microseconds>(curr_time -
                                                            last_buf_swap);
  if (time_in_microseconds.count() < refresh_period_us) {
    usleep(refresh_period_us - time_in_microseconds.count());
  } else {
    printf("Warning! Frame lag! %lu us\n", time_in_microseconds.count());
  }
  last_buf_swap = std::chrono::high_resolution_clock::now();
}

void NTSC::write_pixel(uint8_t pixel) {
  int x = gun_x - hblank;
  int y = gun_y - vblank;
  if (x >= 0 && x < visible_columns && y >= 0 && y < visible_scanlines)
    display->framebuf[y * visible_columns + x] = pixel;

  gun_x++;
  if (gun_x >= columns) {
    gun_x = 0;
    gun_y++;
  }
}

void NTSC::debug_swap_buf() {
  int x = gun_x - hblank;
  int y = gun_y - vblank;

  if (y < 0 || y > visible_scanlines)
    return;
  if (x < 0 || x >= visible_columns)
    x = 0;

  for (int i = y * visible_columns + x; i < visible_columns * visible_scanlines;
       i++)
    display->framebuf[i] = 0x00;

  display->swap_buf();
}
