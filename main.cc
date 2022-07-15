#include <QApplication>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory>

#include "atari.h"
#include "cpu.h"
#include "registers.h"
#include "memory.h"
#include "qt_display.h"

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("invalid arguments!\n");
		printf("usage: atari2600 <program_file>\n");
		exit(-1);
	}

	uint8_t* rom_backing = (uint8_t*)malloc(ROM_END - ROM_START);
	FILE* program_file = fopen(argv[1], "r");
	if (!program_file) {
		printf("could not open %s", argv[1]);
		exit(-1);
	}

	fread(rom_backing, 1, ROM_END - ROM_START, program_file);
	fclose(program_file);

	init_registers(ROM_START);
	should_execute = true;

	auto ram = std::make_shared<RamRegion>(RAM_START, RAM_END);
	auto rom = std::make_shared<RomRegion>(ROM_START, ROM_END, rom_backing);
	auto stack = std::make_shared<StackRegion>(STACK_BOTTOM, STACK_TOP);
	auto irq_vector = std::make_shared<RamRegion>(NMI_VECTOR, IRQ_VECTOR+1);

	memory_regions.push_back(ram);
	memory_regions.push_back(stack);
	memory_regions.push_back(rom);	
	memory_regions.push_back(irq_vector);

	irq_vector_addr = IRQ_VECTOR;
	write_word(IRQ_VECTOR, 0);

	while(should_execute) {
	//	dump_regs();
		execute_next_insn();
	//	getchar();
	}

	dump_regs();

	free(rom_backing);

	QApplication app(argc, argv);

	QtDisplay display(160, 262);

	memset(display.framebuf, 0, 160*262);
	for (int i = 0; i < 128; i++) {
		for (int j = 0; j < 128; j++) {
			display.framebuf[i*160 + j] = ((i/8) << 4) | (j/8);
		}
	}

	display.setWindowTitle("test");
	display.show();

	return app.exec();
}
