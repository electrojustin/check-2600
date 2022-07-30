#include "atari.h"

#include <iostream>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <thread>
#include <unordered_map>

#include "cpu.h"
#include "disasm.h"
#include "memory.h"
#include "pia.h"
#include "registers.h"
#include "tia.h"

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

    // Repeat the previous command by default.
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
    } else if (cmd == "frame") {
      do {
        execute_next_insn();
        tia->process_tia();
        pia->process_pia();
      } while (should_execute && !tia->ntsc->gun_y);
      do {
        execute_next_insn();
        tia->process_tia();
        pia->process_pia();
      } while (should_execute && tia->ntsc->gun_y);
    } else if (cmd == "scan") {
      int old_gun_y = tia->ntsc->gun_y;
      do {
        execute_next_insn();
        tia->process_tia();
        pia->process_pia();
      } while (should_execute && tia->ntsc->gun_y == old_gun_y);
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
      char *end_ptr;
      long break_point = strtoul(
          cmd.substr(strlen("break "), cmd.length()).c_str(), &end_ptr, 16);
      if (*end_ptr) {
        printf("Error! Not a number\n");
      } else if (break_point < 0 || break_point > 0xFFFF) {
        printf("Error! Must be number between 0x0000 and 0xFFFF");
      } else {
        break_points[break_point] = true;
      }
    } else if (cmd.rfind("del ") != std::string::npos) {
      char *end_ptr;
      long break_point = strtoul(
          cmd.substr(strlen("del "), cmd.length()).c_str(), &end_ptr, 16);
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
      printf("frame - continue until next frame\n");
      printf("scan - continue until next scanline\n");
      printf("dump reg - dump registers\n");
      printf("dump mem - dump RAM bytes\n");
      printf("dump tia - dump TIA state\n");
      printf("dump pia - dump PIA state\n");
      printf("dump all - dump all available state\n");
      printf("break XYZW - sets break point to hex address 0xXYZW\n");
      printf("del XYZW - delete break point at hex address 0xXYZW\n");
      printf("exit - exit program\n");
    } else {
      printf("Error! Unrecognized command. Type \"help\" for list of available "
             "commands\n");
      cmd = "help";
    }

    last_cmd = cmd;

    // Flush as much of the screen as we have to the user. This is useful for
    // debugging rendering, so we can watch the scanlines draw as we "step" the
    // program.
    tia->ntsc->debug_swap_buf();
  } while (should_execute);

  printf("program exiting\n");
  exit(0);
}

void emulate(bool debug) {
  if (debug) {
    debug_loop();
  } else {
    while (should_execute) {
      execute_next_insn();
      tia->process_tia();
      pia->process_pia();
    }
  }
}

void load_program_file(const char *filename, int scale) {
  uint8_t *rom_backing = (uint8_t *)malloc(ROM_END - ROM_START);
  FILE *program_file = fopen(filename, "r");
  if (!program_file) {
    printf("could not open %s\n", filename);
    exit(-1);
  }

  fread(rom_backing, 1, ROM_END - ROM_START, program_file);
  fclose(program_file);

  tia = std::make_unique<TIA>(scale);
  pia = std::make_unique<PIA>();

  auto ram = std::make_shared<RamRegion>(RAM_START, RAM_END);
  auto rom = std::make_shared<RomRegion>(ROM_START, ROM_END, rom_backing);

  // These are the most commonly used "mirrors" of the TIA and normal RAM
  // memory.
  auto ram_mirror =
      std::make_shared<MirrorRegion>(RAM_START + 0x100, RAM_END + 0x100, ram);
  auto tia_mirror = std::make_shared<MirrorRegion>(
      TIA_START + 0x100, TIA_END + 0x100, tia->get_memory_region());

  memory_regions.push_back(ram);
  memory_regions.push_back(rom);
  memory_regions.push_back(tia->get_memory_region());
  memory_regions.push_back(pia->get_memory_region());
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
