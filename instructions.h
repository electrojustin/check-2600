#include "operand.h"

#include <stdint.h>
#include <functional>
#include <memory>

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

void (*get_insn(uint8_t opcode, bool should_succeed=true))(Operand*);

// Returns true if the current opcode has the potential to modify the program counter
bool is_branch(uint8_t opcode);

#endif
