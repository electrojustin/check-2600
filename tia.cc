#include "tia.h"

#include <stdio.h>

#include "registers.h"

using std::placeholders::_1;
using std::placeholders::_2;

uint8_t TIA::dma_read_hook(uint16_t addr) {
	auto read_func = dma_read_table[addr];
	if (!read_func) {
		printf("Error! Invalid DMA read at %x\n", addr);
		panic();
		return 0;
	}

	return read_func();
}

void TIA::dma_write_hook(uint16_t addr, uint8_t val) {
	auto write_func = dma_write_table[addr];
	if (!write_func) {
		printf("Error! Invalid DMA write at %x\n", addr);
		panic();
	}

	dma_write_request = write_func;
	dma_val = val;
}

void TIA::process_tia_cycle() {
	if (ntsc.gun_y > 0 && ntsc.gun_y < NTSC::vsync_lines && !vsync_mode) {
		printf("Error! No vertical sync!\n");
		panic();
	} else if (ntsc.gun_y > NTSC::vsync_lines && vsync_mode) {
		printf("Error! Too long of vertical sync!\n");
		panic();
	}

	if (!vblank_mode) {
		ntsc.write_pixel(background_color);
	} else {
		ntsc.write_pixel();
	}
	tia_cycle_num++;
}

void TIA::vsync(uint8_t val) {
	if (val && val != 2) {
		printf("Error! Invalid VSYNC value %x\n", val);
		panic();
	} else {
		vsync_mode = val == 2;
	}
}

void TIA::vblank(uint8_t val) {
	//TODO: Add input control support
	if (val & 0x3C) {
		printf("Error! Invalid VBLANK value %x\n", val);
		panic();
	} else {
		vblank_mode = val == 2;
	}
}

// Sleep the CPU until hblank is (almost) over
void TIA::wsync(uint8_t val) {
	const int cpu_cycle_start = NTSC::hblank / tia_cycle_ratio;
	cycle_num += cpu_cycle_start + (cpu_scanline_cycles - (cycle_num % cpu_scanline_cycles));
}

void TIA::colubk(uint8_t val) {
	background_color = val;
}

TIA::TIA(uint16_t start, uint16_t end) {
	dma_region = std::make_shared<DmaRegion>(start, end, std::bind(&TIA::dma_read_hook, this, _1), std::bind(&TIA::dma_write_hook, this, _1, _2));
	tia_cycle_num = tia_cycle_ratio * cycle_num;

	dma_write_table[0x00] = std::bind(&TIA::vsync, this, _1);
	dma_write_table[0x01] = std::bind(&TIA::vblank, this, _1);
	dma_write_table[0x02] = std::bind(&TIA::wsync, this, _1);
	dma_write_table[0x09] = std::bind(&TIA::colubk, this, _1);
}

void TIA::process_tia() {
	if (dma_write_request) {
		dma_write_request(dma_val);
		dma_write_request = nullptr;
		dma_val = 0;
	}

	while(tia_cycle_num != tia_cycle_ratio * cycle_num)
		process_tia_cycle();
}
