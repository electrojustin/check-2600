#include <stdint.h>

#ifndef MEMORY_H
#define MEMORY_H

#define RAM_SIZE 0x10000

#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC
#define IRQ_VECTOR 0xFFFE
#define ROM_START 0x1000
#define ROM_SIZE 0x1FFF
#define STACK_TOP 0x200
#define STACK_BOTTOM 0x100

extern uint8_t* rom;
extern uint8_t ram[RAM_SIZE];

uint8_t read_byte(uint16_t addr);
uint16_t read_word(uint16_t addr);
void write_byte(uint16_t addr, uint8_t val);
void write_word(uint16_t addr, uint16_t val);

#endif
