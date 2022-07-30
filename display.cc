#include "display.h"

#include "qt_display.h"

std::unique_ptr<Display> create_display(int width, int height, int scale) {
  return std::make_unique<QtDisplay>(width, height, scale);
}
