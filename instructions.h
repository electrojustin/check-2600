#include "operand.h"

#include <functional>
#include <memory>
#include <stdint.h>
#include <string>

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

// Get the function that performs the instruction corresponding with the given
// opcode. |should_succeed| indicates whether or not we should crash if this is
// an invalid instruction. If we're just filling the instruction cache and we
// come across an invalid instruction, we might have just hit a data segment. If
// we're decoding the current memory at |program_counter| however, we need to
// either succeed or crash.
std::function<void(std::shared_ptr<Operand>)>
get_insn(uint8_t opcode, bool should_succeed = true);

std::string get_mnemonic(uint8_t opcode);

#endif
