#include <stdint.h>

#ifndef JIT_ARENA_H
#define JIT_ARENA_H

// Arena is appropriate here because we are mostly dealing with ROMs
class JitArena {
private:
	uint32_t curr_size;
	uint32_t next_alloc_ptr;
	void* mapping;

	void resize();

public:
	JitArena(uint32_t init_size=65536);
	~JitArena();

	void* allocate(uint32_t alloc_size);
};

#endif
