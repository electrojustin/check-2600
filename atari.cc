#include "atari.h"

#include <memory>
#include <thread>
#include <stdio.h>
#include <stdlib.h>

#include "tia.h"
#include "memory.h"
#include "registers.h"
#include "cpu.h"
#include "pia.h"

std::unique_ptr<TIA> tia;
std::unique_ptr<PIA> pia;

std::shared_ptr<RamRegion> ram = nullptr;
std::shared_ptr<RomRegion> rom = nullptr;
std::shared_ptr<RamRegion> irq_vector = nullptr;

std::unique_ptr<std::thread> emulation_thread;

void emulate(bool debug) {
	while(should_execute) {
		if (debug) {
			printf("Gun X: %d  Gun Y: %d\n", tia->ntsc.gun_x, tia->ntsc.gun_y);
			printf("Timer: %x\n", pia->timer);
			dump_regs();
		}
		execute_next_insn();
		tia->process_tia();
		pia->process_pia();
	}
}

void load_program_file(const char* filename) {
	uint8_t* rom_backing = (uint8_t*)malloc(ROM_END - ROM_START);
	FILE* program_file = fopen(filename, "r");
	if (!program_file) {
		printf("could not open %s\n", filename);
		exit(-1);
	}

	fread(rom_backing, 1, ROM_END - ROM_START, program_file);
	fclose(program_file);

	init_registers(ROM_START);

	tia = std::make_unique<TIA>();
	pia = std::make_unique<PIA>();

	ram = std::make_shared<RamRegion>(RAM_START, RAM_END);
	rom = std::make_shared<RomRegion>(ROM_START, ROM_END, rom_backing);

	memory_regions.push_back(ram);
	memory_regions.push_back(rom);
	memory_regions.push_back(tia->get_dma_region());
	memory_regions.push_back(pia->get_dma_region());

	stack_region = ram;

	init_registers(read_word(RESET_VECTOR));

	free(rom_backing);
}

void start_emulation_thread(bool debug) {
	should_execute = true;
	emulation_thread = std::make_unique<std::thread>(emulate, debug);
}
