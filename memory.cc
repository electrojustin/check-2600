#include "memory.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "registers.h"

std::vector<std::shared_ptr<MemoryRegion>> memory_regions;
std::shared_ptr<MemoryRegion> stack_region;

uint16_t irq_vector_addr;

bool dirty_pages[256] = {false};

// Quick lookup table for what region(s) belong to what page.
// This is a speedup over the linear scan.
std::vector<std::shared_ptr<MemoryRegion>> page_table[256];

std::shared_ptr<MemoryRegion> get_region_for_addr(uint16_t addr) {
  // Check the page tables so we don't have to do a linear scan.
  // Technically two different ranges can occupy the same 256 byte page.
  // For example, on the Atari 2600, the TIA and the RAM occupy the zero page.
  for (auto region : page_table[addr >> 8]) {
    if (region->start_addr <= addr && region->end_addr >= addr)
      return region;
  }

  // Linear scan if we haven't cache this page yet.
  std::shared_ptr<MemoryRegion> ret = nullptr;
  for (auto region : memory_regions) {
    if (region->start_addr <= addr && region->end_addr >= addr) {
      ret = region;
      break;
    }
  }
  if (ret)
    page_table[addr >> 8].emplace_back(ret);

  return ret;
}

RamRegion::RamRegion(uint16_t start_addr, uint16_t end_addr) {
  this->start_addr = start_addr;
  this->end_addr = end_addr;
  type = RAM;

  size_t len = end_addr - start_addr + 1;
  backing_memory = (uint8_t *)malloc(len);
  memset(backing_memory, 0, len);
}

RamRegion::~RamRegion() { free(backing_memory); }

uint8_t RamRegion::read_byte(uint16_t addr) {
  return backing_memory[addr - start_addr];
}

void RamRegion::write_byte(uint16_t addr, uint8_t val) {
  backing_memory[addr - start_addr] = val;
  dirty_pages[addr >> 8] = true;
}

RomRegion::RomRegion(uint16_t start_addr, uint16_t end_addr,
                     uint8_t *init_data) {
  this->start_addr = start_addr;
  this->end_addr = end_addr;
  type = ROM;

  size_t len = end_addr - start_addr + 1;
  backing_memory = (uint8_t *)malloc(len);

  memcpy(backing_memory, init_data, len);
}

RomRegion::~RomRegion() { free(backing_memory); }

uint8_t RomRegion::read_byte(uint16_t addr) {
  return backing_memory[addr - start_addr];
}

void RomRegion::write_byte(uint16_t addr, uint8_t val) {
  printf("Error! Attempted to write to ROM address %x\n", addr);
  panic();
}

MappedRegion::MappedRegion(uint16_t start_addr, uint16_t end_addr,
                           std::function<uint8_t(uint16_t)> read_hook,
                           std::function<void(uint16_t, uint8_t)> write_hook) {
  this->start_addr = start_addr;
  this->end_addr = end_addr;
  this->read_hook = read_hook;
  this->write_hook = write_hook;
  type = MAP;
}

uint8_t MappedRegion::read_byte(uint16_t addr) { return read_hook(addr); }

void MappedRegion::write_byte(uint16_t addr, uint8_t val) {
  write_hook(addr, val);
}

MirrorRegion::MirrorRegion(uint16_t start_addr, uint16_t end_addr,
                           std::shared_ptr<MemoryRegion> delegate) {
  this->delegate = delegate;
  this->start_addr = start_addr;
  this->end_addr = end_addr;
}

uint8_t MirrorRegion::read_byte(uint16_t addr) {
  return delegate->read_byte(addr - start_addr + delegate->start_addr);
}

void MirrorRegion::write_byte(uint16_t addr, uint8_t val) {
  delegate->write_byte(addr - start_addr + delegate->start_addr, val);
}

uint8_t read_byte(uint16_t addr) {
  auto region = get_region_for_addr(addr);
  if (!region) {
    printf("Error! Invalid read at address %x\n", addr);
    panic();
    return -1;
  } else {
    return region->read_byte(addr);
  }
}

void write_byte(uint16_t addr, uint8_t val) {
  auto region = get_region_for_addr(addr);
  if (!region) {
    printf("Error! Invalid write at address %x\n", addr);
    panic();
  } else {
    region->write_byte(addr, val);
  }
}

void push_byte(uint8_t val) {
  uint16_t stack_page = stack_region->start_addr & (~(PAGE_SIZE - 1));

  // Note that the stack pointer might take us out of the designated stack
  // segment
  write_byte(stack_page + stack_pointer, val);
  stack_pointer--;
}

uint8_t pop_byte() {
  uint16_t stack_page = stack_region->start_addr & (~(PAGE_SIZE - 1));
  stack_pointer++;

  // Note that the stack pointer might take us out of the designated stack
  // segment
  uint8_t ret = read_byte(stack_page + stack_pointer);

  return ret;
}

void push_word(uint16_t val) {
  push_byte(val >> 8);
  push_byte(val & 0xFF);
}

uint16_t pop_word() {
  uint16_t byte1 = pop_byte();
  uint16_t byte2 = pop_byte();

  return byte1 | (byte2 << 8);
}

uint16_t read_word(uint16_t addr) {
  uint16_t ret = read_byte(addr + 1);
  ret = (ret << 8) | read_byte(addr);
  return ret;
}

void write_word(uint16_t addr, uint16_t val) {
  write_byte(addr, val & 0xFF);
  write_byte(addr + 1, val >> 8);
}

bool has_side_effect(uint16_t addr) {
  auto region = get_region_for_addr(addr);
  return region->has_side_effect(addr);
}

bool is_dirty_page(uint16_t addr) { return dirty_pages[addr >> 8]; }

void mark_page_clean(uint16_t addr) { dirty_pages[addr >> 8] = false; }

// Dumps all 128 bytes of RAM to STDOUT
void dump_memory() {
  printf("RAM:\n");
  printf("  ");
  for (int low_nibble = 0; low_nibble <= 0xF; low_nibble++) {
    printf("%x  ", low_nibble);
  }
  printf("\n");
  for (int high_nibble = 0x8; high_nibble <= 0xF; high_nibble++) {
    printf("%x ", high_nibble);
    for (int low_nibble = 0; low_nibble <= 0xF; low_nibble++) {
      printf("%02x ", read_byte(high_nibble << 4 | low_nibble));
    }
    printf("\n");
  }
  printf("\n");
}
