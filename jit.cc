#include "jit.h"

#include "instructions.h"
#include "registers.h"
#include "memory.h"
#include "cpu.h"

void prefix(Operand* operand) {
	cycle_num += operand->get_cycle_penalty();
}

void suffix(Operand* operand) {
	program_counter += operand->get_insn_len();

	if (additional_callback)
		additional_callback();
}

uint32_t Jit::parse_input_code(uint16_t addr) {
	uint32_t num_insn = 0;

	while (is_read_only(addr)) {
		uint8_t opcode = read_byte(addr);
		uint8_t byte1 = read_byte(addr+1);
		uint8_t byte2 = read_byte(addr+2);

		auto insn = get_insn(opcode, false);
		if (!insn)
			break;
		auto operand = create_operand(addr, opcode, byte1, byte2);

		operand_bank[addr] = operand;
		insn_bank[addr] = insn;
		num_insn++;

		if (is_branch(opcode)) {
			break;
		} else {
			addr += operand->get_insn_len();
		}
	}

	return num_insn;
}

void Jit::generate_code(uint16_t addr) {
	uint32_t num_insn = parse_input_code(addr);
	if (!num_insn)
		return;

	uint32_t codegen_size = num_insn*66+1;
	uint32_t codegen_offset = jit_arena.allocate(codegen_size);
	uint8_t* codegen_alloc = codegen_offset + (uint8_t*)jit_arena.get_base();

	for (int i = 0; i < num_insn; i++) {
		program_counter_map[addr] = codegen_offset;

		// movabs <operand ptr>, %rdi
		*codegen_alloc = 0x48;
		codegen_alloc++;
		*codegen_alloc = 0xBF;
		codegen_alloc++;
		*((uint64_t*)codegen_alloc) = (uint64_t)(operand_bank[addr].get());
		codegen_alloc += 8;

		// movabs <prefix ptr>, %rax
		*codegen_alloc = 0x48;
		codegen_alloc++;
		*codegen_alloc = 0xB8;
		codegen_alloc++;
		*((uint64_t*)codegen_alloc) = (uint64_t)prefix;
		codegen_alloc += 8;

		// call *%rax
		*codegen_alloc = 0xFF;
		codegen_alloc++;
		*codegen_alloc = 0xD0;
		codegen_alloc++;

		// movabs <operand ptr>, %rdi
		*codegen_alloc = 0x48;
		codegen_alloc++;
		*codegen_alloc = 0xBF;
		codegen_alloc++;
		*((uint64_t*)codegen_alloc) = (uint64_t)(operand_bank[addr].get());
		codegen_alloc += 8;

		// movabs <insn ptr>, %rax
		*codegen_alloc = 0x48;
		codegen_alloc++;
		*codegen_alloc = 0xB8;
		codegen_alloc++;
		*((uint64_t*)codegen_alloc) = (uint64_t)(insn_bank[addr]);
		codegen_alloc += 8;

		// call *%rax
		*codegen_alloc = 0xFF;
		codegen_alloc++;
		*codegen_alloc = 0xD0;
		codegen_alloc++;

		// movabs <operand ptr>, %rdi
		*codegen_alloc = 0x48;
		codegen_alloc++;
		*codegen_alloc = 0xBF;
		codegen_alloc++;
		*((uint64_t*)codegen_alloc) = (uint64_t)(operand_bank[addr].get());
		codegen_alloc += 8;

		// movabs <suffix ptr>, %rax
		*codegen_alloc = 0x48;
		codegen_alloc++;
		*codegen_alloc = 0xB8;
		codegen_alloc++;
		*((uint64_t*)codegen_alloc) = (uint64_t)suffix;
		codegen_alloc += 8;

		// call *%rax
		*codegen_alloc = 0xFF;
		codegen_alloc++;
		*codegen_alloc = 0xD0;
		codegen_alloc++;

		addr += operand_bank[addr]->get_insn_len();
		codegen_offset += 66;
	}

	// ret
	*codegen_alloc = 0xC3;
}

void* Jit::get_entry(uint16_t program_counter) {
	if (program_counter_map.count(program_counter))
		return (void*)(program_counter_map[program_counter] + (uint8_t*)jit_arena.get_base());

	generate_code(program_counter);

	if (program_counter_map.count(program_counter)) {
		return (void*)(program_counter_map[program_counter] + (uint8_t*)jit_arena.get_base());
	} else {
		return nullptr;
	}
} 
