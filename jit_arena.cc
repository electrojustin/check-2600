#include "jit_arena.h"

#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define ALIGN(x, y) (x + (y-1)) & (~(y-1))

JitArena::JitArena(uint32_t init_size) {
	curr_size = init_size;
	next_alloc_ptr = 0;

	mapping = mmap(nullptr, curr_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if ((int64_t)mapping < 0) {
		printf("Mapping failed: %s\n", strerror(errno));
		exit(-1);
	}
}

JitArena::~JitArena() {
	munmap(mapping, curr_size);
}

void JitArena::resize() {
	uint32_t new_size = curr_size*2;
	void* new_mapping = mmap(nullptr, new_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if ((int64_t)new_mapping < 0) {
		printf("Mapping failed: %s\n", strerror(errno));
		exit(-1);
	}

	memcpy(new_mapping, mapping, curr_size);

	munmap(mapping, curr_size);
	curr_size = new_size;
	mapping = new_mapping;
}

uint32_t JitArena::allocate(uint32_t alloc_size) {
	alloc_size = ALIGN(alloc_size, 4);

	while (next_alloc_ptr + alloc_size > curr_size)
		resize();

	uint32_t ret = next_alloc_ptr;
	next_alloc_ptr += alloc_size;
	return ret;
}

void* JitArena::get_base() {
	return mapping;
}
