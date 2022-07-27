#include <memory>
#include <stdint.h>
#include <string>

#ifndef OPERAND_H
#define OPERAND_H

class Operand {
public:
  virtual int get_val() = 0;
  virtual void set_val(int val) = 0;
  virtual int get_insn_len() = 0;
  virtual int get_cycle_penalty() = 0;
  virtual std::string to_string() = 0;
};

std::shared_ptr<Operand> create_operand(uint16_t addr, uint8_t opcode,
                                        uint8_t byte1, uint8_t byte2);

#endif
