#include "atari.h"

#include <memory>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <iostream>
#include <string>
#include <string.h>

#include "tia.h"
#include "memory.h"
#include "registers.h"
#include "cpu.h"
#include "pia.h"
#include "disasm.h"

std::unique_ptr<TIA> tia;
std::unique_ptr<PIA> pia;

std::unique_ptr<std::thread> emulation_thread;

std::unordered_map<uint16_t, bool> break_points;

void debug_loop() {
	std::string last_cmd = "help";
	do {
		printf("\n");
		disasm_curr_insn();
		printf(" > ");

		std::string cmd;
		getline(std::cin, cmd);

		if (!cmd.length())
			cmd = last_cmd;

		if (cmd == "step") {
			execute_next_insn();
			tia->process_tia();
			pia->process_pia();
		} else if (cmd == "cont") {
			do {
				execute_next_insn();
				tia->process_tia();	
				pia->process_pia();
			} while (should_execute && !break_points.count(program_counter));
		} else if (cmd == "dump reg") {
			dump_regs();
		} else if (cmd == "dump mem") {
			dump_memory();
		} else if (cmd == "dump tia") {
			tia->dump_tia();
		} else if (cmd == "dump pia") {
			pia->dump_pia();
		} else if (cmd == "dump" || cmd == "dump all") {
			dump_regs();
			dump_memory();
			tia->dump_tia();
			pia->dump_pia();
		} else if (cmd.rfind("break ") != std::string::npos) {
			char* end_ptr;
			long break_point = strtoul(cmd.substr(strlen("break "), cmd.length()).c_str(), &end_ptr, 16);
			if (*end_ptr) {
				printf("Error! Not a number\n");
			} else if (break_point < 0 || break_point > 0xFFFF) {
				printf("Error! Must be number between 0x0000 and 0xFFFF");
			} else {
				break_points[break_point] = true;
			}
		} else if (cmd.rfind("del ") != std::string::npos) {
			char* end_ptr;
			long break_point = strtoul(cmd.substr(strlen("del "), cmd.length()).c_str(), &end_ptr, 16);
			if (*end_ptr) {
				printf("Error! Not a number\n");
			} else if (break_point < 0 || break_point > 0xFFFF) {
				printf("Error! Must be number between 0x0000 and 0xFFFF\n");
			} else if (!break_points.count(break_point)) {
				printf("Error! No such breakpoint!\n");
			} else {
				break_points.erase(break_point);
			}
		} else if (cmd == "exit") {
			should_execute = false;
		} else if (cmd == "help") {
			printf("Possible commands:\n");
			printf("step - steps program\n");
			printf("cont - continue until break point\n");
			printf("dump reg - dump registers\n");
			printf("dump mem - dump RAM bytes\n");
			printf("dump tia - dump TIA state\n");
			printf("dump pia - dump PIA state\n");
			printf("dump all - dump all available state\n");
			printf("break XYZW - sets break point to hex address 0xXYZW\n");
			printf("del XYZW - delete break point\n");
			printf("exit - exit program\n");
		} else {
			printf("Error! Unrecognized command. Type \"help\" for list of available commands\n");
			cmd = "help";
		}

		last_cmd = cmd;

		tia->ntsc.debug_swap_buf();
	} while (should_execute);

	printf("program exiting\n");
	exit(0);
}

void emulate(bool debug) {
	if (debug) {
		debug_loop();
	} else {
		while(should_execute) {
			execute_next_insn();
			tia->process_tia();
			pia->process_pia();
		}
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
