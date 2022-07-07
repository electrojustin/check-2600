#include <stdint.h>

#ifndef REGISTERS_H
#define REGISTERS_H

extern uint8_t acc;
extern uint8_t index_x;
extern uint8_t index_y;
extern uint8_t flags;
extern uint8_t stack_pointer;
extern uint16_t program_counter;

void init_registers(uint16_t rom_start);

bool get_negative();
void set_negative(bool val);
bool get_overflow();
void set_overflow(bool val);
bool get_break();
void set_break(bool val);
bool get_decimal();
void set_decimal(bool val);
bool get_interrupt_enable();
void set_interrupt_enable(bool val);
bool get_zero();
void set_zero(bool val);
bool get_carry();
void set_carry(bool val);

void dump_regs();
void panic();

#endif
