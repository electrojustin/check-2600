#include "cpu.h"

#include <stdio.h>
#include <unordered_map>

#include "instructions.h"
#include "memory.h"
#include "operand.h"
#include "registers.h"

bool should_execute;

// Cache of pre-parsed instructions. Not quite a JIT, but, sorta similar in
// concept.
std::unordered_map<uint16_t, std::function<void()>> instruction_cache;

// Cache a single instruction at the given address
int cache_insn(uint16_t addr, bool should_succeed) {

  // This scenario can happen if we previously cache a series of instructions
  // and then jumped to a location earlier in the program. If that's the case,
  // the instruction cache should already be full for the rest of the page, so
  // we can stop parsing.
  if (instruction_cache.count(addr))
    return -1;

  // Note we don't always need byte1 and byte2 depending on the specific
  // instruction. The logic in create_operand() will ignore them when necessary.
  uint8_t opcode;
  uint8_t byte1;
  uint8_t byte2;

  // Don't accidentally trigger a side effect
  if (has_side_effect(addr)) {
    opcode = 0;
  } else {
    opcode = read_byte(addr);
  }
  if (has_side_effect(addr + 1)) {
    byte1 = 0;
  } else {
    byte1 = read_byte(addr + 1);
  }
  if (has_side_effect(addr + 2)) {
    byte2 = 0;
  } else {
    byte2 = read_byte(addr + 2);
  }

  auto insn = get_insn(opcode, should_succeed);
  if (!insn) {
    // should_succeed is false if we're here, because otherwise the program
    // would be crashed by now.
    return -1;
  }

  auto operand = create_operand(addr, opcode, byte1, byte2);

  auto exec_step = [=]() {
    // Note that we try to increment the cycle counter before evaluating the
    // operand to accurately read timers
    cycle_num += operand->get_cycle_penalty();
    insn(operand);
    program_counter += operand->get_insn_len();
  };

  instruction_cache.emplace(std::make_pair(addr, exec_step));

  return operand->get_insn_len();
}

// Parse from |addr| until the end of the page |addr| is located in.
void parse_page(uint16_t addr) {
  uint16_t page = addr & (~(PAGE_SIZE - 1));
  addr += cache_insn(addr, true);
  while (addr < page + PAGE_SIZE) {
    // The rest of the page may contain code, or it may contain data.
    // We should only crash if the current instruction is invalid, not if there
    // just happens to be invalid instruction data elsewhere in the page.
    int insn_len = cache_insn(addr, false);
    if (insn_len < 0)
      break;
    addr += insn_len;
  }
}

// In the event of self modifying code, this method will invalidate our
// instruction cache for the entire page.
void invalidate_page(uint16_t page) {
  page = page & (~(PAGE_SIZE - 1));
  for (int i = 0; i < PAGE_SIZE; i++) {
    if (instruction_cache.count(page + i)) {
      instruction_cache.erase(page + i);
    }
  }

  mark_page_clean(page);
}

void execute_next_insn() {
  if (instruction_cache.count(program_counter)) {
    if (is_dirty_page(program_counter)) {
      invalidate_page(program_counter);
      parse_page(program_counter);
    }
  } else {
    parse_page(program_counter);
  }

  instruction_cache.at(program_counter)();
}
