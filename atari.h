#ifndef ATARI_H
#define ATARI_H

#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC
#define IRQ_VECTOR 0xFFFE
#define TIA_START 0x0000
#define TIA_END 0x007F
#define PIA_START 0x0280
#define PIA_END 0x0297
#define RAM_START 0x0080
#define RAM_END 0x00FF
#define ROM_START 0xF000
#define ROM_END 0xFFFF
#define STACK_TOP 0x1FF
#define STACK_BOTTOM 0x100

// Loads the given program file into ROM memory.
void load_program_file(const char *filename);

// Starts emulation in a separate thread. This is to give QT5 (or whatever the
// frontend will be) the main thread for event handling.
void start_emulation_thread(bool debug);

#endif
