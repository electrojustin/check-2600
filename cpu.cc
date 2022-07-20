#include "cpu.h"

#include <unordered_map>
#include <stdio.h>

#include "operand.h"
#include "instructions.h"
#include "registers.h"
#include "memory.h"

bool should_execute;

std::unordered_map<uint16_t, std::function<void()>> instruction_cache;

int cache_insn(uint16_t addr) {
	if (instruction_cache.count(addr))
		instruction_cache.erase(addr);

	uint8_t opcode = read_byte(addr);
	uint8_t byte1 = read_byte(addr+1);
	uint8_t byte2 = read_byte(addr+2);

	auto insn = get_insn(opcode);
	auto operand = create_operand(addr, opcode, byte1, byte2);

	auto exec_step = [=]() {
		insn(operand);
		program_counter += operand->get_insn_len();
		cycle_num += operand->get_cycle_penalty();
	};

	instruction_cache.emplace(std::make_pair(addr, exec_step));

	return operand->get_insn_len();
}

void parse_page(uint16_t addr) {
	uint16_t page = addr & (~(PAGE_SIZE-1));
	addr = page;
	while(addr < page + PAGE_SIZE)
		addr += cache_insn(addr);
}

void invalidate_page(uint16_t page) {
	page = page & (~(PAGE_SIZE-1));
	for (int i = 0; i < PAGE_SIZE; i++) {
		if (instruction_cache.count(page+i)) {
			instruction_cache.erase(page+i);
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
