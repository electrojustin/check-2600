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
  handle_overflow((-1 * operand->get_val()) & 0xFF, acc, result);
  acc = result & 0xFF;
}

//////////////////////////////
// Bit twiddling operations //
//////////////////////////////

// Note that all shift and rotate operations are divided into accumulator and
// memory variants. Most instructions affect the same registers no matter what
// the opcode type is, but these shift/rotate instructions are an except since
// the operand type can either be accumulator memory. Rather than expose the
// implementation details of the operands to this class, we work around the
// issue by just splitting the instructions into two related types.

// Bitwise logical AND
// Effects Negative and Zero
void _and(std::shared_ptr<Operand> operand) {
  int result = operand->get_val() & acc;
  handle_arithmetic_flags(result);
  acc = result & 0xFF;
}

// Arithmetic Shift Left. Not actually different from a logical shift left.
// Most significant bit is shifted into Carry register.
// Effects Negative, Zero, and Carry
uint8_t left_shift(uint8_t input) {
  cycle_num += 2;

  set_carry(input & 0x80);
  input <<= 1;
  handle_arithmetic_flags(input);

  return input;
}

void _asl_acc(std::shared_ptr<Operand> operand) { acc = left_shift(acc); }

void _asl_memory(std::shared_ptr<Operand> operand) {
  operand->set_val(left_shift(operand->get_val()));
}

// Exclusive OR with accumulator
// Effects Negative and Carry
void _eor(std::shared_ptr<Operand> operand) {
  int result = operand->get_val() ^ acc;
  handle_arithmetic_flags(result);
  acc = result;
}

// Logical Shift Right one bit.
// Least significant bit is shifted into Carry register.
// Also clears Negative and effects Zero
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

// Bitwise inclusive OR with Accumulator
// Effects Negative and Zero
void _ora(std::shared_ptr<Operand> operand) {
  int result = acc | operand->get_val();
  handle_arithmetic_flags(result);
  acc = result & 0xFF;
}

// ROtate Left.
// Shifts Carry into least significant bit and most significant bit into Carry
// Also effects Negative and Zero
uint8_t rotate_left(uint8_t input) {
  cycle_num += 2;

  bool new_carry = input & 0x80;

  input <<= 1;
  input |= get_carry();
  handle_arithmetic_flags(input);
  set_carry(new_carry);

  return input;
}

void _rol_acc(std::shared_ptr<Operand> operand) { acc = rotate_left(acc); }

void _rol_memory(std::shared_ptr<Operand> operand) {
  operand->set_val(rotate_left(operand->get_val()));
}

// ROtate Right.
// Shifts Carry into most significant bit and least significant bit into Carry
// Also effects Negative and Zero
uint8_t rotate_right(uint8_t input) {
  cycle_num += 2;

  bool new_carry = input & 0x01;

  input >>= 1;
  input |= (int)get_carry() << 7;
  handle_arithmetic_flags(input);
  set_carry(new_carry);

  return input;
}

void _ror_acc(std::shared_ptr<Operand> operand) { acc = rotate_right(acc); }

void _ror_memory(std::shared_ptr<Operand> operand) {
  operand->set_val(rotate_right(operand->get_val()));
}

///////////////////////////////
// Control Flow Instructions //
///////////////////////////////

// Note that all branch instructions add a cycle penalty for taking the branch.
// There's also a penalty if the branch is in a different page.

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

// Branch if Carry Set
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

// Branch if zero set (misleading mnemonic)
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

// Branch if negative set (misleading mnemonic)
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

// Branch if zero clear (misleading mnemonic)
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

// Branch if negative clear (misleading mnemonic)
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

// Branch if oVerflow Clear
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

// Branch if oVerflow Set
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

// Unconditional JuMP
void _jmp(std::shared_ptr<Operand> operand) {
  // Most cycle numbers follow a pretty predictable pattern based on the operand
  // type. This particular instruction doesn't, so we work around that with this
  // decrement.
  cycle_num--;

  program_counter = operand->get_val() - operand->get_insn_len();
}

// Jump to SubRoutine
// This is similar to the x86 "call" instruction. We push the return address
// onto the stack.
void _jsr(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  // A quirk in the 6502 stores return address - 1 for JSR.
  push_word(program_counter + operand->get_insn_len() - 1);
  program_counter = operand->get_val() - operand->get_insn_len();
}

// ReTurn from Interrupt
// Pops 1 byte into the status register, then 2 bytes into the program counter.
// The Atari 2600 doesn't really have hardware interrupts, but it does
// technically have software interrupts, so we include this just in case.
void _rti(std::shared_ptr<Operand> operand) {
  cycle_num += 6;

  flags = pop_byte();
  program_counter = pop_word() - operand->get_insn_len();
}

// ReTurn from Subroutine
// Pops 2 bytes into the program counter
void _rts(std::shared_ptr<Operand> operand) {
  cycle_num += 6;

  program_counter = pop_word() - operand->get_insn_len() + 1;
}

/////////////////////////////
// Comparison Instructions //
/////////////////////////////

// BIT test.
// Bit 7 of the operand is transferred into the Negative flag, and Bit 6 is
// transferred into the Overflow flag. Then the operand and the accumulator are
// bitwise AND'd together, and the Zero flag is set accordingly.
void _bit(std::shared_ptr<Operand> operand) {
  set_negative(operand->get_val() & 0x80);
  set_overflow(operand->get_val() & 0x40);
  set_zero(!(operand->get_val() & acc));
}

// CoMPare.
// Subtracts operand from accumulator, setting the flags appropriately, but then
// discard the results. Note that we don't handle overflow for CMP, unlike actual SBC.
void _cmp(std::shared_ptr<Operand> operand) {
  int result = acc - operand->get_val();
  handle_arithmetic_flags(result);
  set_carry(!(result & (~0xFF)));
}

// ComPare X
// Like CMP, but for the X register instead of the accumulator.
void _cpx(std::shared_ptr<Operand> operand) {
  int result = index_x - operand->get_val();
  handle_arithmetic_flags(result);
  set_carry(!(result & (~0xFF)));
}

// ComPare Y
// Like CMP, but for the Y register instead of the accumulator.
void _cpy(std::shared_ptr<Operand> operand) {
  int result = index_y - operand->get_val();
  handle_arithmetic_flags(result);
  set_carry(!(result & (~0xFF)));
}

////////////////////////////////
// Data Transfer Instructions //
////////////////////////////////

// LoaD Accumulator
// Sets the accumulator equal to the value of the operand.
// Effects Negative and Zero
void _lda(std::shared_ptr<Operand> operand) {
  acc = operand->get_val();
  handle_arithmetic_flags(acc);
}

// LoaD X
// Like LDA, but for the X register
// Effects Negative and Zero
void _ldx(std::shared_ptr<Operand> operand) {
  index_x = operand->get_val();
  handle_arithmetic_flags(index_x);
}

// LoaD Y
// Like LDA, but for the Y register
// Effects Negative and Zero
void _ldy(std::shared_ptr<Operand> operand) {
  index_y = operand->get_val();
  handle_arithmetic_flags(index_y);
}

// STore Accumulator
// Stores accumulator into operand.
void _sta(std::shared_ptr<Operand> operand) { operand->set_val(acc); }

// STore X
// Stores X register into operand.
void _stx(std::shared_ptr<Operand> operand) { operand->set_val(index_x); }

// STore Y
// Stores Y register into operand.
void _sty(std::shared_ptr<Operand> operand) { operand->set_val(index_y); }

// Transfer Accumulator to X
void _tax(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  index_x = acc;
  handle_arithmetic_flags(index_x);
}

// Transfer Accumulator to Y
void _tay(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  index_y = acc;
  handle_arithmetic_flags(index_y);
}

// Transfer Stack pointer to X
void _tsx(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  stack_pointer = index_x;
  handle_arithmetic_flags(stack_pointer);
}

// Transfer X to Accumulator
void _txa(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  acc = index_x;
  handle_arithmetic_flags(acc);
}

// Transfer X to Stack pointer
void _txs(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  stack_pointer = index_x;
  handle_arithmetic_flags(stack_pointer);
}

// Transfer Y to Accumulator
void _tya(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  acc = index_y;
  handle_arithmetic_flags(acc);
}

// PusH Accumulator
// Pushes accumulator onto the stack
void _pha(std::shared_ptr<Operand> operand) {
  cycle_num += 3;

  push_byte(acc);
}

// PusH flags (misleading mnemonic)
// Pushes flag register onto the stack and sets the break flag
void _php(std::shared_ptr<Operand> operand) {
  cycle_num += 3;

  push_byte(flags);
  set_break(true);
}

// PuLl Accumulator
// Pops 1 byte from the stack and sets the accumulator equal to it.
// Effects Negative and Zero
void _pla(std::shared_ptr<Operand> operand) {
  cycle_num += 4;

  acc = pop_byte();
  handle_arithmetic_flags(acc);
}

// PuLl flags (misleading mnemonic)
// Pops 1 byte from the stack and sets the flag register to it.
void _plp(std::shared_ptr<Operand> operand) {
  cycle_num += 4;

  flags = pop_byte();
}

////////////////////////////////
// Miscellaneous Instructions //
////////////////////////////////

// BReaK
// Software interrupt, like "int" on x86.
// This will push the return address and flags register to the stack and begin
// executing the interrupt handler routine at the address specified in the
// interrupt vector address. BRK is actually a 2 byte instruction, with the
// second byte being ignored. This second byte is referred to as the "break
// mark". There's no explicit hardware support for this, but the break mark is
// often used to specify the syscall number in more complex 6502 systems.
void _brk(std::shared_ptr<Operand> operand) {
  int irq_vector = read_word(irq_vector_addr);

  // Most Atari 2600 ROMs won't use interrupts at all, and will simply clear the
  // interrupt vector. If we reach a BRK in one of these games, we should
  // probably just end the program.
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

// CLear Carry
void _clc(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  set_carry(false);
}

// CLear Decimal
void _cld(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  set_decimal(false);
}

// CLear Interrupt enable
void _cli(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  set_interrupt_enable(false);
}

// CLear oVerflow
void _clv(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  set_overflow(false);
}

// NO oPeration
void _nop(std::shared_ptr<Operand> operand) { cycle_num += 2; }

// SEt Carry
void _sec(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  set_carry(true);
}

// SEt Decimal
void _sed(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  set_decimal(true);
}

// SEt Interrupt enable
void _sei(std::shared_ptr<Operand> operand) {
  cycle_num += 2;

  set_interrupt_enable(true);
}

std::function<void(std::shared_ptr<Operand>)> opcode_table[256] = {
    _brk,    _ora,    nullptr,  nullptr, nullptr, _ora, _asl_memory, nullptr,
    _php,    _ora,    _asl_acc, nullptr, nullptr, _ora, _asl_memory, nullptr,
    _bpl,    _ora,    nullptr,  nullptr, nullptr, _ora, _asl_memory, nullptr,
    _clc,    _ora,    nullptr,  nullptr, nullptr, _ora, _asl_memory, nullptr,
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
