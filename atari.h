#ifndef ATARI_H
#define ATARI_H

#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC
#define IRQ_VECTOR 0xFFFE
#define TIA_START 0x0000
#define TIA_END 0x007F
#define RAM_START 0x0080
#define RAM_END 0x00FF
#define ROM_START 0x1000
#define ROM_END 0x1200
#define STACK_TOP 0x1FF
#define STACK_BOTTOM 0x100

#endif
