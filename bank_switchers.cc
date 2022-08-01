#include "bank_switchers.h"

#include "registers.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

AtariRomRegion::AtariRomRegion(uint16_t start_addr, uint16_t end_addr,
                               uint8_t *init_data,
                               std::vector<uint16_t> bank_addrs) {
  this->num_banks = bank_addrs.size();
  this->start_addr = start_addr;
  this->end_addr = end_addr;
  this->bank = num_banks - 1;
  type = ROM;

  size_t len = (this->end_addr - this->start_addr + 1) * num_banks;
  backing_memory = (uint8_t *)malloc(len);

  memcpy(backing_memory, init_data, len);

  for (int i = 0; i < bank_addrs.size(); i++)
    bank_map[bank_addrs[i]] = i;
}

AtariRomRegion::~AtariRomRegion() { free(backing_memory); }

uint8_t AtariRomRegion::read_byte(uint16_t addr) {
  if (bank_map.count(addr & 0xFFF)) {
    bank = bank_map[addr & 0xFFF];

    // We have to dump the entire instruction cache.
    for (int i = 0x1000; i < end_addr; i += 0x100)
      mark_page_dirty(i);
  }

  return backing_memory[addr - start_addr + 0x1000 * bank];
}

// Writing to bank switch registers is valid, no other writes are.
void AtariRomRegion::write_byte(uint16_t addr, uint8_t val) {
  if (bank_map.count(addr & 0xFFF)) {
    bank = bank_map[addr & 0xFFF];

    for (int i = 0x1000; i < end_addr; i += 0x100)
      mark_page_dirty(i);
  } else {
    printf("Error! Attempted to write to ROM address %x\n", addr);
    panic();
  }
}

// We don't want the instruction cache to accidentally switch banks just because
// we're in the same page.
bool AtariRomRegion::has_side_effect(uint16_t addr) {
  return bank_map.count(addr & 0xFFF);
}
