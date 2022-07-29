#include "instructions.h"

#include <stdio.h>

#include "cpu.h"
#include "memory.h"
#include "registers.h"

// Helper function for handling Zero and Negative flags. Note that we don't
// handle Carry or Overflow here because they are more complicated, and not all
// instructions support them.
void handle_arithmetic_flags(int result) {
  set_zero(!(result & 0xFF));
  set_negative(result & 0x80);
}

// Overflow is actually different from carry. It is defined as a change in sign
// that is *not* the intended result of the given operation. So for example,
// adding two positive numbers should never result in a negative number, but
// because our register is only 8-bit, we may overflow and get a negative
// anyway.
void handle_overflow(int val1, int val2, int result) {
  if ((val1 & 0x80) && (val2 & 0x80) && !(result & 0x80)) {
    set_overflow(true);
  } else if (!(val1 & 0x80) && !(val2 & 0x80) && (result & 0x80)) {
    set_overflow(true);
  } else {
    set_overflow(false);
  }
}

// Note that we try to increment the cycle counter before evaluating the operand
// to accurately read timers

///////////////////////////
// Arithmetic Operations //
///////////////////////////

// ADd with Carry. Adds the operand to the accumulator and also adds 1 to that
// result if the carry flag is set. Effects Negative, Overflow, Carry, and Zero
void _adc(std::shared_ptr<Operand> operand) {
  int carry = get_carry() ? 1 : 0;
  int result;
  if (!get_decimal()) {
    // Normal operation
    result = operand->get_val() + acc + carry;
    set_carry(result & (~0xFF));
  } else {
    // Binary coded decimal mode operation. BCD can really only represent
    // numbers 00-99. Each nibble represents a digit 0-9. Carry and Zero are set
    // in expected ways. Negative and Overflow are also technically set, but
    // their meaning is ambiguous and confusing in BCD mode, and are not often
    // used.
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

// Note that neither increment nor decrement affect Carry or Overflow

// DECrement memory
// Effects Negative and Zero
void _dec(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  int result = operand->get_val() - 1;
  handle_arithmetic_flags(result);
  operand->set_val(result & 0xFF);
}

// DEcrement X
// Effects Negative and Zero
void _dex(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  index_x--;
  handle_arithmetic_flags(index_x);
}

// DEcrement Y
// Effects Negative and Zero
void _dey(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  index_y--;
  handle_arithmetic_flags(index_y);
}

// INCrement memory
// Effects Negative and Zero
void _inc(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  int result = operand->get_val() + 1;
  handle_arithmetic_flags(result);
  operand->set_val(result & 0xFF);
}

// INcrement X
// Effects Negative and Zero
void _inx(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  index_x++;
  handle_arithmetic_flags(index_x);
}

// INcrement Y
// Effects Negative and Zero
void _iny(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  index_y++;
  handle_arithmetic_flags(index_y);
}

// SuBtract with borrow (mnemonic is misleading)
// Borrow is a pseudoflag is the inverse of carry.
// If borrow is set, we subtract an extra 1 from our result.
// If our result is negative, we clear the carry flag, which is unintuitive.
// Effects Negative, Zero, Carry, and Overflow
void _sbc(std::shared_ptr<Operand> operand) {
  int carry = get_carry() ? 0 : 1;
  int result;
  if (!get_decimal()) {
    result = acc - operand->get_val() - carry;
    set_carry(!(result & (~0xFF)));
  } else {
    // SBC also supports a Binary Coded Decimal mode.
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

//////////////////////////////
// Bit twiddling operations //
//////////////////////////////

// Bitwise logical AND
// Effects Negative and Zero
void _and(std::shared_ptr<Operand> operand) {
  int result = operand->get_val() & acc;
  handle_arithmetic_flags(result);
  acc = result & 0xFF;
}

// Arithmetic Shift Left. Not actually different from a logical shift left.
// Effects Negative, Zero, and Carry
void _asl(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  int result = (int)acc << operand->get_val();
  handle_arithmetic_flags(result);
  set_carry(result & (~0xFF));
  acc = result & 0xFF;
}

// Exclusive OR
// Effects Negative and Carry
void _eor(std::shared_ptr<Operand> operand) {
  int result = operand->get_val() ^ acc;
  handle_arithmetic_flags(result);
  acc = result;
}

// Logical Shift Right one bit.
// This instruction along with the rotate instructions are divided into
// accumulator and memory variants Least significant bit is shifted into
// accumulator
uint8_t right_shift(uint8_t input) {
  cycle_num += 2;

  set_carry(input & 0x01);
  input >>= 1;
  handle_arithmetic_flags(input);

  return input;
}
void _lsr_acc(std::shared_ptr<Operand> operand) { acc = right_shift(acc); }

void _lsr_memory(std::shared_ptr<Operand> operand) {
  operand->set_val(operand->get_val());
}

void _ora(std::shared_ptr<Operand> operand) {
  int result = acc | operand->get_val();
  handle_arithmetic_flags(result);
  acc = result & 0xFF;
}

uint8_t rotate_left(uint8_t input) {
  cycle_num += 2;

  int result = input << 1 | get_carry();
  handle_arithmetic_flags(result);
  set_carry(input & 0x80);

  return result & 0xFF;
}

void _rol_acc(std::shared_ptr<Operand> operand) { acc = rotate_left(acc); }

void _rol_memory(std::shared_ptr<Operand> operand) {
  operand->set_val(rotate_left(operand->get_val()));
}

uint8_t rotate_right(uint8_t input) {
  cycle_num += 2;

  int result = input >> 1 | (int)get_carry() << 7;
  handle_arithmetic_flags(result);
  set_carry(input & 0x01);

  return result & 0xFF;
}

void _ror_acc(std::shared_ptr<Operand> operand) { acc = rotate_right(acc); }

void _ror_memory(std::shared_ptr<Operand> operand) {
  operand->set_val(rotate_right(operand->get_val()));
}

///////////////////////////////
// Control Flow Instructions //
///////////////////////////////

// Note that all branch instructions add a cycle penalty for taking the branch.
// There's no branch predictor on the 6502

// Branch if Carry Clear
void _bcc(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  if (!get_carry()) {
    cycle_num++;

    uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
    if ((new_program_counter & (~(PAGE_SIZE - 1))) !=
        (program_counter & (~(PAGE_SIZE - 1))))
      cycle_num++;
    program_counter = new_program_counter;
  }
}

void _bcs(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  if (get_carry()) {
    cycle_num++;

    uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
    if ((new_program_counter & (~(PAGE_SIZE - 1))) !=
        (program_counter & (~(PAGE_SIZE - 1))))
      cycle_num++;
    program_counter = new_program_counter;
  }
}

void _beq(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  if (get_zero()) {
    cycle_num++;

    uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
    if ((new_program_counter & (~(PAGE_SIZE - 1))) !=
        (program_counter & (~(PAGE_SIZE - 1))))
      cycle_num++;
    program_counter = new_program_counter;
  }
}

void _bmi(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  if (get_negative()) {
    cycle_num++;

    uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
    if ((new_program_counter & (~(PAGE_SIZE - 1))) !=
        (program_counter & (~(PAGE_SIZE - 1))))
      cycle_num++;
    program_counter = new_program_counter;
  }
}

void _bne(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  if (!get_zero()) {
    cycle_num++;

    uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
    if ((new_program_counter & (~(PAGE_SIZE - 1))) !=
        (program_counter & (~(PAGE_SIZE - 1))))
      cycle_num++;
    program_counter = new_program_counter;
  }
}

void _bpl(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  if (!get_negative()) {
    cycle_num++;

    uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
    if ((new_program_counter & (~(PAGE_SIZE - 1))) !=
        (program_counter & (~(PAGE_SIZE - 1))))
      cycle_num++;
    program_counter = new_program_counter;
  }
}

void _bvc(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  if (!get_overflow()) {
    cycle_num++;

    uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
    if ((new_program_counter & (~(PAGE_SIZE - 1))) !=
        (program_counter & (~(PAGE_SIZE - 1))))
      cycle_num++;
    program_counter = new_program_counter;
  }
}

void _bvs(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  if (get_overflow()) {
    cycle_num++;

    uint16_t new_program_counter = operand->get_val() - operand->get_insn_len();
    if ((new_program_counter & (~(PAGE_SIZE - 1))) !=
        (program_counter & (~(PAGE_SIZE - 1))))
      cycle_num++;
    program_counter = new_program_counter;
  }
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

void _rti(std::shared_ptr<Operand> operand) {
  cycle_num += 6;

  flags = pop_byte();
  program_counter = pop_word() - operand->get_insn_len();
}

void _rts(std::shared_ptr<Operand> operand) {
  cycle_num += 6;

  program_counter = pop_word() - operand->get_insn_len() + 1;
}

/////////////////////////////
// Comparison Instructions //
/////////////////////////////

void _bit(std::shared_ptr<Operand> operand) {
  set_negative(operand->get_val() & 0x80);
  set_overflow(operand->get_val() & 0x40);
  set_zero(!(operand->get_val() & acc));
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

////////////////////////////////
// Data Transfer Instructions //
////////////////////////////////

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

void _sta(std::shared_ptr<Operand> operand) { operand->set_val(acc); }

void _stx(std::shared_ptr<Operand> operand) { operand->set_val(index_x); }

void _sty(std::shared_ptr<Operand> operand) { operand->set_val(index_y); }

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

////////////////////////////////
// Miscellaneous Instructions //
////////////////////////////////

void _brk(std::shared_ptr<Operand> operand) {
  int irq_vector = read_word(irq_vector_addr);
  if (!irq_vector) {
    should_execute = false;
    return;
  }

  if (!get_interrupt_enable())
    return;

  cycle_num += 7;

  push_word(program_counter + operand->get_insn_len() +
            1); // Leave extra space for a break mark
  push_byte(flags);
  program_counter = irq_vector - operand->get_insn_len();
  set_break(true);
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

void _nop(std::shared_ptr<Operand> operand) { cycle_num += 2; }

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

std::function<void(std::shared_ptr<Operand>)> opcode_table[256] = {
    _brk,    _ora,    nullptr,  nullptr, nullptr, _ora, _asl,        nullptr,
    _php,    _ora,    _asl,     nullptr, nullptr, _ora, _asl,        nullptr,
    _bpl,    _ora,    nullptr,  nullptr, nullptr, _ora, _asl,        nullptr,
    _clc,    _ora,    nullptr,  nullptr, nullptr, _ora, _asl,        nullptr,
    _jsr,    _and,    nullptr,  nullptr, _bit,    _and, _rol_memory, nullptr,
    _plp,    _and,    _rol_acc, nullptr, _bit,    _and, _rol_memory, nullptr,
    _bmi,    _and,    nullptr,  nullptr, nullptr, _and, _rol_memory, nullptr,
    _sec,    _and,    nullptr,  nullptr, nullptr, _and, _rol_memory, nullptr,
    _rti,    _eor,    nullptr,  nullptr, nullptr, _eor, _lsr_memory, nullptr,
    _pha,    _eor,    _lsr_acc, nullptr, _jmp,    _eor, _lsr_memory, nullptr,
    _bvc,    _eor,    nullptr,  nullptr, nullptr, _eor, _lsr_memory, nullptr,
    _cli,    _eor,    nullptr,  nullptr, nullptr, _eor, _lsr_memory, nullptr,
    _rts,    _adc,    nullptr,  nullptr, nullptr, _adc, _ror_memory, nullptr,
    _pla,    _adc,    _ror_acc, nullptr, _jmp,    _adc, _ror_memory, nullptr,
    _bvs,    _adc,    nullptr,  nullptr, nullptr, _adc, _ror_memory, nullptr,
    _sei,    _adc,    nullptr,  nullptr, nullptr, _adc, _ror_memory, nullptr,
    nullptr, _sta,    nullptr,  nullptr, _sty,    _sta, _stx,        nullptr,
    _dey,    nullptr, _txa,     nullptr, _sty,    _sta, _stx,        nullptr,
    _bcc,    _sta,    nullptr,  nullptr, _sty,    _sta, _stx,        nullptr,
    _tya,    _sta,    _txs,     nullptr, nullptr, _sta, nullptr,     nullptr,
    _ldy,    _lda,    _ldx,     nullptr, _ldy,    _lda, _ldx,        nullptr,
    _tay,    _lda,    _tax,     nullptr, _ldy,    _lda, _ldx,        nullptr,
    _bcs,    _lda,    nullptr,  nullptr, _ldy,    _lda, _ldx,        nullptr,
    _clv,    _lda,    _tsx,     nullptr, _ldy,    _lda, _ldx,        nullptr,
    _cpy,    _cmp,    nullptr,  nullptr, _cpy,    _cmp, _dec,        nullptr,
    _iny,    _cmp,    _dex,     nullptr, _cpy,    _cmp, _dec,        nullptr,
    _bne,    _cmp,    nullptr,  nullptr, nullptr, _cmp, _dec,        nullptr,
    _cld,    _cmp,    nullptr,  nullptr, nullptr, _cmp, _dec,        nullptr,
    _cpx,    _sbc,    nullptr,  nullptr, _cpx,    _sbc, _inc,        nullptr,
    _inx,    _sbc,    _nop,     nullptr, _cpx,    _sbc, _inc,        nullptr,
    _beq,    _sbc,    nullptr,  nullptr, nullptr, _sbc, _inc,        nullptr,
    _sed,    _sbc,    nullptr,  nullptr, nullptr, _sbc, _inc,        nullptr,
};

std::function<void(std::shared_ptr<Operand>)> get_insn(uint8_t opcode,
                                                       bool should_succeed) {
  auto ret = opcode_table[opcode];
  if (!ret && should_succeed) {
    printf("Error! Invalid opcode %x\n", opcode);
    panic();
  }

  return ret;
}

std::string mnemonics[256] = {
    "BRK", "ORA", "",    "", "",    "ORA", "ASL", "", "PHP", "ORA", "ASL", "",
    "",    "ORA", "ASL", "", "BPL", "ORA", "",    "", "",    "ORA", "ASL", "",
    "CLC", "ORA", "",    "", "",    "ORA", "ASL", "", "JSR", "AND", "",    "",
    "BIT", "AND", "ROL", "", "PLP", "AND", "ROL", "", "BIT", "AND", "ROL", "",
    "BMI", "AND", "",    "", "",    "AND", "ROL", "", "SEC", "AND", "",    "",
    "",    "AND", "ROL", "", "RTI", "EOR", "",    "", "",    "EOR", "LSR", "",
    "PHA", "EOR", "LSR", "", "JMP", "EOR", "LSR", "", "BVC", "EOR", "",    "",
    "",    "EOR", "LSR", "", "CLI", "EOR", "",    "", "",    "EOR", "LSR", "",
    "RTS", "ADC", "",    "", "",    "ADC", "ROR", "", "PLA", "ADC", "ROR", "",
    "JMP", "ADC", "ROR", "", "BVS", "ADC", "",    "", "",    "ADC", "ROR", "",
    "SEI", "ADC", "",    "", "",    "ADC", "ROR", "", "",    "STA", "",    "",
    "STY", "STA", "STX", "", "DEY", "",    "TXA", "", "STY", "STA", "STX", "",
    "BCC", "STA", "",    "", "STY", "STA", "STX", "", "TYA", "STA", "TXS", "",
    "",    "STA", "",    "", "LDY", "LDA", "LDX", "", "LDY", "LDA", "LDX", "",
    "TAY", "LDA", "TAX", "", "LDY", "LDA", "LDX", "", "BCS", "LDA", "",    "",
    "LDY", "LDA", "LDX", "", "CLV", "LDA", "TSX", "", "LDY", "LDA", "LDX", "",
    "CPY", "CMP", "",    "", "CPY", "CMP", "DEC", "", "INY", "CMP", "DEX", "",
    "CPY", "CMP", "DEC", "", "BNE", "CMP", "",    "", "",    "CMP", "DEC", "",
    "CLD", "CMP", "",    "", "",    "CMP", "DEC", "", "CPX", "SBC", "",    "",
    "CPX", "SBC", "INC", "", "INX", "SBC", "NOP", "", "CPX", "SBC", "INC", "",
    "BEQ", "SBC", "",    "", "",    "SBC", "INC", "", "SED", "SBC", "",    "",
    "",    "SBC", "INC", "",
};

std::string get_mnemonic(uint8_t opcode) { return mnemonics[opcode]; }
