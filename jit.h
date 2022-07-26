#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <functional>

#include "jit_arena.h"
#include "operand.h"

#ifndef JIT_H
#define JIT_H

class Jit {
private:
	JitArena jit_arena;
	std::unordered_map<uint16_t, uint32_t> program_counter_map;
	std::unordered_map<uint16_t, std::shared_ptr<Operand>> operand_bank; // Keeps the shared pointers from freeing our operands
	std::unordered_map<uint16_t, void(*)(Operand*)> insn_bank;

	uint32_t parse_input_code(uint16_t addr);
	void generate_code(uint16_t addr);
public:
	void* get_entry(uint16_t program_counter);
};

#endif
