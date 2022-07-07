#include "instructions.h"

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

void _adc(std::shared_ptr<Operand> operand) {
	int carry = get_carry() ? 1 : 0;
	int result = operand->get_val() + acc + carry;
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), acc, result);
	set_carry(result & (~0xFF));
	acc = result & 0xFF;
}

void _and(std::shared_ptr<Operand> operand) {
	int result = operand->get_val() & acc;
	handle_arithmetic_flags(result);
	acc = result & 0xFF;
}

void _asl(std::shared_ptr<Operand> operand) {
	int result = ((acc & 0x7F) << operand->get_val()) | (acc & 0x80);
	handle_arithmetic_flags(result);
	set_carry(result & (~0xFF));
	acc = result & 0xFF;
	cycle_num += 2;
}

void _bcc(std::shared_ptr<Operand> operand) {
	if (!get_carry()) {
		program_counter = operand->get_val() - operand->get_insn_len();
		cycle_num++;
	}

	cycle_num += 2;
}

void _bcs(std::shared_ptr<Operand> operand) {
	if (get_carry()) {
		program_counter = operand->get_val() - operand->get_insn_len();
		cycle_num++;
	}

	cycle_num += 2;
}

void _beq(std::shared_ptr<Operand> operand) {
	if (get_zero()) {
		program_counter = operand->get_val() - operand->get_insn_len();
		cycle_num++;
	}

	cycle_num += 2;
}

void _bit(std::shared_ptr<Operand> operand) {
	set_negative(operand->get_val() & 0x40);
	set_overflow(operand->get_val() & 0x20);
	set_zero(operand->get_val() & acc);
}

void _bmi(std::shared_ptr<Operand> operand) {
	if (get_negative()) {
		program_counter = operand->get_val() - operand->get_insn_len();
		cycle_num++;
	}

	cycle_num += 2;
}

void _bne(std::shared_ptr<Operand> operand) {
	if (!get_zero()) {
		program_counter = operand->get_val() - operand->get_insn_len();
		cycle_num++;
	}

	cycle_num += 2;
}

void _bpl(std::shared_ptr<Operand> operand) {
	if (!get_negative()) {
		program_counter = operand->get_val() - operand->get_insn_len();
		cycle_num++;
	}

	cycle_num += 2;
}

void _brk(std::shared_ptr<Operand> operand) {
	int irq_vector = read_word(irq_vector_addr);
	if (!irq_vector) {
		should_execute = false;
		return;
	}

	if (!get_interrupt_enable())
		return;

	push_word(program_counter + operand->get_insn_len() + 1); // Leave extra space for a break mark
	push_byte(flags);
	program_counter = irq_vector - operand->get_insn_len();
	set_break(true);

	cycle_num += 7;
}

void _bvc(std::shared_ptr<Operand> operand) {
	if (!get_overflow()) {
		program_counter = operand->get_val() - operand->get_insn_len();
		cycle_num++;
	}

	cycle_num += 2;
}

void _bvs(std::shared_ptr<Operand> operand) {
	if (get_overflow()) {
		program_counter = operand->get_val() - operand->get_insn_len();
		cycle_num++;
	}

	cycle_num += 2;
}

void _clc(std::shared_ptr<Operand> operand) {
	set_carry(false);
	cycle_num += 2;
}

void _cld(std::shared_ptr<Operand> operand) {
	set_decimal(false);
	cycle_num += 2;
}

void _cli(std::shared_ptr<Operand> operand) {
	set_interrupt_enable(false);
	cycle_num += 2;
}

void _clv(std::shared_ptr<Operand> operand) {
	set_overflow(false);
	cycle_num += 2;
}

void _cmp(std::shared_ptr<Operand> operand) {
	int result = (acc - operand->get_val()) & 0xFF;
	handle_overflow(acc, operand->get_val(), result);
	handle_arithmetic_flags(result);
	set_carry(!(result & (~0xFF)));
}

void _cpx(std::shared_ptr<Operand> operand) {
	int result = (index_x - operand->get_val()) & 0xFF;
	handle_overflow(index_x, operand->get_val(), result);
	handle_arithmetic_flags(result);
	set_carry(!(result & (~0xFF)));
}

void _cpy(std::shared_ptr<Operand> operand) {
	int result = (index_y - operand->get_val()) & 0xFF;
	handle_overflow(index_y, operand->get_val(), result);
	handle_arithmetic_flags(result);
	set_carry(!(result & (~0xFF)));
}

void _dec(std::shared_ptr<Operand> operand) {
	int result = operand->get_val() - 1;
	handle_arithmetic_flags(result);
	operand->set_val(result & 0xFF);
	cycle_num += 2;
}

void _dex(std::shared_ptr<Operand> operand) {
	index_x--;
	handle_arithmetic_flags(index_x);
	cycle_num += 2;
}

void _dey(std::shared_ptr<Operand> operand) {
	index_y--;
	handle_arithmetic_flags(index_y);
	cycle_num += 2;
}

void _eor(std::shared_ptr<Operand> operand) {
	int result = operand->get_val() ^ acc;
	handle_arithmetic_flags(result);
	acc = result;
}

void _inc(std::shared_ptr<Operand> operand) {
	int result = operand->get_val() + 1;
	handle_arithmetic_flags(result);
	operand->set_val(result & 0xFF);
	cycle_num += 2;
}

void _inx(std::shared_ptr<Operand> operand) {
	index_x++;
	handle_arithmetic_flags(index_x);
	cycle_num += 2;
}

void _iny(std::shared_ptr<Operand> operand) {
	index_y++;
	handle_arithmetic_flags(index_y);
	cycle_num += 2;
}

void _jmp(std::shared_ptr<Operand> operand) {
	program_counter = operand->get_val() - operand->get_insn_len();
	cycle_num--; // Weird hack for this particular instruction.
}

void _jsr(std::shared_ptr<Operand> operand) {
	push_word(program_counter + operand->get_insn_len());
	program_counter = operand->get_val() - operand->get_insn_len();
	cycle_num += 2;
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
	int result = acc >> operand->get_val();
	handle_arithmetic_flags(result);
	set_carry(result & (~0xFF));
	acc = result & 0xFF;
	cycle_num += 2;
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
	push_byte(acc);
	cycle_num += 3;
}

void _php(std::shared_ptr<Operand> operand) {
	push_byte(flags);
	cycle_num += 3;
}

void _pla(std::shared_ptr<Operand> operand) {
	acc = pop_byte();
	handle_arithmetic_flags(acc);
	cycle_num += 4;
}

void _plp(std::shared_ptr<Operand> operand) {
	flags = pop_byte();
	cycle_num += 4;
}

void _rol(std::shared_ptr<Operand> operand) {
	int val = operand->get_val();
	int result = ((acc << val) | (acc >> (8-val))) & 0xFF;
	handle_arithmetic_flags(result);
	set_carry(result & (~0xFF));
	acc = result;
	cycle_num += 2;
}

void _ror(std::shared_ptr<Operand> operand) {
	int val = operand->get_val();
	int result = ((acc >> val) | (acc << (8-val))) & 0xFF;
	handle_arithmetic_flags(result);
	set_carry(result & (~0xFF));
	acc = result;
	cycle_num += 2;
}

void _rti(std::shared_ptr<Operand> operand) {
	flags = pop_byte();
	program_counter = pop_word() - operand->get_insn_len();
	cycle_num += 6;
}

void _rts(std::shared_ptr<Operand> operand) {
	program_counter = pop_word() - operand->get_insn_len();
	cycle_num += 6;
}

void _sbc(std::shared_ptr<Operand> operand) {
	int carry = get_carry() ? 0 : 1;
	int result = acc - operand->get_val() - carry;
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), acc, result);
	set_carry(!(result & (~0xFF)));
	acc = result & 0xFF;
}

void _sec(std::shared_ptr<Operand> operand) {
	set_carry(true);
	cycle_num += 2;
}

void _sed(std::shared_ptr<Operand> operand) {
	set_decimal(true);
	cycle_num += 2;
}

void _sei(std::shared_ptr<Operand> operand) {
	set_interrupt_enable(true);
	cycle_num += 2;
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
	index_x = acc;
	handle_arithmetic_flags(index_x);
	cycle_num += 2;
}

void _tay(std::shared_ptr<Operand> operand) {
	index_y = acc;
	handle_arithmetic_flags(index_y);
	cycle_num += 2;
}

void _tsx(std::shared_ptr<Operand> operand) {
	stack_pointer = index_x;
	handle_arithmetic_flags(stack_pointer);
	cycle_num += 2;
}

void _txa(std::shared_ptr<Operand> operand) {
	acc = index_x;
	handle_arithmetic_flags(acc);
	cycle_num += 2;
}

void _txs(std::shared_ptr<Operand> operand) {
	stack_pointer = index_x;
	handle_arithmetic_flags(stack_pointer);
	cycle_num += 2;
}

void _tya(std::shared_ptr<Operand> operand) {
	acc = index_y;
	handle_arithmetic_flags(acc);
	cycle_num += 2;
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

std::function<void(std::shared_ptr<Operand>)> get_insn(uint8_t opcode) {
	auto ret = opcode_table[opcode];
	if (!ret) {
		printf("Error! Invalid opcode %x\n", opcode);
		panic();
	}

	return ret;
}
