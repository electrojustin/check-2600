#include "pia.h"

#include <stdio.h>
#include <memory>
#include <functional>

#include "atari.h"
#include "memory.h"
#include "registers.h"
#include "input.h"

using std::placeholders::_1;
using std::placeholders::_2;

uint8_t PIA::dma_read_hook(uint16_t addr) {
	// Process clock ticks before reading timer values for better accuracy
	process_pia();

	uint8_t ret;
	switch (addr) {
		case 0x0280:
			return ~(((uint8_t)player0_up << 4) |
				 ((uint8_t)player0_down << 5) |
				 ((uint8_t)player0_left << 6) |
				 ((uint8_t)player0_right << 7) |
				 (uint8_t)player1_up |
				 ((uint8_t)player1_down << 1) |
				 ((uint8_t)player1_left << 2) |
				 ((uint8_t)player1_right << 3));
		case 0x0281:
			return 0;
		case 0x0282:
			return 0xFF;
		case 0x0283:
			return 0;
		case 0x0284:
			return timer;
		case 0x0285:
			ret = ((uint8_t)underflow_since_read << 6) | ((uint8_t)underflow_since_write << 7);
			underflow_since_read = false;
			return ret;
		default:
			printf("Warning! Invalid DMA read at %x\n", addr);
			return 0;
	}
}

void PIA::dma_write_hook(uint16_t addr, uint8_t val) {
	switch(addr) {
		case 0x0280:
		case 0x0281:
			// TODO: Add proper I/O control.
			break;
		case 0x0294:
			interval = 1;
			break;
		case 0x0295:
			interval = 8;
			break;
		case 0x0296:
			interval = 64;
			break;
		case 0x0297:
			interval = 1024;
			break;
		default:
			printf("Error! Invalid DMA write at %x\n", addr);
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
	dma_region = std::make_shared<DmaRegion>(PIA_START, PIA_END, std::bind(&PIA::dma_read_hook, this, _1), std::bind(&PIA::dma_write_hook, this, _1, _2));
}

void PIA::process_pia() {
	if (timer_needs_started) {
		timer_needs_started = false;
		cycle_counter = interval-1;
		underflow_since_read = false;
		underflow_since_write = false;
	} else {
		uint64_t num_cycles_to_process = cycle_num - last_process_cycle_num;

		for (uint64_t i=0; i < num_cycles_to_process; i++)
			process_clock_tick();
	}

	last_process_cycle_num = cycle_num;
}
