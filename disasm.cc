#include "disasm.h"

#include <stdint.h>
#include <stdio.h>

#include "instructions.h"
#include "memory.h"
#include "operand.h"
#include "registers.h"

void disasm_curr_insn() {
  uint8_t opcode = read_byte(program_counter);
  uint8_t byte1 = read_byte(program_counter + 1);
  uint8_t byte2 = read_byte(program_counter + 2);
  auto mnemonic = get_mnemonic(opcode);

  if (!mnemonic.length()) {
    printf("<Invalid Instruction>\n");
  } else {
    auto operand = create_operand(program_counter, opcode, byte1, byte2);
    auto disasm_string = mnemonic + "\t" + operand->to_string();
    printf("%x\t%s\n", program_counter, disasm_string.c_str());
  }
}
