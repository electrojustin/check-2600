#include <stdint.h>

#ifndef CPU_H
#define CPU_H

// Executes the instruction located at |program_counter|
void execute_next_insn();

// Flag to tell the emulator when to stop. In silicon, the machine always ran
// from power on, but for emulation sake we stop the program when we detect a
// BRK with no IRQ vector set.
extern bool should_execute;

#endif
