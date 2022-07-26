#include <stdint.h>
#include <functional>

#ifndef CPU_H
#define CPU_H

void execute_next_insn();

extern bool should_execute;

extern std::function<void(void)> additional_callback;

#endif
