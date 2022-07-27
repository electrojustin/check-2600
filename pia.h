#include <stdint.h>

#include "memory.h"

#ifndef PIA_H
#define PIA_H

class PIA {
  std::shared_ptr<DmaRegion> dma_region;

  uint8_t dma_read_hook(uint16_t addr);
  void dma_write_hook(uint16_t addr, uint8_t val);

  bool timer_needs_started = false;
  uint64_t last_process_cycle_num = 0;
  int interval = 1024;

  bool underflow_since_read = false;
  bool underflow_since_write = false;

  void process_clock_tick();

public:
  uint8_t timer = 0;
  int cycle_counter = 0;

  PIA();

  std::shared_ptr<DmaRegion> get_dma_region() { return dma_region; }

  void process_pia();

  void dump_pia();
};

#endif
