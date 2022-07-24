#include "tia.h"

#include <stdio.h>

#include "atari.h"
#include "registers.h"
#include "input.h"

using std::placeholders::_1;
using std::placeholders::_2;

uint8_t reverse_byte(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

int mod(int a, int b) {
	int ret = a % b;
	return ret < 0 ? ret + b : ret;
}

uint8_t TIA::dma_read_hook(uint16_t addr) {
	auto read_func = dma_read_table[addr];
	if (!read_func) {
		printf("Warning! Invalid DMA read at %x. PC: %x\n", addr, program_counter);
		return 0;
	}

	return read_func();
}

void TIA::dma_write_hook(uint16_t addr, uint8_t val) {
	auto write_func = dma_write_table[addr];
	if (!write_func) {
		printf("Warning! Invalid DMA write at %x. PC: %x\n", addr, program_counter);
		return;
	}

	dma_write_request = write_func;
	dma_val = val;
}

bool TIA::should_draw_playfield(int visible_x) {
	return visible_x >= 0 && ((playfield_mask >> (visible_x / 4)) & 0x01);
}

void TIA::draw_playfield(int visible_x) {
	if (!playfield_score_mode) {
		ntsc.write_pixel(playfield_color);
	} else {
		if (visible_x < NTSC::visible_columns/2) {
			ntsc.write_pixel(player0_color);
		} else {
			ntsc.write_pixel(player1_color);
		}
	}
}

bool TIA::should_draw_ball(int visible_x) {
	return ball_enable &&
	       visible_x >= ball_x &&
	       (visible_x - ball_x) < ball_size;
}

void TIA::draw_ball() {
	ntsc.write_pixel(playfield_color);
}

bool TIA::should_draw_player(int visible_x, int player_x, uint8_t player_mask, int duplicate_mask, int scale) {
	if (!duplicate_mask) {
		return visible_x >= player_x &&
		       visible_x < player_x + player_size*scale &&
		       ((player_mask >> ((visible_x - player_x)/scale)) & 0x01);
	} else {
		return visible_x >= player_x &&
		       ((duplicate_mask >> ((visible_x - player_x)/player_size)) & 0x01) &&
		       ((player_mask >> ((visible_x - player_x)%player_size)) & 0x01);
	}
}

void TIA::draw_player(uint8_t player_color) {
	ntsc.write_pixel(player_color);
}

bool TIA::should_draw_missile(int visible_x, int missile_x, int missile_size, bool missile_enabled, int duplicate_mask) {
	if (!missile_enabled)
		return false;

	if (!duplicate_mask) {
		return visible_x >= missile_x &&
		       (visible_x - missile_x) < missile_size;
	} else {
		return visible_x >= missile_x &&
		       ((duplicate_mask >> (visible_x - missile_x)/player_size) & 0x01) &&
		       ((visible_x - missile_x) % player_size) < missile_size;
	}
}

void TIA::draw_missile(uint8_t missile_color) {
	ntsc.write_pixel(missile_color);
}

void TIA::process_tia_cycle() {
	if (!vblank_mode) {
		int visible_x = ntsc.gun_x - NTSC::hblank;


		// What objects occupy the current pixel.
		bool player0 = should_draw_player(visible_x, player0_x, player0_mask, player0_duplicate_mask, player0_scale);
		bool player1 = should_draw_player(visible_x, player1_x, player1_mask, player1_duplicate_mask, player1_scale);
		bool missile0 = should_draw_missile(visible_x, missile0_x, missile0_size, missile0_enable, player0_duplicate_mask);
		bool missile1 = should_draw_missile(visible_x, missile1_x, missile1_size, missile1_enable, player1_duplicate_mask);
		bool ball = should_draw_ball(visible_x);
		bool playfield = should_draw_playfield(visible_x);

		// Update collision registers
		missile0_player1 |= missile0 && player1;
		missile0_player0 |= missile0 && player0;
		missile1_player0 |= missile1 && player0;
		missile1_player1 |= missile1 && player1;
		player0_playfield |= player0 && playfield;
		player0_ball |= player0 && ball;
		player1_playfield |= player1 && playfield;
		player1_ball |= player1 && ball;
		missile0_playfield |= missile0 && playfield;
		missile0_ball |= missile0 && ball;
		missile1_playfield |= missile1 && playfield;
		missile1_ball |= missile1 && ball;
		ball_playfield |= ball && playfield;
		player0_player1 |= player0 && player1;
		missile0_missile1 |= missile0 && missile1;

		if (playfield_priority && playfield) {
			draw_playfield(visible_x);
		} else if (playfield_priority && ball) {
			draw_ball();
		} else if (player0) {
			draw_missile(player0_color);
		} else if (missile0) {
			draw_player(player0_color);
		} else if (player1) {
			draw_player(player1_color);
		} else if (missile1) {
			draw_missile(player1_color);
		} else if (!playfield_priority && playfield) {
			draw_playfield(visible_x);
		} else if (!playfield_priority && ball) {
			draw_ball();
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

void TIA::reset_sprite_position(int& sprite, int hblank_fudge, int fudge) {
	sprite = (tia_cycle_num % NTSC::columns) - NTSC::hblank; 
	if (sprite < 0) {
		sprite = hblank_fudge;
	} else {
		sprite += fudge;
	}
}

void TIA::vsync(uint8_t val) {
	bool new_vsync_mode = val & 0x02;
	if (vsync_mode && !new_vsync_mode)
		ntsc.vsync();
	vsync_mode = new_vsync_mode;
}

void TIA::vblank(uint8_t val) {
	//TODO: Add input control support
	vblank_mode = val == 2;
}

void TIA::rsync(uint8_t val) {
	tia_cycle_num = -3;
	ntsc.gun_x = -3;
}

// Sleep the CPU until hblank is (almost) over
void TIA::wsync(uint8_t val) {
	cycle_num += (NTSC::columns - (tia_cycle_num % NTSC::columns)) / tia_cycle_ratio;
}

void TIA::handle_nusiz(uint8_t val, int& dup_mask, int& scale, int& missile_size) {
	missile_size = 1 << ((val >> 4) & 0x03);

	int player_settings = val & 0x07;

	switch (player_settings) {
		case 0:
			dup_mask = 0;
			scale = 1;
			break;
		case 1:
			dup_mask = 0b101;
			scale = 1;
			break;
		case 2:
			dup_mask = 0b10001;
			scale = 1;
			break;
		case 3:
			dup_mask = 0b10101;
			scale = 1;
			break;
		case 4:
			dup_mask = 0b100000001;
			scale = 1;
			break;
		case 5:
			dup_mask = 0;
			scale = 2;
			break;
		case 6:
			dup_mask = 0b100010001;
			scale = 1;
			break;
		case 7:
			dup_mask = 0;
			scale = 4;
			break;
		default:
			printf("Invalid player setting\n");
			panic();
			break;
	}
}

void TIA::nusiz0(uint8_t val) {
	handle_nusiz(val, player0_duplicate_mask, player0_scale, missile0_size);
}

void TIA::nusiz1(uint8_t val) {
	handle_nusiz(val, player1_duplicate_mask, player1_scale, missile1_size);
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
	playfield_score_mode = val & 0x02;
	playfield_priority = val & 0x04;

	handle_playfield_mirror();

	ball_size = 1 << ((val >> 4) & 0x03);
}

void TIA::refp0(uint8_t val) {
	bool new_p0_reflect = val & 0x4;
	if (new_p0_reflect != player0_reflect)
		player0_mask = reverse_byte(player0_mask);
}

void TIA::refp1(uint8_t val) {
	bool new_p1_reflect = val & 0x4;
	if (new_p1_reflect != player1_reflect)
		player1_mask = reverse_byte(player1_mask);
}

void TIA::pf0(uint8_t val) {
	playfield_mask &= ~0x0F;
	playfield_mask |= val >> 4;
	handle_playfield_mirror();
}

void TIA::pf1(uint8_t val) {
	val = reverse_byte(val);
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
	reset_sprite_position(player0_x, 3, resp_player_offset);
}

void TIA::resp1(uint8_t val) {
	reset_sprite_position(player1_x, 3, resp_player_offset);
}

void TIA::resm0(uint8_t val) {
	reset_sprite_position(missile0_x, 2, resp_missile_ball_offset);
}

void TIA::resm1(uint8_t val) {
	reset_sprite_position(missile1_x, 2, resp_missile_ball_offset);
}

void TIA::resbl(uint8_t val) {
	reset_sprite_position(ball_x, 2, resp_missile_ball_offset);
}

void TIA::grp0(uint8_t val) {
	val = reverse_byte(val);

	if (!player0_mask_delay) {
		player0_mask = val;
	} else {
		player0_mask_buf = val;
	}

	if (player1_mask_delay)
		player1_mask = player1_mask_buf;
}

void TIA::grp1(uint8_t val) {
	val = reverse_byte(val);

	if (!player1_mask_delay) {
		player1_mask = val;
	} else {
		player1_mask_buf = val;
	}

	if (player0_mask_delay)
		player0_mask = player0_mask_buf;

	if (ball_enable_delay)
		ball_enable = ball_enable_buf;
}

void TIA::enam0(uint8_t val) {
	missile0_enable = val & 0x02;
}

void TIA::enam1(uint8_t val) {
	missile1_enable = val & 0x02;
}

void TIA::enabl(uint8_t val) {
	if (!ball_enable_delay) {
		ball_enable = val & 0x02;
	} else {
		ball_enable_buf = val & 0x02;
	}
}

void TIA::hmp0(uint8_t val) {
	player0_motion = -(((int8_t)(val & 0xF0)) / 16);
}

void TIA::hmp1(uint8_t val) {
	player1_motion = -(((int8_t)(val & 0xF0)) / 16);
}

void TIA::hmm0(uint8_t val) {
	missile0_motion = -(((int8_t)(val & 0xF0)) / 16);
}

void TIA::hmm1(uint8_t val) {
	missile1_motion = -(((int8_t)(val & 0xF0)) / 16);
}

void TIA::hmbl(uint8_t val) {
	ball_motion = -(((int8_t)(val & 0xF0)) / 16);
}

void TIA::vdelp0(uint8_t val) {
	player0_mask_delay = val & 0x02;
}

void TIA::vdelp1(uint8_t val) {
	player1_mask_delay = val & 0x02;
}

void TIA::vdelbl(uint8_t val) {
	ball_enable_delay = val & 0x02;
}

void TIA::handle_resmp(int player_scale, int player_x, int& missile_x) {
	switch (player_scale) {
		case 1:
			missile_x = player_x + 3;
			break;
		case 2:
			missile_x = player_x + 6;
			break;
		case 4:
			missile_x = player_x + 10;
			break;
		default:
			printf("Error! Invalid player scale for RESMP call!\n");
			panic();
			break;
	}
}

void TIA::resmp0(uint8_t val) {
	if (val & 0x02)
		handle_resmp(player0_scale, player0_x, missile0_x);
}

void TIA::resmp1(uint8_t val) {
	if (val & 0x02)
		handle_resmp(player1_scale, player1_x, missile1_x);
}

void TIA::hmove(uint8_t val) {
	player0_x += player0_motion;
	player0_x = mod(player0_x, NTSC::visible_columns);
	player1_x += player1_motion;
	player1_x = mod(player1_x, NTSC::visible_columns);
	missile0_x += missile0_motion;
	missile0_x = mod(missile0_x, NTSC::visible_columns);
	missile1_x += missile1_motion;
	missile1_x = mod(missile1_x, NTSC::visible_columns);
	ball_x += ball_motion;
	ball_x = mod(ball_x, NTSC::visible_columns);
}

void TIA::hmclr(uint8_t val) {
	player0_motion = 0;
	player1_motion = 0;
	missile0_motion = 0;
	missile1_motion = 0;
	ball_motion = 0;
}

void TIA::cxclr(uint8_t val) {
	missile0_player1 = false;
	missile0_player0 = false;
	missile1_player0 = false;
	missile1_player1 = false;
	player0_playfield = false;
	player0_ball = false;
	player1_playfield = false;
	player1_ball = false;
	missile0_playfield = false;
	missile0_ball = false;
	missile1_playfield = false;
	missile1_ball = false;
	ball_playfield = false;
	player0_player1 = false;
	missile0_missile1 = false;
}


uint8_t TIA::cxm0p() {
	return (uint8_t)missile0_player0 << 7 | (uint8_t)missile0_player1 << 6;
}

uint8_t TIA::cxm1p() {
	return (uint8_t)missile1_player0 << 7 | (uint8_t)missile1_player1 << 6;
}

uint8_t TIA::cxp0fb() {
	return (uint8_t)player0_playfield << 7 | (uint8_t)player0_ball << 6;
}

uint8_t TIA::cxp1fb() {
	return (uint8_t)player1_playfield << 7 | (uint8_t)player1_ball << 6;
}

uint8_t TIA::cxm0fb() {
	return (uint8_t)missile0_playfield << 7 | (uint8_t)missile0_ball << 6;
}

uint8_t TIA::cxm1fb() {
	return (uint8_t)missile1_playfield << 7 | (uint8_t)missile1_ball << 6;
}

uint8_t TIA::cxblpf() {
	return (uint8_t)ball_playfield << 7;
}

uint8_t TIA::cxppmm() {
	return (uint8_t)player0_player1 << 7 | (uint8_t)missile0_missile1 << 6;
}


// TODO: Implement actual joystick controls
uint8_t TIA::inpt0() {
	return 0;
}

uint8_t TIA::inpt1() {
	return 0;
}

uint8_t TIA::inpt2() {
	return 0;
}

uint8_t TIA::inpt3() {
	return 0;
}

uint8_t TIA::inpt4() {
	return ~(uint8_t)player0_fire << 7;
}

uint8_t TIA::inpt5() {
	return ~(uint8_t)player1_fire << 7;
}

// TODO: Add audio support
void TIA::audv0(uint8_t val) {}
void TIA::audv1(uint8_t val) {}
void TIA::audf0(uint8_t val) {}
void TIA::audf1(uint8_t val) {}
void TIA::audc0(uint8_t val) {}
void TIA::audc1(uint8_t val) {}


TIA::TIA() {
	dma_region = std::make_shared<DmaRegion>(TIA_START, TIA_END, std::bind(&TIA::dma_read_hook, this, _1), std::bind(&TIA::dma_write_hook, this, _1, _2));
	tia_cycle_num = tia_cycle_ratio * cycle_num;
	last_process_cycle_num = cycle_num;

	dma_write_table[0x00] = std::bind(&TIA::vsync, this, _1);
	dma_write_table[0x01] = std::bind(&TIA::vblank, this, _1);
	dma_write_table[0x02] = std::bind(&TIA::wsync, this, _1);
	dma_write_table[0x03] = std::bind(&TIA::rsync, this, _1);
	dma_write_table[0x04] = std::bind(&TIA::nusiz0, this, _1);
	dma_write_table[0x05] = std::bind(&TIA::nusiz1, this, _1);
	dma_write_table[0x06] = std::bind(&TIA::colup0, this, _1);
	dma_write_table[0x07] = std::bind(&TIA::colup1, this, _1);
	dma_write_table[0x08] = std::bind(&TIA::colupf, this, _1);
	dma_write_table[0x09] = std::bind(&TIA::colubk, this, _1);
	dma_write_table[0x0A] = std::bind(&TIA::ctrlpf, this, _1);
	dma_write_table[0x0B] = std::bind(&TIA::refp0, this, _1);
	dma_write_table[0x0C] = std::bind(&TIA::refp1, this, _1);
	dma_write_table[0x0D] = std::bind(&TIA::pf0, this, _1);
	dma_write_table[0x0E] = std::bind(&TIA::pf1, this, _1);
	dma_write_table[0x0F] = std::bind(&TIA::pf2, this, _1);
	dma_write_table[0x10] = std::bind(&TIA::resp0, this, _1);
	dma_write_table[0x11] = std::bind(&TIA::resp1, this, _1);
	dma_write_table[0x12] = std::bind(&TIA::resm0, this, _1);
	dma_write_table[0x13] = std::bind(&TIA::resm1, this, _1);
	dma_write_table[0x14] = std::bind(&TIA::resbl, this, _1);
	dma_write_table[0x15] = std::bind(&TIA::audc0, this, _1);
	dma_write_table[0x16] = std::bind(&TIA::audc1, this, _1);
	dma_write_table[0x17] = std::bind(&TIA::audf0, this, _1);
	dma_write_table[0x18] = std::bind(&TIA::audf1, this, _1);
	dma_write_table[0x19] = std::bind(&TIA::audv0, this, _1);
	dma_write_table[0x1A] = std::bind(&TIA::audv1, this, _1);
	dma_write_table[0x1B] = std::bind(&TIA::grp0, this, _1);
	dma_write_table[0x1C] = std::bind(&TIA::grp1, this, _1);
	dma_write_table[0x1D] = std::bind(&TIA::enam0, this, _1);
	dma_write_table[0x1E] = std::bind(&TIA::enam1, this, _1);
	dma_write_table[0x1F] = std::bind(&TIA::enabl, this, _1);
	dma_write_table[0x20] = std::bind(&TIA::hmp0, this, _1);
	dma_write_table[0x21] = std::bind(&TIA::hmp1, this, _1);
	dma_write_table[0x22] = std::bind(&TIA::hmm0, this, _1);
	dma_write_table[0x23] = std::bind(&TIA::hmm1, this, _1);
	dma_write_table[0x24] = std::bind(&TIA::hmbl, this, _1);
	dma_write_table[0x25] = std::bind(&TIA::vdelp0, this, _1);
	dma_write_table[0x26] = std::bind(&TIA::vdelp1, this, _1);
	dma_write_table[0x27] = std::bind(&TIA::vdelbl, this, _1);
	dma_write_table[0x28] = std::bind(&TIA::resmp0, this, _1);
	dma_write_table[0x29] = std::bind(&TIA::resmp1, this, _1);
	dma_write_table[0x2A] = std::bind(&TIA::hmove, this, _1);
	dma_write_table[0x2B] = std::bind(&TIA::hmclr, this, _1);
	dma_write_table[0x2C] = std::bind(&TIA::cxclr, this, _1);

	dma_read_table[0x30] = std::bind(&TIA::cxm0p, this);
	dma_read_table[0x31] = std::bind(&TIA::cxm1p, this);
	dma_read_table[0x32] = std::bind(&TIA::cxp0fb, this);
	dma_read_table[0x33] = std::bind(&TIA::cxp1fb, this);
	dma_read_table[0x34] = std::bind(&TIA::cxm0fb, this);
	dma_read_table[0x35] = std::bind(&TIA::cxm1fb, this);
	dma_read_table[0x36] = std::bind(&TIA::cxblpf, this);
	dma_read_table[0x37] = std::bind(&TIA::cxppmm, this);
	dma_read_table[0x38] = std::bind(&TIA::inpt0, this);
	dma_read_table[0x39] = std::bind(&TIA::inpt1, this);
	dma_read_table[0x3A] = std::bind(&TIA::inpt2, this);
	dma_read_table[0x3B] = std::bind(&TIA::inpt3, this);
	dma_read_table[0x3C] = std::bind(&TIA::inpt4, this);
	dma_read_table[0x3D] = std::bind(&TIA::inpt5, this);

	for (int i = 0; i < 0x40; i++)
		dma_write_table[i+0x40] = dma_write_table[i];

	for (int i = 0; i < 0x0D; i++)
		dma_read_table[i] = dma_read_table[i+0x30];
}

void TIA::process_tia() {
	for (uint64_t i = 0; i < cycle_num - last_process_cycle_num; i++) {
		for (int j = 0; j < tia_cycle_ratio; j++)
			process_tia_cycle();
	}

	last_process_cycle_num = cycle_num;

	if (dma_write_request) {
		dma_write_request(dma_val);
		dma_write_request = nullptr;
		dma_val = 0;
	}
}
