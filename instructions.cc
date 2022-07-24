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

void _adc(std::shared_ptr<Operand> operand) {
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

void _and(std::shared_ptr<Operand> operand) {
	int result = operand->get_val() & acc;
	handle_arithmetic_flags(result);
	acc = result & 0xFF;
}

void _asl(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	int result = (int)acc << operand->get_val();
	handle_arithmetic_flags(result);
	set_carry(result & (~0xFF));
	acc = result & 0xFF;
}

void _bcc(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	if (!get_carry()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

void _bcs(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	if (get_carry()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

void _beq(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	if (get_zero()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

void _bit(std::shared_ptr<Operand> operand) {
	set_negative(operand->get_val() & 0x40);
	set_overflow(operand->get_val() & 0x20);
	set_zero(operand->get_val() & acc);
}

void _bmi(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	if (get_negative()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

void _bne(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	if (!get_zero()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

void _bpl(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	if (!get_negative()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

void _brk(std::shared_ptr<Operand> operand) {
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

void _bvc(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	if (!get_overflow()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

void _bvs(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	if (get_overflow()) {
		cycle_num++;

		uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
		if ((new_program_counter & (~(PAGE_SIZE-1))) != (program_counter & (~(PAGE_SIZE-1))))
			cycle_num++;
		program_counter = new_program_counter;
	}
}

void _clc(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	set_carry(false);
}

void _cld(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	set_decimal(false);
}

void _cli(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	set_interrupt_enable(false);
}

void _clv(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	set_overflow(false);
}

void _cmp(std::shared_ptr<Operand> operand) {
	int result = acc - operand->get_val();
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), acc, result);
	set_carry(!(result & (~0xFF)));
}

void _cpx(std::shared_ptr<Operand> operand) {
	int result = index_x - operand->get_val();
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), index_x, result);
	set_carry(!(result & (~0xFF)));
}

void _cpy(std::shared_ptr<Operand> operand) {
	int result = index_y - operand->get_val();
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), index_y, result);
	set_carry(!(result & (~0xFF)));
}

void _dec(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	int result = operand->get_val() - 1;
	handle_arithmetic_flags(result);
	operand->set_val(result & 0xFF);
}

void _dex(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	index_x--;
	handle_arithmetic_flags(index_x);
}

void _dey(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	index_y--;
	handle_arithmetic_flags(index_y);
}

void _eor(std::shared_ptr<Operand> operand) {
	int result = operand->get_val() ^ acc;
	handle_arithmetic_flags(result);
	acc = result;
}

void _inc(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	int result = operand->get_val() + 1;
	handle_arithmetic_flags(result);
	operand->set_val(result & 0xFF);
}

void _inx(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	index_x++;
	handle_arithmetic_flags(index_x);
}

void _iny(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	index_y++;
	handle_arithmetic_flags(index_y);
}

void _jmp(std::shared_ptr<Operand> operand) {
	cycle_num--; // Weird hack for this particular instruction.

	program_counter = operand->get_val() - operand->get_insn_len();
}

void _jsr(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	// A quirk in the 6502 stores return address - 1 for JSR.
	push_word(program_counter + operand->get_insn_len() - 1);
	program_counter = operand->get_val() - operand->get_insn_len();
}

void _lda(std::shared_ptr<Operand> operand) {
	acc = operand->get_val();
	handle_arithmetic_flags(acc);
}

void _ldx(std::shared_ptr<Operand> operand) {
	index_x = operand->get_val();
	handle_arithmetic_flags(index_x);
}

void _ldy(std::shared_ptr<Operand> operand) {
	index_y = operand->get_val();
	handle_arithmetic_flags(index_y);
}

void _lsr(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	int result = acc >> operand->get_val();
	handle_arithmetic_flags(result);
	set_carry(((uint16_t)acc << (8-operand->get_val())) & 0xFF);
	acc = result & 0xFF;
}

void _nop(std::shared_ptr<Operand> operand) {
	cycle_num += 2;
}

void _ora(std::shared_ptr<Operand> operand) {
	int result = acc | operand->get_val();
	handle_arithmetic_flags(result);
	acc = result & 0xFF;
}

void _pha(std::shared_ptr<Operand> operand) {
	cycle_num += 3;

	push_byte(acc);
}

void _php(std::shared_ptr<Operand> operand) {
	cycle_num += 3;

	push_byte(flags);
}

void _pla(std::shared_ptr<Operand> operand) {
	cycle_num += 4;

	acc = pop_byte();
	handle_arithmetic_flags(acc);
}

void _plp(std::shared_ptr<Operand> operand) {
	cycle_num += 4;

	flags = pop_byte();
}

void _rol(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	int result = ((acc << operand->get_val()) | (acc >> (8-operand->get_val()))) & 0xFF;
	handle_arithmetic_flags(result);
	set_carry(result & (~0xFF));
	acc = result;
}

void _ror(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	int result = ((acc >> operand->get_val()) | (acc << (8-operand->get_val()))) & 0xFF;
	handle_arithmetic_flags(result);
	set_carry(result & (~0xFF));
	acc = result;
}

void _rti(std::shared_ptr<Operand> operand) {
	cycle_num += 6;

	flags = pop_byte();
	program_counter = pop_word() - operand->get_insn_len();
}

void _rts(std::shared_ptr<Operand> operand) {
	cycle_num += 6;

	program_counter = pop_word() - operand->get_insn_len() + 1;
}

void _sbc(std::shared_ptr<Operand> operand) {
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

void _sec(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	set_carry(true);
}

void _sed(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	set_decimal(true);
}

void _sei(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	set_interrupt_enable(true);
}

void _sta(std::shared_ptr<Operand> operand) {
	operand->set_val(acc);
}

void _stx(std::shared_ptr<Operand> operand) {
	operand->set_val(index_x);
}

void _sty(std::shared_ptr<Operand> operand) {
	operand->set_val(index_y);
}

void _tax(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	index_x = acc;
	handle_arithmetic_flags(index_x);
}

void _tay(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	index_y = acc;
	handle_arithmetic_flags(index_y);
}

void _tsx(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	stack_pointer = index_x;
	handle_arithmetic_flags(stack_pointer);
}

void _txa(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	acc = index_x;
	handle_arithmetic_flags(acc);
}

void _txs(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	stack_pointer = index_x;
	handle_arithmetic_flags(stack_pointer);
}

void _tya(std::shared_ptr<Operand> operand) {
	cycle_num += 2;

	acc = index_y;
	handle_arithmetic_flags(acc);
}

std::function<void(std::shared_ptr<Operand>)> opcode_table[256] = {
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
        _rol,
        nullptr,
        _plp,
        _and,
        _rol,
        nullptr,
        _bit,
        _and,
        _rol,
        nullptr,
        _bmi,
        _and,
        nullptr,
        nullptr,
        nullptr,
        _and,
        _rol,
        nullptr,
        _sec,
        _and,
        nullptr,
        nullptr,
        nullptr,
        _and,
        _rol,
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
        _ror,
        nullptr,
        _pla,
        _adc,
        _ror,
        nullptr,
        _jmp,
        _adc,
        _ror,
        nullptr,
        _bvs,
        _adc,
        nullptr,
        nullptr,
        nullptr,
        _adc,
        _ror,
        nullptr,
        _sei,
        _adc,
        nullptr,
        nullptr,
        nullptr,
        _adc,
        _ror,
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

std::function<void(std::shared_ptr<Operand>)> get_insn(uint8_t opcode, bool should_succeed) {
	auto ret = opcode_table[opcode];
	if (!ret && should_succeed) {
		printf("Error! Invalid opcode %x\n", opcode);
		panic();
	}

	return ret;
}
