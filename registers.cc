#include "registers.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>

uint8_t acc;
uint8_t index_x;
uint8_t index_y;
uint8_t flags;
uint8_t stack_pointer;
uint16_t program_counter;

uint64_t cycle_num;

void init_registers(uint16_t rom_start) {
  acc = 0;
  index_x = 0;
  index_y = 0;
  flags = 0b00000000;
  stack_pointer = 255;
  program_counter = rom_start;
  cycle_num = 0;
}

#define NEGATIVE_FLAG 0x80
#define OVERFLOW_FLAG 0x40
#define BREAK_FLAG 0x10
#define DECIMAL_FLAG 0x08
#define INTERRUPT_ENABLE_FLAG 0x04
#define ZERO_FLAG 0x02
#define CARRY_FLAG 0x01

#define GETTER(func, flag)                                                     \
  bool func() { return flags & flag; }

#define SETTER(func, flag)                                                     \
  void func(bool val) {                                                        \
    if (val) {                                                                 \
      flags |= flag;                                                           \
    } else {                                                                   \
      flags = (flags & (~flag));                                               \
    }                                                                          \
  }

GETTER(get_negative, NEGATIVE_FLAG)
SETTER(set_negative, NEGATIVE_FLAG)
GETTER(get_overflow, OVERFLOW_FLAG)
SETTER(set_overflow, OVERFLOW_FLAG)
GETTER(get_break, BREAK_FLAG)
SETTER(set_break, BREAK_FLAG)
GETTER(get_decimal, DECIMAL_FLAG)
SETTER(set_decimal, DECIMAL_FLAG)
GETTER(get_interrupt_enable, INTERRUPT_ENABLE_FLAG)
SETTER(set_interrupt_enable, INTERRUPT_ENABLE_FLAG)
GETTER(get_zero, ZERO_FLAG)
SETTER(set_zero, ZERO_FLAG)
GETTER(get_carry, CARRY_FLAG)
SETTER(set_carry, CARRY_FLAG)

std::string flags_to_string() {
  std::string ret = "";
  if (get_negative())
    ret += "NEGATIVE | ";
  if (get_overflow())
    ret += "OVERFLOW | ";
  if (get_break())
    ret += "BREAK | ";
  if (get_decimal())
    ret += "DECIMAL | ";
  if (get_interrupt_enable())
    ret += "INTERRUPT_ENABLE | ";
  if (get_zero())
    ret += "ZERO | ";
  if (get_carry())
    ret += "CARRY | ";
  if (ret.length())
    return ret.substr(0, ret.length() - 3);
  return "";
}

void dump_regs() {
  printf("A: %02x\n", acc);
  printf("X: %02x\n", index_x);
  printf("Y: %02x\n", index_y);
  printf("Flags: %s\n", flags_to_string().c_str());
  printf("SP: %02x\n", stack_pointer);
  printf("PC: %02x\n", program_counter);
  printf("Cycle: %lu\n", cycle_num);
  printf("\n");
}

void panic() {
  printf("Unrecoverable error!\n");
  dump_regs();
  dump_memory();
  fflush(stdout);
  usleep(1000);
  exit(-1);
}
