#include <memory>
#include <stdint.h>
#include <functional>

#include "ntsc.h"
#include "memory.h"

#ifndef TIA_H
#define TIA_H

class TIA {
	NTSC ntsc;
	std::shared_ptr<DmaRegion> dma_region;
	uint64_t tia_cycle_num;
	bool vsync_mode = false;
	bool vblank_mode = false;

	uint8_t background_color = 0;

	uint8_t dma_val = 0;
	std::function<void(uint8_t)> dma_write_request = nullptr;

	std::function<uint8_t(void)> dma_read_table[128] = { nullptr };
	std::function<void(uint8_t)> dma_write_table[128] = { nullptr };

	uint8_t dma_read_hook(uint16_t addr);
	void dma_write_hook(uint16_t addr, uint8_t val);
	void process_tia_cycle();

	void vsync(uint8_t val);
	void vblank(uint8_t val);
	void wsync(uint8_t val);
	void colubk(uint8_t val);
public:
	// Ratio of TIA clock to CPU clock
	const static int tia_cycle_ratio = 3;
	// Number of CPU clocks per scanline
	const static int cpu_scanline_cycles = NTSC::columns/tia_cycle_ratio;

	TIA(uint16_t start, uint16_t end);

	std::shared_ptr<DmaRegion> get_dma_region() {
		return dma_region;
	}

	void process_tia();
};

#endif
