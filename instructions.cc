#include "instructions.h"

#include "registers.h"
#include "memory.h"
#include "cpu.h"

void handle_arithmetic_flags(int result) {
	set_zero(!(result & 0xFF));
	set_carry(result & (~0xFF));
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
	acc = result & 0xFF;
}

void _bcc(std::shared_ptr<Operand> operand) {
	if (!get_carry())
		program_counter = operand->get_val() - operand->get_insn_len();
}

void _bcs(std::shared_ptr<Operand> operand) {
	if (get_carry())
		program_counter = operand->get_val() - operand->get_insn_len();
}

void _beq(std::shared_ptr<Operand> operand) {
	if (get_zero())
		program_counter = operand->get_val() - operand->get_insn_len();
}

void _bit(std::shared_ptr<Operand> operand) {
	set_negative(operand->get_val() & 0x40);
	set_overflow(operand->get_val() & 0x20);
	set_zero(operand->get_val() & acc);
}

void _bmi(std::shared_ptr<Operand> operand) {
	if (get_negative())
		program_counter = operand->get_val() - operand->get_insn_len();
}

void _bne(std::shared_ptr<Operand> operand) {
	if (!get_zero())
		program_counter = operand->get_val() - operand->get_insn_len();
}

void _bpl(std::shared_ptr<Operand> operand) {
	if (!get_negative())
		program_counter = operand->get_val() - operand->get_insn_len();
}

void _brk(std::shared_ptr<Operand> operand) {
	int irq_vector = read_word(IRQ_VECTOR);
	if (!irq_vector) {
		should_execute = false;
		return;
	}

	if (!get_interrupt_enable())
		return;

	stack_pointer -= 2;
	write_word(STACK_BOTTOM + stack_pointer, program_counter + operand->get_insn_len());
	stack_pointer--;
	write_byte(STACK_BOTTOM + stack_pointer, flags);
	program_counter = irq_vector - operand->get_insn_len();
}

void _bvc(std::shared_ptr<Operand> operand) {
	if (!get_overflow())
		program_counter = operand->get_val() - operand->get_insn_len();
}

void _bvs(std::shared_ptr<Operand> operand) {
	if (get_overflow())
		program_counter = operand->get_val() - operand->get_insn_len();
}

void _clc(std::shared_ptr<Operand> operand) {
	set_carry(false);
}

void _cld(std::shared_ptr<Operand> operand) {
	set_decimal(false);
}

void _cli(std::shared_ptr<Operand> operand) {
	set_interrupt_enable(false);
}

void _clv(std::shared_ptr<Operand> operand) {
	set_overflow(false);
}

void _cmp(std::shared_ptr<Operand> operand) {
	int result = (acc - operand->get_val()) & 0xFF;
	handle_overflow(acc, operand->get_val(), result);
	handle_arithmetic_flags(result);
}

void _cpx(std::shared_ptr<Operand> operand) {
	int result = (index_x - operand->get_val()) & 0xFF;
	handle_overflow(index_x, operand->get_val(), result);
	handle_arithmetic_flags(result);
}

void _cpy(std::shared_ptr<Operand> operand) {
	int result = (index_y - operand->get_val()) & 0xFF;
	handle_overflow(index_y, operand->get_val(), result);
	handle_arithmetic_flags(result);
}

void _dec(std::shared_ptr<Operand> operand) {
	int result = operand->get_val() - 1;
	handle_arithmetic_flags(result);
	operand->set_val(result & 0xFF);
}

void _dex(std::shared_ptr<Operand> operand) {
	index_x--;
	handle_arithmetic_flags(index_x);
}

void _dey(std::shared_ptr<Operand> operand) {
	index_y--;
	handle_arithmetic_flags(index_y);
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
}

void _inx(std::shared_ptr<Operand> operand) {
	index_x++;
	handle_arithmetic_flags(index_x);
}

void _iny(std::shared_ptr<Operand> operand) {
	index_y++;
	handle_arithmetic_flags(index_y);
}

void _jmp(std::shared_ptr<Operand> operand) {
	program_counter = operand->get_val() - operand->get_insn_len();
}

void _jsr(std::shared_ptr<Operand> operand) {
	stack_pointer -= 2;
	write_word(STACK_BOTTOM + stack_pointer, program_counter + operand->get_insn_len());
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
	int result = acc >> operand->get_val();
	handle_arithmetic_flags(result);
	acc = result & 0xFF;
}

void _nop(std::shared_ptr<Operand> operand) {}

void _ora(std::shared_ptr<Operand> operand) {
	int result = acc | operand->get_val();
	handle_arithmetic_flags(result);
	acc = result & 0xFF;
}

void _pha(std::shared_ptr<Operand> operand) {
	stack_pointer--;
	write_byte(STACK_BOTTOM + stack_pointer, acc);
}

void _php(std::shared_ptr<Operand> operand) {
	stack_pointer--;
	write_byte(STACK_BOTTOM + stack_pointer, flags);
}

void _pla(std::shared_ptr<Operand> operand) {
	acc = read_byte(STACK_BOTTOM + stack_pointer);
	stack_pointer++;
	handle_arithmetic_flags(acc);
}

void _plp(std::shared_ptr<Operand> operand) {
	flags = read_byte(STACK_BOTTOM + stack_pointer);
	stack_pointer++;
}

void _rol(std::shared_ptr<Operand> operand) {
	int val = operand->get_val();
	int result = ((acc << val) | (acc >> (8-val))) & 0xFF;
	handle_arithmetic_flags(result);
	acc = result;
}

void _ror(std::shared_ptr<Operand> operand) {
	int val = operand->get_val();
	int result = ((acc >> val) | (acc << (8-val))) & 0xFF;
	handle_arithmetic_flags(result);
	acc = result;
}

void _rti(std::shared_ptr<Operand> operand) {
	flags = read_byte(STACK_BOTTOM + stack_pointer);
	stack_pointer++;
	program_counter = read_word(STACK_BOTTOM + stack_pointer) - operand->get_insn_len();
	stack_pointer += 2;
}

void _rts(std::shared_ptr<Operand> operand) {
	program_counter = read_word(STACK_BOTTOM + stack_pointer) - operand->get_insn_len();
	stack_pointer += 2;
}

void _sbc(std::shared_ptr<Operand> operand) {
	int carry = get_carry() ? 1 : 0;
	int result = acc - operand->get_val() - carry;
	handle_arithmetic_flags(result);
	handle_overflow(operand->get_val(), acc, result);
	acc = result & 0xFF;
}

void _sec(std::shared_ptr<Operand> operand) {
	set_carry(true);
}

void _sed(std::shared_ptr<Operand> operand) {
	set_decimal(true);
}

void _sei(std::shared_ptr<Operand> operand) {
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
	index_x = acc;
	handle_arithmetic_flags(index_x);
}

void _tay(std::shared_ptr<Operand> operand) {
	index_y = acc;
	handle_arithmetic_flags(index_y);
}

void _tsx(std::shared_ptr<Operand> operand) {
	stack_pointer = index_x;
	handle_arithmetic_flags(stack_pointer);
}

void _txa(std::shared_ptr<Operand> operand) {
	acc = index_x;
	handle_arithmetic_flags(acc);
}

void _txs(std::shared_ptr<Operand> operand) {
	stack_pointer = index_x;
	handle_arithmetic_flags(stack_pointer);
}

void _tya(std::shared_ptr<Operand> operand) {
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

std::function<void(std::shared_ptr<Operand>)> get_insn(uint8_t opcode) {
	auto ret = opcode_table[opcode];
	if (!ret) {
		printf("Error! Invalid opcode %x\n", opcode);
		panic();
	}

	return ret;
}
