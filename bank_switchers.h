#include "memory.h"

#include <unordered_map>
#include <vector>

#ifndef BANK_SWITCHERS_H
#define BANK_SWITCHERS_H

enum BankSwitcherType {
  none,
  atari8k,
  atari16k,
  atari32k,
};

// Atari 8K maps to 0xD000-0xDFFF and 0xF000-0xFFFF.
// Atari 16K maps to 0x9000-0x9FFF, 0xB000-0xBFFF, 0xD000-0xDFFF, and
// 0xF000-0xFFFF. Similar pattern for Atari 32K.

class AtariRomRegion : public MemoryRegion {
private:
  uint8_t *backing_memory;
  int bank = 0;
  int num_banks = 0;
  std::unordered_map<uint16_t, int> bank_map;

public:
  AtariRomRegion(uint16_t start_addr, uint16_t end_addr, uint8_t *init_data,
                 std::vector<uint16_t> bank_addrs);
  ~AtariRomRegion();
  uint8_t read_byte(uint16_t addr) override;
  void write_byte(uint16_t addr, uint8_t val) override;
  bool has_side_effect(uint16_t addr) override;
};

#endif
