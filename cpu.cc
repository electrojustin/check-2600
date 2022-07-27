#include "cpu.h"

#include <stdio.h>
#include <unordered_map>

#include "instructions.h"
#include "memory.h"
#include "operand.h"
#include "registers.h"

bool should_execute;

std::unordered_map<uint16_t, std::function<void()>> instruction_cache;

int cache_insn(uint16_t addr, bool should_succeed) {
  if (instruction_cache.count(addr))
    return -1;

  uint8_t opcode = read_byte(addr);
  uint8_t byte1 = read_byte(addr + 1);
  uint8_t byte2 = read_byte(addr + 2);

  auto insn = get_insn(opcode, should_succeed);
  if (!insn) {
    // Should succeed is false if we're here, because otherwise the program
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

void parse_page(uint16_t addr) {
  uint16_t page = addr & (~(PAGE_SIZE - 1));
  addr += cache_insn(addr, true);
  while (addr < page + PAGE_SIZE) {
    // The rest of the page may contain code, or it may contain data.
    // We should only crash if the current instruction is invalid.
    int insn_len = cache_insn(addr, false);
    if (insn_len < 0)
      break;
    addr += insn_len;
  }
}

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
