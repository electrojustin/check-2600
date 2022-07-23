#include <memory>
#include <stdint.h>
#include <functional>

#include "ntsc.h"
#include "memory.h"

#ifndef TIA_H
#define TIA_H

class TIA {
	std::shared_ptr<DmaRegion> dma_region;
	int64_t tia_cycle_num;
	uint64_t last_process_cycle_num;
	bool vsync_mode = false;
	bool vblank_mode = false;
	bool rsync_mode = false;

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
	uint8_t player0_mask_buf = 0;
	bool player0_mask_delay = false;
	uint8_t player1_mask = 0;
	uint8_t player1_mask_buf = 0;
	bool player1_mask_delay = false;
	uint8_t player0_color = 0;
	uint8_t player1_color = 0;
	int player0_scale = 1;
	int player1_scale = 1;
	int player0_duplicate_mask = 0;
	int player1_duplicate_mask = 0;
	bool player0_reflect = false;
	bool player1_reflect = false;

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
	bool ball_enable_buf = false;
	bool ball_enable_delay = false;

	// Collisions
	bool missile0_player1 = false;
	bool missile0_player0 = false;
	bool missile1_player0 = false;
	bool missile1_player1 = false;
	bool player0_playfield = false;
	bool player0_ball = false;
	bool player1_playfield = false;
	bool player1_ball = false;
	bool missile0_playfield = false;
	bool missile0_ball = false;
	bool missile1_playfield = false;
	bool missile1_ball = false;
	bool ball_playfield = false;
	bool player0_player1 = false;
	bool missile0_missile1 = false;

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

	bool should_draw_missile(int visible_x, int missile_x, int missile_size, bool missile_enabled, int duplicate_mask);
	void draw_missile(uint8_t missile_color);

	// If a sprite position is reset during the horizontal blanking period, the sprite will appear at the far left side of the screen, plus a few pixels. hblank_fuzz is that "few pixel fudge factor".
	void reset_sprite_position(int& sprite, int hblank_fudge, int fudge);

	void handle_nusiz(uint8_t val, int& dup_mask, int& scale, int& missile_size); 
	void handle_resmp(int player_scale, int player_x, int& missile_x);

	// Graphics registers
	void vsync(uint8_t val);
	void vblank(uint8_t val);
	void rsync(uint8_t val);
	void wsync(uint8_t val);
	void nusiz0(uint8_t val);
	void nusiz1(uint8_t val);
	void colup0(uint8_t val);
	void colup1(uint8_t val);
	void colupf(uint8_t val);
	void colubk(uint8_t val);
	void ctrlpf(uint8_t val);
	void refp0(uint8_t val);
	void refp1(uint8_t val);
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
	void vdelp0(uint8_t val);
	void vdelp1(uint8_t val);
	void vdelbl(uint8_t val);
	void resmp0(uint8_t val);
	void resmp1(uint8_t val);
	void hmove(uint8_t val);
	void hmclr(uint8_t val);
	void cxclr(uint8_t val);

	// Collision registers
	uint8_t cxm0p();
	uint8_t cxm1p();
	uint8_t cxp0fb();
	uint8_t cxp1fb();
	uint8_t cxm0fb();
	uint8_t cxm1fb();
	uint8_t cxblpf();
	uint8_t cxppmm();

	// Input regsiters
	uint8_t inpt0();
	uint8_t inpt1();
	uint8_t inpt2();
	uint8_t inpt3();
	uint8_t inpt4();
	uint8_t inpt5();

	// Audio registers
	void audv0(uint8_t val);
	void audv1(uint8_t val);
	void audf0(uint8_t val);
	void audf1(uint8_t val);
	void audc0(uint8_t val);
	void audc1(uint8_t val);
public:
	// Ratio of TIA clock to CPU clock
	const static int tia_cycle_ratio = 3;
	// Number of CPU clocks per scanline
	const static int cpu_scanline_cycles = NTSC::columns/tia_cycle_ratio;
	// Player sprite size
	const static int player_size = 8;
	// RESXX offsets
	const static int resp_player_offset = 5;
	const static int resp_missile_ball_offset = 4;

	NTSC ntsc;

	TIA();

	std::shared_ptr<DmaRegion> get_dma_region() {
		return dma_region;
	}

	void process_tia();
};

#endif
