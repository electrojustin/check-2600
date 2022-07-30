#include <stdint.h>

#include "memory.h"

#ifndef PIA_H
#define PIA_H

class PIA {
  std::shared_ptr<MappedRegion> memory_region;

  uint8_t memory_read_hook(uint16_t addr);
  void memory_write_hook(uint16_t addr, uint8_t val);

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

  std::shared_ptr<MappedRegion> get_memory_region() { return memory_region; }

  // Process outstanding PIA cycles
  void process_pia();

  // Dump PIA state to STDOUT
  void dump_pia();
};

#endif
