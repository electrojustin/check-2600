#include "pia.h"

#include <functional>
#include <memory>
#include <stdio.h>

#include "atari.h"
#include "input.h"
#include "memory.h"
#include "registers.h"

using std::placeholders::_1;
using std::placeholders::_2;

uint8_t PIA::memory_read_hook(uint16_t addr) {
  // Process clock ticks before reading timer values for better accuracy
  process_pia();

  uint8_t ret;
  switch (addr) {
  // SWCHA
  // Bit 0 is player 0 up
  // Bit 1 is player 0 down
  // Bit 2 is player 0 left
  // Bit 3 is player 0 right
  // Bit 4 is player 1 up
  // Bit 5 is player 1 down
  // Bit 6 is player 1 left
  // Bit 7 is player 1 right
  case 0x0280:
    return ~(((uint8_t)player0_up << 4) | ((uint8_t)player0_down << 5) |
             ((uint8_t)player0_left << 6) | ((uint8_t)player0_right << 7) |
             (uint8_t)player1_up | ((uint8_t)player1_down << 1) |
             ((uint8_t)player1_left << 2) | ((uint8_t)player1_right << 3));
  // SWACNT not implemented
  case 0x0281:
    return 0;
  // SWCHB not implemented
  case 0x0282:
    return 0x3F;
  // SWBCNT not implemented
  case 0x0283:
    return 0;
  // INTIM
  // Timer value
  case 0x0284:
    return timer;
  // INSTAT
  // Bit 7 is set if timer underflowed since it was last written to
  // Bit 6 is set if timer underflowed since it was last written to OR read from
  case 0x0285:
    ret = ((uint8_t)underflow_since_read << 6) |
          ((uint8_t)underflow_since_write << 7);
    underflow_since_read = false;
    return ret;
  default:
    printf("Warning! Invalid PIA read at %x\n", addr);
    return 0;
  }
}

void PIA::memory_write_hook(uint16_t addr, uint8_t val) {
  switch (addr) {
  // SWCHA output
  case 0x0280:
  // SWACNT
  case 0x0281:
    // TODO: Add proper I/O control.
    break;
  // TIM1T
  // Set timer with interval of 1 CPU clock
  case 0x0294:
    interval = 1;
    break;
  // TIM8T
  // Set timer with interval of 8 CPU clocks
  case 0x0295:
    interval = 8;
    break;
  // TIM64T
  // Set timer with interval of 64 CPU clocks
  case 0x0296:
    interval = 64;
    break;
  // T1024T
  // Set timer with interval 1024 CPU clocks
  case 0x0297:
    interval = 1024;
    break;
  default:
    printf("Error! Invalid PIA write at %x\n", addr);
    panic();
    return;
  }

  timer = val;
  timer_needs_started = true;
}

void PIA::process_clock_tick() {
  cycle_counter++;

  if (cycle_counter == interval) {
    if (!timer) {
      underflow_since_read = true;
      underflow_since_write = true;
    }

    cycle_counter = 0;

    timer--;
  }
}

PIA::PIA() {
  memory_region = std::make_shared<MappedRegion>(
      PIA_START, PIA_END, std::bind(&PIA::memory_read_hook, this, _1),
      std::bind(&PIA::memory_write_hook, this, _1, _2));
}

void PIA::process_pia() {
  if (timer_needs_started) {
    timer_needs_started = false;
    cycle_counter = interval - 1;
    underflow_since_read = false;
    underflow_since_write = false;
  } else {
    uint64_t num_cycles_to_process = cycle_num - last_process_cycle_num;

    for (uint64_t i = 0; i < num_cycles_to_process; i++)
      process_clock_tick();
  }

  last_process_cycle_num = cycle_num;
}

void PIA::dump_pia() {
  printf("Timer: %d\n", timer);
  printf("Interval: %d\n", interval);
  printf("Next tick: %d cycles\n", interval - cycle_counter);
}
