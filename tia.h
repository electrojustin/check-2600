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

	uint64_t playfield_mask = 0;
	uint8_t playfield_color = 0;
	bool playfield_mirrored = false;
	bool playfield_score_mode = false;
	bool playfield_priority = false;

	int player0_x = 0;
	int player0_motion = 0;
	int player1_x = 0;
	int player1_motion = 0;
	uint8_t player0_mask = 0;
	uint8_t player1_mask = 0;
	uint8_t player0_color = 0;
	uint8_t player1_color = 0;
	int player0_scale = 1;
	int player1_scale = 1;
	int player0_duplicate_mask = 0;
	int player1_duplicate_mask = 0;

	int missile0_x = 0;
	int missile0_motion = 0;
	int missile1_x = 0;
	int missile1_motion = 0;
	int missile0_size = 1;
	int missile1_size = 1;
	bool missile0_enable = false;
	bool missile1_enable = false;

	int ball_x = 0;
	int ball_motion = 0;
	int ball_size = 1;
	bool ball_enable = false;

	uint8_t dma_val = 0;
	std::function<void(uint8_t)> dma_write_request = nullptr;

	std::function<uint8_t(void)> dma_read_table[128] = { nullptr };
	std::function<void(uint8_t)> dma_write_table[128] = { nullptr };

	uint8_t dma_read_hook(uint16_t addr);
	void dma_write_hook(uint16_t addr, uint8_t val);
	void process_tia_cycle();

	void handle_playfield_mirror();

	bool should_draw_playfield(int visible_x);
	void draw_playfield(int visible_x);

	bool should_draw_ball(int visible_x);
	void draw_ball();

	bool should_draw_player(int visible_x, int player_x, uint8_t player_mask, int duplicate_mask, int scale);
	void draw_player(uint8_t player_color);

	bool should_draw_missile(int visible_x, int missile_x, int missile_size, bool missile_enabled);
	void draw_missile(uint8_t missile_color);

	// If a sprite position is reset during the horizontal blanking period, the sprite will appear at the far left side of the screen, plus a few pixels. hblank_fuzz is that "few pixel fudge factor".
	void reset_sprite_position(int& sprite, int hblank_fudge);

	void handle_nusiz(uint8_t val, int& dup_mask, int& scale, int& missile_size); 
	void handle_resmp(int player_scale, int player_x, int& missile_x);

	void vsync(uint8_t val);
	void vblank(uint8_t val);
	void wsync(uint8_t val);
	void nusiz0(uint8_t val);
	void nusiz1(uint8_t val);
	void colup0(uint8_t val);
	void colup1(uint8_t val);
	void colupf(uint8_t val);
	void colubk(uint8_t val);
	void ctrlpf(uint8_t val);
	void pf0(uint8_t val);
	void pf1(uint8_t val);
	void pf2(uint8_t val);
	void resp0(uint8_t val);
	void resp1(uint8_t val);
	void resm0(uint8_t val);
	void resm1(uint8_t val);
	void resbl(uint8_t val);
	void grp0(uint8_t val);
	void grp1(uint8_t val);
	void enam0(uint8_t val);
	void enam1(uint8_t val);
	void enabl(uint8_t val);
	void hmp0(uint8_t val);
	void hmp1(uint8_t val);
	void hmm0(uint8_t val);
	void hmm1(uint8_t val);
	void hmbl(uint8_t val);
	void resmp0(uint8_t val);
	void resmp1(uint8_t val);
	void hmove(uint8_t val);
	void hmclr(uint8_t val);
public:
	// Ratio of TIA clock to CPU clock
	const static int tia_cycle_ratio = 3;
	// Number of CPU clocks per scanline
	const static int cpu_scanline_cycles = NTSC::columns/tia_cycle_ratio;
	// Player sprite size
	const static int player_size = 8;

	TIA(uint16_t start, uint16_t end);

	std::shared_ptr<DmaRegion> get_dma_region() {
		return dma_region;
	}

	void process_tia();
};

#endif
