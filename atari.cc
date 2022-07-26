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

	auto ram = std::make_shared<RamRegion>(RAM_START, RAM_END);
	auto rom = std::make_shared<RomRegion>(ROM_START, ROM_END, rom_backing);

	auto ram_mirror = std::make_shared<MirrorRegion>(RAM_START+0x100, RAM_END+0x100, ram);
	auto tia_mirror = std::make_shared<MirrorRegion>(TIA_START+0x100, TIA_END+0x100, tia->get_dma_region());

	memory_regions.push_back(ram);
	memory_regions.push_back(rom);
	memory_regions.push_back(tia->get_dma_region());
	memory_regions.push_back(pia->get_dma_region());
	memory_regions.push_back(ram_mirror);
	memory_regions.push_back(tia_mirror);

	stack_region = ram;

	init_registers(read_word(RESET_VECTOR));

	free(rom_backing);
}

void start_emulation_thread(bool debug) {
	should_execute = true;
	emulation_thread = std::make_unique<std::thread>(emulate, debug);
}
