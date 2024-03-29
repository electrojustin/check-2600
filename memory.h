#include <functional>
#include <memory>
#include <stdint.h>
#include <vector>

#ifndef MEMORY_H
#define MEMORY_H

#define PAGE_SIZE 0x100

enum MemoryType { ROM, RAM, MAP, MIRROR };

// Generic class for memory region information.
class MemoryRegion {
public:
  MemoryType type;
  uint16_t start_addr;
  uint16_t end_addr;

  virtual uint8_t read_byte(uint16_t addr) = 0;
  virtual void write_byte(uint16_t addr, uint8_t val) = 0;

  // Some memory addresses have side effects when you read from them, such as
  // the bank switching addresses. This means we should not cache them, because
  // we might trigger something unintentionally.
  virtual bool has_side_effect(uint16_t addr) { return false; }
};

// General purpose read/write memory. Also the memory type for the stack.
class RamRegion : public MemoryRegion {
private:
  uint8_t *backing_memory;

public:
  RamRegion(uint16_t start_addr, uint16_t end_addr);
  ~RamRegion();
  uint8_t read_byte(uint16_t addr) override;
  void write_byte(uint16_t addr, uint8_t val) override;
};

// Read only memory
class RomRegion : public MemoryRegion {
private:
  uint8_t *backing_memory;

public:
  RomRegion(uint16_t start_addr, uint16_t end_addr, uint8_t *init_data);
  ~RomRegion();
  uint8_t read_byte(uint16_t addr) override;
  void write_byte(uint16_t addr, uint8_t val) override;
};

// Memory mapped peripheral (PIA and TIA)
class MappedRegion : public MemoryRegion {
private:
  std::function<uint8_t(uint16_t)> read_hook;
  std::function<void(uint16_t, uint8_t)> write_hook;

public:
  MappedRegion(uint16_t start_addr, uint16_t end_addr,
               std::function<uint8_t(uint16_t)> read_hook,
               std::function<void(uint16_t, uint8_t)> write_hook);
  uint8_t read_byte(uint16_t addr) override;
  void write_byte(uint16_t addr, uint8_t val) override;
  bool has_side_effect(uint16_t addr) override { return true; }
};

// "Mirror" region. Because of quirks in the Atari's addressing bus, multiple
// addresses can actually point to the same memory. This is actually used a fair
// amount in some games, because the stack, usually at 0x100, is actually
// mirrored in the zeropage for easy access.
class MirrorRegion : public MemoryRegion {
private:
  std::shared_ptr<MemoryRegion> delegate;

public:
  MirrorRegion(uint16_t start_addr, uint16_t end_addr,
               std::shared_ptr<MemoryRegion> delegate);
  uint8_t read_byte(uint16_t addr) override;
  void write_byte(uint16_t addr, uint8_t val) override;
  bool has_side_effect(uint16_t addr) override;
};

extern std::vector<std::shared_ptr<MemoryRegion>> memory_regions;
// stack_region should be a pointer to a memory address already in the above
// vector.
extern std::shared_ptr<MemoryRegion> stack_region;
extern uint16_t irq_vector_addr;

uint8_t read_byte(uint16_t addr);
uint16_t read_word(uint16_t addr);
void write_byte(uint16_t addr, uint8_t val);
void write_word(uint16_t addr, uint16_t val);
void push_byte(uint8_t val);
void push_word(uint16_t val);
uint8_t pop_byte();
uint16_t pop_word();

bool has_side_effect(uint16_t addr);

// Cache control. Useful for caching parsed instructions.
bool is_dirty_page(uint16_t addr);
void mark_page_clean(uint16_t addr);
void mark_page_dirty(uint16_t addr);

// Print all 128 bytes of RAM to STDOUT
void dump_memory();

#endif
