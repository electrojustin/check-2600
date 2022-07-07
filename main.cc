#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "registers.h"
#include "memory.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("invalid arguments!\n");
		printf("usage: atari2600 <program_file>\n");
		exit(-1);
	}

	FILE* program_file = fopen(argv[1], "r");
	if (!program_file) {
		printf("could not open %s", argv[1]);
		exit(-1);
	}

	fread(rom, 1, ROM_SIZE, program_file);
	fclose(program_file);

	init_registers();
	should_execute = true;

	while(should_execute)
		execute_next_insn();

	dump_regs();
}
