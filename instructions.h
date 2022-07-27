#include "operand.h"

#include <stdint.h>
#include <functional>
#include <memory>
#include <string>

#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

std::function<void(std::shared_ptr<Operand>)> get_insn(uint8_t opcode, bool should_succeed=true);

std::string get_mnemonic(uint8_t opcode);

#endif
