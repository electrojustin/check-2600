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

// Atari bank switching is very simple. The banks are mapped to memory
// addresses, and if those addresses are either read or written, the
// corresponding bank is swapped in. ROM addresses are mirrored every 0x1000
// starting at 0x1000.

class AtariRomRegion : public MemoryRegion {
private:
  uint8_t *backing_memory;
  int bank = 0;
  int num_banks = 0;
  std::unordered_map<uint16_t, int> bank_map; // The magic memory addresses.

public:
  AtariRomRegion(uint16_t start_addr, uint16_t end_addr, uint8_t *init_data,
                 std::vector<uint16_t> bank_addrs);
  ~AtariRomRegion();
  uint8_t read_byte(uint16_t addr) override;
  void write_byte(uint16_t addr, uint8_t val) override;
  bool has_side_effect(uint16_t addr) override;
};

#endif
