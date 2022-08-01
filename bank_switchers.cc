#include "bank_switchers.h"

#include "registers.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

AtariRomRegion::AtariRomRegion(uint16_t start_addr, uint16_t end_addr,
                               uint8_t *init_data,
                               std::vector<uint16_t> bank_addrs) {
  this->num_banks = bank_addrs.size();
  this->start_addr = start_addr - 0x2000 * (num_banks - 1);
  this->end_addr = end_addr;
  this->bank = num_banks - 1;
  type = ROM;

  size_t len = (end_addr - start_addr + 1);
  backing_memory = (uint8_t *)malloc(len);

  for (int i = 0; i < num_banks; i++)
    memcpy(backing_memory + 2 * i * 0x1000, init_data + i * 0x1000, 0x1000);

  for (int i = 0; i < bank_addrs.size(); i++)
    bank_map[bank_addrs[i]] = i;
}

AtariRomRegion::~AtariRomRegion() { free(backing_memory); }

// Technically we are allowing reads to banks that aren't currently mapped,
// because we have the entire ROM in memory. The only thing we really need to
// simulate is the program counter switching to its equivalent position in the
// new bank.
uint8_t AtariRomRegion::read_byte(uint16_t addr) {
  if (bank_map.count(addr & 0xFFF)) {
    bank = bank_map[addr];
    program_counter &= 0xFFF;
    program_counter |= (end_addr & 0xF000) - 0x2000 * (num_banks - bank - 1);
  }

  return backing_memory[addr - start_addr];
}

// Writing to bank switch registers is valid, no other writes are.
void AtariRomRegion::write_byte(uint16_t addr, uint8_t val) {
  if (bank_map.count(addr & 0xFFF)) {
    bank = bank_map[addr];
    program_counter &= 0xFFF;
    program_counter |= (end_addr & 0xF000) - 0x2000 * (num_banks - bank - 1);
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
