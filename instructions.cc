#include "instructions.h"

#include <stdio.h>

#include "registers.h"
#include "memory.h"
#include "cpu.h"

void handle_arithmetic_flags(int result) {
	set_zero(!(result & 0xFF));
	set_negative(result & 0x80);
}

void handle_overflow(int val1, int val2, int result) {
	if ((val1 & 0x80) && (val2 & 0x80) && !(result & 0x80)) {
		set_overflow(true);
	} else if (!(val1 & 0x80) && !(val2 & 0x80) && (result & 0x80)) {
		set_overflow(true);
	} else {
		set_overflow(false);
	}
}

// Note that we try to increment the cycle counter before evaluating the operand to accurately read timers

__attribute__((sysv_abi, noinline)) void _adc(Operand* operand) {
	int carry = get_carry() ? 1 : 0;
	int result;
	if (!get_decimal()) {
		result = operand->get_val() + acc + carry;
		set_carry(result & (~0xFF));
	} else {
		int acc_digit0 = acc & 0xF;
		int acc_digit1 = acc >> 4;
		int operand_digit0 = operand->get_val() & 0xF;
		int operand_digit1 = operand->get_val() >> 4;
		int result_digit0 = acc_digit0 + operand_digit0 + carry;
		carry = result_digit0 > 9;
		result_digit0 = result_digit0 % 10;
		int result_digit1 = acc_digit1 + operand_digit1 + carry;
		carry = result_digit1 > 9;
		set_carry(carry);
		result_digit1 = result_digit1 % 10;
		result = carry << 8 | result_digit1 << 4 | result_digit0;
	}
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), acc, result);
	acc = result & 0xFF;
}

__attribute__((sysv_abi, noinline)) void _and(Operand* operand) {
	int result = operand->get_val() & acc;
	handle_arithmetic_flags(result);
	acc = result & 0xFF;
}

__attribute__((sysv_abi, noinline)) void _asl(Operand* operand) {
	cycle_num += 2;

	int result = (int)acc << operand->get_val();
	handle_arithmetic_flags(result);
	set_carry(result & (~0xFF));
	acc = result & 0xFF;
}

__attribute__((sysv_abi, noinline)) void _bcc(Operand* operand) {
	cycle_num += 2;

	if (!get_carry()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

__attribute__((sysv_abi, noinline)) void _bcs(Operand* operand) {
	cycle_num += 2;

	if (get_carry()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

__attribute__((sysv_abi, noinline)) void _beq(Operand* operand) {
	cycle_num += 2;

	if (get_zero()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

__attribute__((sysv_abi, noinline)) void _bit(Operand* operand) {
	set_negative(operand->get_val() & 0x80);
	set_overflow(operand->get_val() & 0x40);
	set_zero(!(operand->get_val() & acc));
}

__attribute__((sysv_abi, noinline)) void _bmi(Operand* operand) {
	cycle_num += 2;

	if (get_negative()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

__attribute__((sysv_abi, noinline)) void _bne(Operand* operand) {
	cycle_num += 2;

	if (!get_zero()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

__attribute__((sysv_abi, noinline)) void _bpl(Operand* operand) {
	cycle_num += 2;

	if (!get_negative()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

__attribute__((sysv_abi, noinline)) void _brk(Operand* operand) {
	int irq_vector = read_word(irq_vector_addr);
	if (!irq_vector) {
		should_execute = false;
		return;
	}

	if (!get_interrupt_enable())
		return;

	cycle_num += 7;

	push_word(program_counter + operand->get_insn_len() + 1); // Leave extra space for a break mark
	push_byte(flags);
	program_counter = irq_vector - operand->get_insn_len();
	set_break(true);
}

__attribute__((sysv_abi, noinline)) void _bvc(Operand* operand) {
	cycle_num += 2;

	if (!get_overflow()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

__attribute__((sysv_abi, noinline)) void _bvs(Operand* operand) {
	cycle_num += 2;

	if (get_overflow()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

__attribute__((sysv_abi, noinline)) void _clc(Operand* operand) {
	cycle_num += 2;

	set_carry(false);
}

__attribute__((sysv_abi, noinline)) void _cld(Operand* operand) {
	cycle_num += 2;

	set_decimal(false);
}

__attribute__((sysv_abi, noinline)) void _cli(Operand* operand) {
	cycle_num += 2;

	set_interrupt_enable(false);
}

__attribute__((sysv_abi, noinline)) void _clv(Operand* operand) {
	cycle_num += 2;

	set_overflow(false);
}

__attribute__((sysv_abi, noinline)) void _cmp(Operand* operand) {
	int result = acc - operand->get_val();
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), acc, result);
	set_carry(!(result & (~0xFF)));
}

__attribute__((sysv_abi, noinline)) void _cpx(Operand* operand) {
	int result = index_x - operand->get_val();
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), index_x, result);
	set_carry(!(result & (~0xFF)));
}

__attribute__((sysv_abi, noinline)) void _cpy(Operand* operand) {
	int result = index_y - operand->get_val();
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), index_y, result);
	set_carry(!(result & (~0xFF)));
}

__attribute__((sysv_abi, noinline)) void _dec(Operand* operand) {
	cycle_num += 2;

	int result = operand->get_val() - 1;
	handle_arithmetic_flags(result);
	operand->set_val(result & 0xFF);
}

__attribute__((sysv_abi, noinline)) void _dex(Operand* operand) {
	cycle_num += 2;

	index_x--;
	handle_arithmetic_flags(index_x);
}

__attribute__((sysv_abi, noinline)) void _dey(Operand* operand) {
	cycle_num += 2;

	index_y--;
	handle_arithmetic_flags(index_y);
}

__attribute__((sysv_abi, noinline)) void _eor(Operand* operand) {
	int result = operand->get_val() ^ acc;
	handle_arithmetic_flags(result);
	acc = result;
}

__attribute__((sysv_abi, noinline)) void _inc(Operand* operand) {
	cycle_num += 2;

	int result = operand->get_val() + 1;
	handle_arithmetic_flags(result);
	operand->set_val(result & 0xFF);
}

__attribute__((sysv_abi, noinline)) void _inx(Operand* operand) {
	cycle_num += 2;

	index_x++;
	handle_arithmetic_flags(index_x);
}

__attribute__((sysv_abi, noinline)) void _iny(Operand* operand) {
	cycle_num += 2;

	index_y++;
	handle_arithmetic_flags(index_y);
}

__attribute__((sysv_abi, noinline)) void _jmp(Operand* operand) {
	cycle_num--; // Weird hack for this particular instruction.

	program_counter = operand->get_val() - operand->get_insn_len();
}

__attribute__((sysv_abi, noinline)) void _jsr(Operand* operand) {
	cycle_num += 2;

	// A quirk in the 6502 stores return address - 1 for JSR.
	push_word(program_counter + operand->get_insn_len() - 1);
	program_counter = operand->get_val() - operand->get_insn_len();
}

__attribute__((sysv_abi, noinline)) void _lda(Operand* operand) {
	acc = operand->get_val();
	handle_arithmetic_flags(acc);
}

__attribute__((sysv_abi, noinline)) void _ldx(Operand* operand) {
	index_x = operand->get_val();
	handle_arithmetic_flags(index_x);
}

__attribute__((sysv_abi, noinline)) void _ldy(Operand* operand) {
	index_y = operand->get_val();
	handle_arithmetic_flags(index_y);
}

__attribute__((sysv_abi, noinline)) void _lsr(Operand* operand) {
	cycle_num += 2;

	int result = acc >> operand->get_val();
	handle_arithmetic_flags(result);
	set_carry(((uint16_t)acc << (8-operand->get_val())) & 0xFF);
	acc = result & 0xFF;
}

__attribute__((sysv_abi, noinline)) void _nop(Operand* operand) {
	cycle_num += 2;
}

__attribute__((sysv_abi, noinline)) void _ora(Operand* operand) {
	int result = acc | operand->get_val();
	handle_arithmetic_flags(result);
	acc = result & 0xFF;
}

__attribute__((sysv_abi, noinline)) void _pha(Operand* operand) {
	cycle_num += 3;

	push_byte(acc);
}

__attribute__((sysv_abi, noinline)) void _php(Operand* operand) {
	cycle_num += 3;

	push_byte(flags);
}

__attribute__((sysv_abi, noinline)) void _pla(Operand* operand) {
	cycle_num += 4;

	acc = pop_byte();
	handle_arithmetic_flags(acc);
}

__attribute__((sysv_abi, noinline)) void _plp(Operand* operand) {
	cycle_num += 4;

	flags = pop_byte();
}

uint8_t rotate_left(uint8_t input) {
	cycle_num += 2;

	int result = input << 1 | get_carry();
	handle_arithmetic_flags(result);
	set_carry(input & 0x80);

	return result & 0xFF;
}

__attribute__((sysv_abi, noinline)) void _rol_acc(Operand* operand) {
	acc = rotate_left(acc);
}

__attribute__((sysv_abi, noinline)) void _rol_memory(Operand* operand) {
	operand->set_val(rotate_left(operand->get_val()));
}

uint8_t rotate_right(uint8_t input) {
	cycle_num += 2;

	int result = input >> 1 | (int)get_carry() << 7;
	handle_arithmetic_flags(result);
	set_carry(input & 0x01);

	return result & 0xFF;
}

__attribute__((sysv_abi, noinline)) void _ror_acc(Operand* operand) {
	acc = rotate_right(acc);
}

__attribute__((sysv_abi, noinline)) void _ror_memory(Operand* operand) {
	operand->set_val(rotate_right(operand->get_val()));
}

__attribute__((sysv_abi, noinline)) void _rti(Operand* operand) {
	cycle_num += 6;

	flags = pop_byte();
	program_counter = pop_word() - operand->get_insn_len();
}

__attribute__((sysv_abi, noinline)) void _rts(Operand* operand) {
	cycle_num += 6;

	program_counter = pop_word() - operand->get_insn_len() + 1;
}

__attribute__((sysv_abi, noinline)) void _sbc(Operand* operand) {
	int carry = get_carry() ? 0 : 1;
	int result;
	if (!get_decimal()) {
		result = acc - operand->get_val() - carry;
		set_carry(!(result & (~0xFF)));
	} else {
		int acc_digit0 = acc & 0xF;
		int acc_digit1 = acc >> 4;
		int operand_digit0 = operand->get_val() & 0xF;
		int operand_digit1 = operand->get_val() >> 4;
		int result_digit0 = acc_digit0 - operand_digit0 - carry;
		carry = result_digit0 < 0;
		if (result_digit0 < 0)
			result_digit0 += 10;
		int result_digit1 = acc_digit1 - operand_digit1 - carry;
		carry = result_digit1 < 0;
		set_carry(!carry);
		if (result_digit1 < 0)
			result_digit1 += 10;
		result = carry << 8 | result_digit1 << 4 | result_digit0;
	}
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), acc, result);
	acc = result & 0xFF;
}

__attribute__((sysv_abi, noinline)) void _sec(Operand* operand) {
	cycle_num += 2;

	set_carry(true);
}

__attribute__((sysv_abi, noinline)) void _sed(Operand* operand) {
	cycle_num += 2;

	set_decimal(true);
}

__attribute__((sysv_abi, noinline)) void _sei(Operand* operand) {
	cycle_num += 2;

	set_interrupt_enable(true);
}

__attribute__((sysv_abi, noinline)) void _sta(Operand* operand) {
	operand->set_val(acc);
}

__attribute__((sysv_abi, noinline)) void _stx(Operand* operand) {
	operand->set_val(index_x);
}

__attribute__((sysv_abi, noinline)) void _sty(Operand* operand) {
	operand->set_val(index_y);
}

__attribute__((sysv_abi, noinline)) void _tax(Operand* operand) {
	cycle_num += 2;

	index_x = acc;
	handle_arithmetic_flags(index_x);
}

__attribute__((sysv_abi, noinline)) void _tay(Operand* operand) {
	cycle_num += 2;

	index_y = acc;
	handle_arithmetic_flags(index_y);
}

__attribute__((sysv_abi, noinline)) void _tsx(Operand* operand) {
	cycle_num += 2;

	stack_pointer = index_x;
	handle_arithmetic_flags(stack_pointer);
}

__attribute__((sysv_abi, noinline)) void _txa(Operand* operand) {
	cycle_num += 2;

	acc = index_x;
	handle_arithmetic_flags(acc);
}

__attribute__((sysv_abi, noinline)) void _txs(Operand* operand) {
	cycle_num += 2;

	stack_pointer = index_x;
	handle_arithmetic_flags(stack_pointer);
}

__attribute__((sysv_abi, noinline)) void _tya(Operand* operand) {
	cycle_num += 2;

	acc = index_y;
	handle_arithmetic_flags(acc);
}

void (*opcode_table[256])(Operand*) = {
        _brk,
        _ora,
        nullptr,
        nullptr,
        nullptr,
        _ora,
        _asl,
        nullptr,
        _php,
        _ora,
        _asl,
        nullptr,
        nullptr,
        _ora,
        _asl,
        nullptr,
        _bpl,
        _ora,
        nullptr,
        nullptr,
        nullptr,
        _ora,
        _asl,
        nullptr,
        _clc,
        _ora,
        nullptr,
        nullptr,
        nullptr,
        _ora,
        _asl,
        nullptr,
        _jsr,
        _and,
        nullptr,
        nullptr,
        _bit,
        _and,
        _rol_memory,
        nullptr,
        _plp,
        _and,
        _rol_acc,
        nullptr,
        _bit,
        _and,
        _rol_memory,
        nullptr,
        _bmi,
        _and,
        nullptr,
        nullptr,
        nullptr,
        _and,
        _rol_memory,
        nullptr,
        _sec,
        _and,
        nullptr,
        nullptr,
        nullptr,
        _and,
        _rol_memory,
        nullptr,
        _rti,
        _eor,
        nullptr,
        nullptr,
        nullptr,
        _eor,
        _lsr,
        nullptr,
        _pha,
        _eor,
        _lsr,
        nullptr,
        _jmp,
        _eor,
        _lsr,
        nullptr,
        _bvc,
        _eor,
        nullptr,
        nullptr,
        nullptr,
        _eor,
        _lsr,
        nullptr,
        _cli,
        _eor,
        nullptr,
        nullptr,
        nullptr,
        _eor,
        _lsr,
        nullptr,
        _rts,
        _adc,
        nullptr,
        nullptr,
        nullptr,
        _adc,
        _ror_memory,
        nullptr,
        _pla,
        _adc,
        _ror_acc,
        nullptr,
        _jmp,
        _adc,
        _ror_memory,
        nullptr,
        _bvs,
        _adc,
        nullptr,
        nullptr,
        nullptr,
        _adc,
        _ror_memory,
        nullptr,
        _sei,
        _adc,
        nullptr,
        nullptr,
        nullptr,
        _adc,
        _ror_memory,
        nullptr,
        nullptr,
        _sta,
        nullptr,
        nullptr,
        _sty,
        _sta,
        _stx,
        nullptr,
        _dey,
        nullptr,
        _txa,
        nullptr,
        _sty,
        _sta,
        _stx,
        nullptr,
        _bcc,
        _sta,
        nullptr,
        nullptr,
        _sty,
        _sta,
        _stx,
        nullptr,
        _tya,
        _sta,
        _txs,
        nullptr,
        nullptr,
        _sta,
        nullptr,
        nullptr,
        _ldy,
        _lda,
        _ldx,
        nullptr,
        _ldy,
        _lda,
        _ldx,
        nullptr,
        _tay,
        _lda,
        _tax,
        nullptr,
        _ldy,
        _lda,
        _ldx,
        nullptr,
        _bcs,
        _lda,
        nullptr,
        nullptr,
        _ldy,
        _lda,
        _ldx,
        nullptr,
        _clv,
        _lda,
        _tsx,
        nullptr,
        _ldy,
        _lda,
        _ldx,
        nullptr,
        _cpy,
        _cmp,
        nullptr,
        nullptr,
        _cpy,
        _cmp,
        _dec,
        nullptr,
        _iny,
        _cmp,
        _dex,
        nullptr,
        _cpy,
        _cmp,
        _dec,
        nullptr,
        _bne,
        _cmp,
        nullptr,
        nullptr,
        nullptr,
        _cmp,
        _dec,
        nullptr,
        _cld,
        _cmp,
        nullptr,
        nullptr,
        nullptr,
        _cmp,
        _dec,
        nullptr,
        _cpx,
        _sbc,
        nullptr,
        nullptr,
        _cpx,
        _sbc,
        _inc,
        nullptr,
        _inx,
        _sbc,
        _nop,
        nullptr,
        _cpx,
        _sbc,
        _inc,
        nullptr,
        _beq,
        _sbc,
        nullptr,
        nullptr,
        nullptr,
        _sbc,
        _inc,
        nullptr,
        _sed,
        _sbc,
        nullptr,
        nullptr,
        nullptr,
        _sbc,
        _inc,
        nullptr,
};

void (*get_insn(uint8_t opcode, bool should_succeed))(Operand*) {
	auto ret = opcode_table[opcode];
	if (!ret && should_succeed) {
		printf("Error! Invalid opcode %x\n", opcode);
		panic();
	}

	return ret;
}

bool is_branch(uint8_t opcode) {
	if (!(opcode & 0x0F)) {
		return opcode != 0xA0 && opcode != 0xC0 && opcode != 0xE0;
	} else {
		return opcode == 0x4C || opcode == 0x6C;
	}
}
