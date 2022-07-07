#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

#include "registers.h"

uint8_t* rom = ram + ROM_START;
uint8_t ram[RAM_SIZE];

uint8_t read_byte(uint16_t addr) {
	if (addr >= RAM_SIZE) {
		printf("Invalid read at %x\n", addr);
		panic();
	}

	return ram[addr]; 
}

uint16_t read_word(uint16_t addr) {
	if (addr+1 >= RAM_SIZE) {
		printf("Invalid read at %x\n", addr);
		panic();
	}

	return *((uint16_t*)(ram + addr));
}

void write_byte(uint16_t addr, uint8_t val) {
	if (addr >= RAM_SIZE) {
		printf("Invalid write at %x\n", addr);
		panic();
	}

	ram[addr] = val;
}

void write_word(uint16_t addr, uint16_t val) {
	if (addr+1 >= RAM_SIZE) {
		printf("Invalid write at %x\n", addr);
		panic();
	}

	*((uint16_t*)(ram + addr)) = val;
}
