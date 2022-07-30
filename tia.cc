#include "tia.h"

#include <stdio.h>

#include "atari.h"
#include "input.h"
#include "registers.h"
#include "sound.h"

using std::placeholders::_1;
using std::placeholders::_2;

uint8_t reverse_byte(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

// Default C++ '%' does not necessarily handle negative mods correctly, so we
// wrote our own.
int mod(int a, int b) {
  int ret = a % b;
  return ret < 0 ? ret + b : ret;
}

uint8_t TIA::memory_read_hook(uint16_t addr) {
  auto read_func = memory_read_table[addr];
  if (!read_func) {
    printf("Warning! Invalid TIA read at %x. PC: %x\n", addr, program_counter);
    return 0;
  }

  return read_func();
}

void TIA::memory_write_hook(uint16_t addr, uint8_t val) {
  auto write_func = memory_write_table[addr];
  if (!write_func) {
    printf("Warning! Invalid TIA write at %x. PC: %x\n", addr, program_counter);
    return;
  }

  memory_write_request = write_func;
  memory_val = val;
}

bool TIA::can_draw_playfield(int visible_x) {
  return visible_x >= 0 && ((playfield_mask >> (visible_x / 4)) & 0x01);
}

void TIA::draw_playfield(int visible_x) {
  if (!playfield_score_mode) {
    ntsc.write_pixel(playfield_color);
  } else {
    if (visible_x < NTSC::visible_columns / 2) {
      ntsc.write_pixel(player0_color);
    } else {
      ntsc.write_pixel(player1_color);
    }
  }
}

bool TIA::can_draw_ball(int visible_x) {
  return ball_enable && visible_x >= ball_x && (visible_x - ball_x) < ball_size;
}

void TIA::draw_ball() { ntsc.write_pixel(playfield_color); }

bool TIA::can_draw_player(int visible_x, int player_x, uint8_t player_mask,
                          int duplicate_mask, int scale) {
  if (!duplicate_mask) {
    return visible_x >= player_x &&
           visible_x < player_x + player_size * scale &&
           ((player_mask >> ((visible_x - player_x) / scale)) & 0x01);
  } else {
    return visible_x >= player_x &&
           ((duplicate_mask >> ((visible_x - player_x) / player_size)) &
            0x01) &&
           ((player_mask >> ((visible_x - player_x) % player_size)) & 0x01);
  }
}

void TIA::draw_player(uint8_t player_color) { ntsc.write_pixel(player_color); }

bool TIA::can_draw_missile(int visible_x, int missile_x, int missile_size,
                           bool missile_enabled, int duplicate_mask) {
  if (!missile_enabled)
    return false;

  if (!duplicate_mask) {
    return visible_x >= missile_x && (visible_x - missile_x) < missile_size;
  } else {
    return visible_x >= missile_x &&
           ((duplicate_mask >> (visible_x - missile_x) / player_size) & 0x01) &&
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
    bool player0 = can_draw_player(visible_x, player0_x, player0_mask,
                                   player0_duplicate_mask, player0_scale);
    bool player1 = can_draw_player(visible_x, player1_x, player1_mask,
                                   player1_duplicate_mask, player1_scale);
    bool missile0 = can_draw_missile(visible_x, missile0_x, missile0_size,
                                     missile0_enable, player0_duplicate_mask);
    bool missile1 = can_draw_missile(visible_x, missile1_x, missile1_size,
                                     missile1_enable, player1_duplicate_mask);
    bool ball = can_draw_ball(visible_x);
    bool playfield = can_draw_playfield(visible_x);

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

    // Pick something to draw based on priority logic
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

void TIA::reset_sprite_position(int &sprite, int hblank_fudge, int fudge) {
  sprite = (tia_cycle_num % NTSC::columns) - NTSC::hblank;
  if (sprite < 0) {
    sprite = hblank_fudge;
  } else {
    sprite += fudge;
  }
}

// Bit 1 is the only active bit.
// Vertical sync occurs when we set the VSYNC for 3 scanlines and then clear it.
void TIA::vsync(uint8_t val) {
  bool new_vsync_mode = val & 0x02;
  if (vsync_mode && !new_vsync_mode)
    ntsc.vsync();
  vsync_mode = new_vsync_mode;
}

// Bit 1 is the only active bit.
// Blanks the screen before and after the visible draw area.
void TIA::vblank(uint8_t val) {
  // TODO: Add input control support
  vblank_mode = val == 0x02;
}

// Resets the electron gun to the far left of the screen, no matter what the
// current clock cycle is. Scanline will be unchanged.
void TIA::rsync(uint8_t val) {
  tia_cycle_num = -3;
  ntsc.gun_x = -3;
}

// Sleep the CPU until we finish the scanline. Scanline number will increment,
// and we will output the rest of the pixels on the current line.
void TIA::wsync(uint8_t val) {
  if (tia_cycle_num % NTSC::columns)
    cycle_num +=
        (NTSC::columns - (tia_cycle_num % NTSC::columns)) / tia_cycle_ratio;
}

// NUSIZ registers have complicated behavior.
// Bits 4-5 represent log(missile_size)
// Bits 0-2 represent the "player-missile" number.
// The "player-missile" numbers mean the following:
// 0  One copy, no scaling
// 1  Two copies close together (8 pixels)
// 2  Two copies at medium distance (24 pixels)
// 3  Three copies close together (8 pixels)
// 4  Two copies far apart (56 pixels)
// 5  Player is double scale
// 6  Three copies at medium distance (24 pixels)
// 7  Player is quadruple scale
void TIA::handle_nusiz(uint8_t val, int &dup_mask, int &scale,
                       int &missile_size) {
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

// Set player 0 (and missile 0) color
void TIA::colup0(uint8_t val) { player0_color = val; }

// Set player 1 (and missile 1) color
void TIA::colup1(uint8_t val) { player1_color = val; }

// Set playfield (and ball) color
void TIA::colupf(uint8_t val) { playfield_color = val; }

// Set background color
void TIA::colubk(uint8_t val) { background_color = val; }

// Playfield control
// Bit 0 controls mirroring
// Bit 1 controls "score mode", where the playfield color is replaced by the
// player color. Player 0 for the left half of the screen, player 1 for the
// right half. Bit 2 controls the priority. If true, the playfield will be drawn
// over the players, rather than the other way around.
void TIA::ctrlpf(uint8_t val) {
  playfield_mirrored = val & 0x01;
  playfield_score_mode = val & 0x02;
  playfield_priority = val & 0x04;

  handle_playfield_mirror();

  ball_size = 1 << ((val >> 4) & 0x03);
}

// Only bit 2 is used.
// Controls the reflection of the player sprite.
void TIA::refp0(uint8_t val) {
  bool new_p0_reflect = val & 0x8;
  if (new_p0_reflect != player0_reflect)
    player0_mask = reverse_byte(player0_mask);
  player0_reflect = new_p0_reflect;
}

void TIA::refp1(uint8_t val) {
  bool new_p1_reflect = val & 0x8;
  if (new_p1_reflect != player1_reflect)
    player1_mask = reverse_byte(player1_mask);
  player1_reflect = new_p1_reflect;
}

// Playfield registers. Note that the playfield is either repeated or mirrored,
// so we only need to specify 80 pixels. If we're fast though, we can update the
// registers as the scanline is being written, making an asymmetric playfield.

// Sets the first 16 pixels of the playfield. 1 bit = 4 pixels. Upper 4 bits
// ignored.
void TIA::pf0(uint8_t val) {
  playfield_mask &= ~0x0F;
  playfield_mask |= val >> 4;
  handle_playfield_mirror();
}

// Sets pixels 16-48 of the playfield. 1 bit = 4 pixels. This register is
// reversed from the other two.
void TIA::pf1(uint8_t val) {
  val = reverse_byte(val);
  playfield_mask &= ~0xFF0;
  playfield_mask |= ((uint64_t)val) << 4;
  handle_playfield_mirror();
}

// Sets pixels 48-80 of the playfield. 1 bit = 4 pixels.
void TIA::pf2(uint8_t val) {
  playfield_mask &= ~0xFF000;
  playfield_mask |= ((uint64_t)val) << 12;
  handle_playfield_mirror();
}

// Reset player position to current pixel
void TIA::resp0(uint8_t val) {
  reset_sprite_position(player0_x, 3, resp_player_offset);
}

void TIA::resp1(uint8_t val) {
  reset_sprite_position(player1_x, 3, resp_player_offset);
}

// Reset missile position to current pixel
void TIA::resm0(uint8_t val) {
  reset_sprite_position(missile0_x, 2, resp_missile_ball_offset);
}

void TIA::resm1(uint8_t val) {
  reset_sprite_position(missile1_x, 2, resp_missile_ball_offset);
}

// Reset ball position to current pixel
void TIA::resbl(uint8_t val) {
  reset_sprite_position(ball_x, 2, resp_missile_ball_offset);
}

// Set player sprite. 1 bit = 1 pixel
void TIA::grp0(uint8_t val) {
  if (!player0_reflect) {
    val = reverse_byte(val);
  }

  if (!player0_mask_delay) {
    player0_mask = val;
  } else {
    player0_mask_buf = val;
  }

  if (player1_mask_delay)
    player1_mask = player1_mask_buf;
}

void TIA::grp1(uint8_t val) {
  if (!player1_reflect) {
    val = reverse_byte(val);
  }

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

// Set missiles enabled. Only bit 1 is used.
void TIA::enam0(uint8_t val) { missile0_enable = val & 0x02; }

void TIA::enam1(uint8_t val) { missile1_enable = val & 0x02; }

// Set ball enabled. Only bit 1 is used.
void TIA::enabl(uint8_t val) {
  if (!ball_enable_delay) {
    ball_enable = val & 0x02;
  } else {
    ball_enable_buf = val & 0x02;
  }
}

// Set player "motion".
// Upper 4 bits represent a signed 4 bit value.
void TIA::hmp0(uint8_t val) { player0_motion = -(((int8_t)(val & 0xF0)) / 16); }

void TIA::hmp1(uint8_t val) { player1_motion = -(((int8_t)(val & 0xF0)) / 16); }

// Set missile "motion".
// Upper 4 bits represent a signed 4 bit value.
void TIA::hmm0(uint8_t val) {
  missile0_motion = -(((int8_t)(val & 0xF0)) / 16);
}

void TIA::hmm1(uint8_t val) {
  missile1_motion = -(((int8_t)(val & 0xF0)) / 16);
}

// Set ball "motion".
// Upper 4 bits represent a signed 4 bit value.
void TIA::hmbl(uint8_t val) { ball_motion = -(((int8_t)(val & 0xF0)) / 16); }

// Delay setting GRP0 until GRP1 is set.
// Only bit 1 is used.
void TIA::vdelp0(uint8_t val) { player0_mask_delay = val & 0x02; }

// Delay setting GRP1 until GRP0 is set.
// Only bit 1 is used.
void TIA::vdelp1(uint8_t val) { player1_mask_delay = val & 0x02; }

// Delay setting ENABL until GRP1 is set.
void TIA::vdelbl(uint8_t val) { ball_enable_delay = val & 0x02; }

void TIA::handle_resmp(int player_scale, int player_x, int &missile_x) {
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

// Reset missile position to corresponding player position, plus some offset
// depending on the player scale. Only bit 1 is used.
void TIA::resmp0(uint8_t val) {
  if (val & 0x02)
    handle_resmp(player0_scale, player0_x, missile0_x);
}

void TIA::resmp1(uint8_t val) {
  if (val & 0x02)
    handle_resmp(player1_scale, player1_x, missile1_x);
}

// Change sprite positions based on their "motion" registers. Sprites cannot go
// offscreen from this, so we implement a modulo.
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

// Clear motion registers.
void TIA::hmclr(uint8_t val) {
  player0_motion = 0;
  player1_motion = 0;
  missile0_motion = 0;
  missile1_motion = 0;
  ball_motion = 0;
}

// Clear collision registers
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

// Collision registers have a quirk where they return 0x02 bitwise OR'd with the actual collision values.

// Bit 7 set if missile 0 and player 0 collided.
// Bit 6 set if missile 0 and player 1 collided.
uint8_t TIA::cxm0p() {
  return (uint8_t)missile0_player0 << 7 | (uint8_t)missile0_player1 << 6 | 0x02;
}

// Bit 7 set if missile 1 and player 0 collided.
// Bit 6 set if missile 1 and player 1 collided.
uint8_t TIA::cxm1p() {
  return (uint8_t)missile1_player0 << 7 | (uint8_t)missile1_player1 << 6 | 0x02;
}

// Bit 7 set if player 0 and playfield collided.
// Bit 6 set if player 0 and ball collided.
uint8_t TIA::cxp0fb() {
  return (uint8_t)player0_playfield << 7 | (uint8_t)player0_ball << 6 | 0x02;
}

// Bit 7 set if player 1 and playfield collided.
// Bit 6 set if player 1 and ball collided.
uint8_t TIA::cxp1fb() {
  return (uint8_t)player1_playfield << 7 | (uint8_t)player1_ball << 6 | 0x02;
}

// Bit 7 set if missile 0 and playfield collided.
// Bit 6 set if missile 0 and ball collided.
uint8_t TIA::cxm0fb() {
  return (uint8_t)missile0_playfield << 7 | (uint8_t)missile0_ball << 6 | 0x02;
}

// Bit 7 set if missile 1 and playfield collided.
// Bit 6 set if missile 1 and ball collided.
uint8_t TIA::cxm1fb() {
  return (uint8_t)missile1_playfield << 7 | (uint8_t)missile1_ball << 6 | 0x02;
}

// Bit 7 set ball and playfield collided.
uint8_t TIA::cxblpf() { return (uint8_t)ball_playfield << 7 | 0x02; }

// Bit 7 set if player 0 and player 1 collided.
// Bit 6 set if missile 0 and missile 1 collided.
uint8_t TIA::cxppmm() {
  return (uint8_t)player0_player1 << 7 | (uint8_t)missile0_missile1 << 6 | 0x02;
}

// TODO: Implement actual joystick controls
// Analog in 0
uint8_t TIA::inpt0() { return 0; }

// Analog in 1
uint8_t TIA::inpt1() { return 0; }

// Analog in 2
uint8_t TIA::inpt2() { return 0; }

// Analog in 3
uint8_t TIA::inpt3() { return 0; }

// Bit 7 set to player 0 fire button.
uint8_t TIA::inpt4() { return ~(uint8_t)player0_fire << 7; }

// Bit 7 set to player 1 fire button.
uint8_t TIA::inpt5() { return ~(uint8_t)player1_fire << 7; }

// See more info on Atari 2600 sound in the sounds directory

// Set audio channel 0 volume
void TIA::audv0(uint8_t val) { volume0 = val; }

// Set audio channel 1 volume
void TIA::audv1(uint8_t val) { volume1 = val; }

// Set audio channel 0 *sampling* frequency
void TIA::audf0(uint8_t val) { freq0 = val; }

// Set audio channel 1 *sampling* frequency
void TIA::audf1(uint8_t val) { freq1 = val; }

// Set audio channel 0 waveform
void TIA::audc0(uint8_t val) { noise_control0 = val; }

// Set audio channel 1 waveform
void TIA::audc1(uint8_t val) { noise_control1 = val; }

TIA::TIA() {
  memory_region = std::make_shared<MappedRegion>(
      TIA_START, TIA_END, std::bind(&TIA::memory_read_hook, this, _1),
      std::bind(&TIA::memory_write_hook, this, _1, _2));
  tia_cycle_num = tia_cycle_ratio * cycle_num;
  last_process_cycle_num = cycle_num;

  memory_write_table[0x00] = std::bind(&TIA::vsync, this, _1);
  memory_write_table[0x01] = std::bind(&TIA::vblank, this, _1);
  memory_write_table[0x02] = std::bind(&TIA::wsync, this, _1);
  memory_write_table[0x03] = std::bind(&TIA::rsync, this, _1);
  memory_write_table[0x04] = std::bind(&TIA::nusiz0, this, _1);
  memory_write_table[0x05] = std::bind(&TIA::nusiz1, this, _1);
  memory_write_table[0x06] = std::bind(&TIA::colup0, this, _1);
  memory_write_table[0x07] = std::bind(&TIA::colup1, this, _1);
  memory_write_table[0x08] = std::bind(&TIA::colupf, this, _1);
  memory_write_table[0x09] = std::bind(&TIA::colubk, this, _1);
  memory_write_table[0x0A] = std::bind(&TIA::ctrlpf, this, _1);
  memory_write_table[0x0B] = std::bind(&TIA::refp0, this, _1);
  memory_write_table[0x0C] = std::bind(&TIA::refp1, this, _1);
  memory_write_table[0x0D] = std::bind(&TIA::pf0, this, _1);
  memory_write_table[0x0E] = std::bind(&TIA::pf1, this, _1);
  memory_write_table[0x0F] = std::bind(&TIA::pf2, this, _1);
  memory_write_table[0x10] = std::bind(&TIA::resp0, this, _1);
  memory_write_table[0x11] = std::bind(&TIA::resp1, this, _1);
  memory_write_table[0x12] = std::bind(&TIA::resm0, this, _1);
  memory_write_table[0x13] = std::bind(&TIA::resm1, this, _1);
  memory_write_table[0x14] = std::bind(&TIA::resbl, this, _1);
  memory_write_table[0x15] = std::bind(&TIA::audc0, this, _1);
  memory_write_table[0x16] = std::bind(&TIA::audc1, this, _1);
  memory_write_table[0x17] = std::bind(&TIA::audf0, this, _1);
  memory_write_table[0x18] = std::bind(&TIA::audf1, this, _1);
  memory_write_table[0x19] = std::bind(&TIA::audv0, this, _1);
  memory_write_table[0x1A] = std::bind(&TIA::audv1, this, _1);
  memory_write_table[0x1B] = std::bind(&TIA::grp0, this, _1);
  memory_write_table[0x1C] = std::bind(&TIA::grp1, this, _1);
  memory_write_table[0x1D] = std::bind(&TIA::enam0, this, _1);
  memory_write_table[0x1E] = std::bind(&TIA::enam1, this, _1);
  memory_write_table[0x1F] = std::bind(&TIA::enabl, this, _1);
  memory_write_table[0x20] = std::bind(&TIA::hmp0, this, _1);
  memory_write_table[0x21] = std::bind(&TIA::hmp1, this, _1);
  memory_write_table[0x22] = std::bind(&TIA::hmm0, this, _1);
  memory_write_table[0x23] = std::bind(&TIA::hmm1, this, _1);
  memory_write_table[0x24] = std::bind(&TIA::hmbl, this, _1);
  memory_write_table[0x25] = std::bind(&TIA::vdelp0, this, _1);
  memory_write_table[0x26] = std::bind(&TIA::vdelp1, this, _1);
  memory_write_table[0x27] = std::bind(&TIA::vdelbl, this, _1);
  memory_write_table[0x28] = std::bind(&TIA::resmp0, this, _1);
  memory_write_table[0x29] = std::bind(&TIA::resmp1, this, _1);
  memory_write_table[0x2A] = std::bind(&TIA::hmove, this, _1);
  memory_write_table[0x2B] = std::bind(&TIA::hmclr, this, _1);
  memory_write_table[0x2C] = std::bind(&TIA::cxclr, this, _1);

  memory_read_table[0x00] = std::bind(&TIA::cxm0p, this);
  memory_read_table[0x01] = std::bind(&TIA::cxm1p, this);
  memory_read_table[0x02] = std::bind(&TIA::cxp0fb, this);
  memory_read_table[0x03] = std::bind(&TIA::cxp1fb, this);
  memory_read_table[0x04] = std::bind(&TIA::cxm0fb, this);
  memory_read_table[0x05] = std::bind(&TIA::cxm1fb, this);
  memory_read_table[0x06] = std::bind(&TIA::cxblpf, this);
  memory_read_table[0x07] = std::bind(&TIA::cxppmm, this);
  memory_read_table[0x08] = std::bind(&TIA::inpt0, this);
  memory_read_table[0x09] = std::bind(&TIA::inpt1, this);
  memory_read_table[0x0A] = std::bind(&TIA::inpt2, this);
  memory_read_table[0x0B] = std::bind(&TIA::inpt3, this);
  memory_read_table[0x0C] = std::bind(&TIA::inpt4, this);
  memory_read_table[0x0D] = std::bind(&TIA::inpt5, this);

  for (int i = 0; i < 0x40; i++)
    memory_write_table[i + 0x40] = memory_write_table[i];

  for (int i = 0x10; i < 0x80; i += 0x10) {
    for (int j = 0; j < 0x10; j++)
      memory_read_table[j + i] = memory_read_table[j];
  }
}

void TIA::process_tia() {
  // It's important we process the TIA cycles before the write requests so we
  // get the timing of the "reset sprite position" registers correct. They
  // should always happen at the end of the last clock cycle.
  for (uint64_t i = 0; i < cycle_num - last_process_cycle_num; i++) {
    for (int j = 0; j < tia_cycle_ratio; j++)
      process_tia_cycle();
  }

  last_process_cycle_num = cycle_num;

  if (memory_write_request) {
    memory_write_request(memory_val);
    memory_write_request = nullptr;
    memory_val = 0;
  }
}

void TIA::dump_tia() {
  printf("TIA cycle num: %lu\n", tia_cycle_num);

  printf("Gun X: %d  Gun Y: %d\n", ntsc.gun_x, ntsc.gun_y);

  printf("Background color: %x\n", background_color);

  printf("Playfield / ball color: %x\n", playfield_color);
  printf("Playfield mask: ");
  for (int i = 0; i < 40; i++)
    printf("%c", ((playfield_mask >> i) & 0x01) ? '#' : '_');
  printf("\n");

  printf("Player 0 / Missile 0 color: %x\n", player0_color);
  printf("Player 0 X: %d   Player 0 motion: %d\n", player0_x, player0_motion);
  printf("Player 0 mask: ");
  for (int i = 0; i < 8; i++)
    printf("%c", ((player0_mask >> i) & 0x01) ? '#' : '_');
  printf("\n");
  printf("Missile 0 enabled: %s\n", missile0_enable ? "true" : "false");
  printf("Missile 0 size: %d\n", missile0_size);
  printf("Missile 0 X: %d  Missile 0 motion: %d\n", missile0_x,
         missile0_motion);
  printf("Player 0 scale: %d\n", player0_scale);
  printf("Player-missile 0 copy mask: ");
  for (int i = 0; i < 10; i++)
    printf("%c", ((player0_duplicate_mask >> i) & 0x01) ? '#' : '_');
  printf("\n");

  printf("Player 1 / Missile 1 color: %x\n", player1_color);
  printf("Player 1 X: %d   Player 1 motion: %d\n", player1_x, player1_motion);
  printf("Player 1 mask: ");
  for (int i = 0; i < 8; i++)
    printf("%c", ((player1_mask >> i) & 0x01) ? '#' : '_');
  printf("\n");
  printf("Missile 1 enabled: %s\n", missile1_enable ? "true" : "false");
  printf("Missile 1 size: %d\n", missile1_size);
  printf("Missile 1 X: %d  Missile 1 motion: %d\n", missile1_x,
         missile1_motion);
  printf("Player 1 scale: %d\n", player1_scale);
  printf("Player-missile 1 copy mask: ");
  for (int i = 0; i < 10; i++)
    printf("%c", ((player1_duplicate_mask >> i) & 0x01) ? '#' : '_');
  printf("\n");

  printf("Ball enabled: %s\n", ball_enable ? "true" : "false");
  printf("Ball size: %d\n", ball_size);
  printf("Ball X: %d  Ball motion: %d\n", ball_x, ball_motion);
}
