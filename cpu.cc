#include "cpu.h"

#include "operand.h"
#include "instructions.h"
#include "registers.h"
#include "memory.h"

bool should_execute;

void execute_next_insn() {
	uint8_t opcode = read_byte(program_counter);
	uint8_t byte1 = read_byte(program_counter+1);
	uint8_t byte2 = read_byte(program_counter+2);

	auto insn = get_insn(opcode);
	auto operand = create_operand(opcode, byte1, byte2);

	insn(operand);

	program_counter += operand->get_insn_len();
}
