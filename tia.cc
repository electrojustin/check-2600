#include "tia.h"

#include <stdio.h>

#include "registers.h"

using std::placeholders::_1;
using std::placeholders::_2;

uint8_t reverse_byte(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

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
//	printf("scanline: %d\n", ntsc.gun_y);
//	dump_regs();
	if (ntsc.gun_y > 0 && ntsc.gun_y < NTSC::vsync_lines && !vsync_mode) {
		printf("Error! No vertical sync!\n");
		panic();
	} else if (ntsc.gun_y > NTSC::vsync_lines && vsync_mode) {
		printf("Error! Too long of vertical sync!\n");
		panic();
	}

	if (!vblank_mode) {
		int visible_x = ntsc.gun_x - NTSC::hblank;
		if (visible_x >= player0_x && visible_x < player0_x + player_size &&
		    ((player0_mask >> (visible_x - player0_x)) & 0x01)) {
			ntsc.write_pixel(player0_color);
		} else if (visible_x >= player1_x && visible_x < player1_x + player_size &&
		    ((player1_mask >> (visible_x - player1_x)) & 0x01)) {
			ntsc.write_pixel(player1_color);
		} else if (visible_x >= 0 && ((playfield_mask >> (visible_x / 4)) & 0x01)) {
			// TODO: Handle score mode
			ntsc.write_pixel(playfield_color);
		} else {
			ntsc.write_pixel(background_color);
		}
	} else {
		ntsc.write_pixel();
	}
	tia_cycle_num++;
}

void TIA::handle_playfield_mirror() {
	playfield_mask = playfield_mask & 0xFFFFF;
	if (!playfield_mirrored) {
		playfield_mask |= playfield_mask | playfield_mask << 20;
	} else {
		uint64_t pf0 = playfield_mask & 0x0F;
		uint64_t pf1 = (playfield_mask >> 4) & 0xFF;
		uint64_t pf2 = (playfield_mask >> 12) & 0xFF;
		pf0 = reverse_byte(pf0) >> 4;
		pf1 = reverse_byte(pf1);
		pf2 = reverse_byte(pf2);
		playfield_mask |= playfield_mask;
		playfield_mask |= pf2 << 20;
		playfield_mask |= pf1 << 28;
		playfield_mask |= pf0 << 36;
	}
}

void TIA::reset_sprite_position(int& sprite, int hblank_fudge) {
	sprite = ((cycle_num % cpu_scanline_cycles) * tia_cycle_ratio) - NTSC::hblank;
	if (sprite < 0)
		sprite = hblank_fudge;
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
	cycle_num += cpu_scanline_cycles - (cycle_num % cpu_scanline_cycles);
}

void TIA::colup0(uint8_t val) {
	player0_color = val;
}

void TIA::colup1(uint8_t val) {
	player1_color = val;
}

void TIA::colupf(uint8_t val) {
	playfield_color = val;
}

void TIA::colubk(uint8_t val) {
	background_color = val;
}

void TIA::ctrlpf(uint8_t val) {
	playfield_mirrored = val & 0x01;

	handle_playfield_mirror();
}

void TIA::pf0(uint8_t val) {
	playfield_mask &= ~0x0F;
	playfield_mask |= val >> 4;
	handle_playfield_mirror();
}

void TIA::pf1(uint8_t val) {
	playfield_mask &= ~0xFF0;
	playfield_mask |= ((uint64_t)val) << 4;
	handle_playfield_mirror();
}

void TIA::pf2(uint8_t val) {
	playfield_mask &= ~0xFF000;
	playfield_mask |= ((uint64_t)val) << 12;
	handle_playfield_mirror();
}

void TIA::resp0(uint8_t val) {
	reset_sprite_position(player0_x, 3);
}

void TIA::resp1(uint8_t val) {
	reset_sprite_position(player1_x, 3);
}

void TIA::resm0(uint8_t val) {
	reset_sprite_position(missile0_x, 2);
}

void TIA::resm1(uint8_t val) {
	reset_sprite_position(missile1_x, 2);
}

void TIA::resbl(uint8_t val) {
	reset_sprite_position(ball_x, 2);
}

void TIA::grp0(uint8_t val) {
	player0_mask = val;
}

void TIA::grp1(uint8_t val) {
	player1_mask = val;
}


TIA::TIA(uint16_t start, uint16_t end) {
	dma_region = std::make_shared<DmaRegion>(start, end, std::bind(&TIA::dma_read_hook, this, _1), std::bind(&TIA::dma_write_hook, this, _1, _2));
	tia_cycle_num = tia_cycle_ratio * cycle_num;

	dma_write_table[0x00] = std::bind(&TIA::vsync, this, _1);
	dma_write_table[0x01] = std::bind(&TIA::vblank, this, _1);
	dma_write_table[0x02] = std::bind(&TIA::wsync, this, _1);
	dma_write_table[0x06] = std::bind(&TIA::colup0, this, _1);
	dma_write_table[0x07] = std::bind(&TIA::colup1, this, _1);
	dma_write_table[0x08] = std::bind(&TIA::colupf, this, _1);
	dma_write_table[0x09] = std::bind(&TIA::colubk, this, _1);
	dma_write_table[0x0A] = std::bind(&TIA::ctrlpf, this, _1);
	dma_write_table[0x0D] = std::bind(&TIA::pf0, this, _1);
	dma_write_table[0x0E] = std::bind(&TIA::pf1, this, _1);
	dma_write_table[0x0F] = std::bind(&TIA::pf2, this, _1);
	dma_write_table[0x10] = std::bind(&TIA::resp0, this, _1);
	dma_write_table[0x11] = std::bind(&TIA::resp1, this, _1);
	dma_write_table[0x12] = std::bind(&TIA::resm0, this, _1);
	dma_write_table[0x13] = std::bind(&TIA::resm1, this, _1);
	dma_write_table[0x14] = std::bind(&TIA::resbl, this, _1);
	dma_write_table[0x1B] = std::bind(&TIA::grp0, this, _1);
	dma_write_table[0x1C] = std::bind(&TIA::grp1, this, _1);
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