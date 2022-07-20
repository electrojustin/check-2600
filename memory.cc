#include "memory.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "registers.h"

std::vector<std::shared_ptr<MemoryRegion>> memory_regions;

uint16_t irq_vector_addr;

bool dirty_pages[256] = { false };

RamRegion::RamRegion(uint16_t start_addr, uint16_t end_addr) {
	this->start_addr = start_addr;
	this->end_addr = end_addr;
	type = RAM;

	size_t len = end_addr - start_addr + 1;
	backing_memory = (uint8_t*)malloc(len);
}

RamRegion::~RamRegion() {
	free(backing_memory);
}

uint8_t RamRegion::read_byte(uint16_t addr) {
	return backing_memory[addr - start_addr];
}

void RamRegion::write_byte(uint16_t addr, uint8_t val) {
	backing_memory[addr - start_addr] = val;
	dirty_pages[addr >> 8] = true;
}

RomRegion::RomRegion(uint16_t start_addr, uint16_t end_addr, uint8_t* init_data) {
	this->start_addr = start_addr;
	this->end_addr = end_addr;
	type = ROM;

	size_t len = end_addr - start_addr + 1;
	backing_memory = (uint8_t*)malloc(len);

	memcpy(backing_memory, init_data, len);
}

RomRegion::~RomRegion() {
	free(backing_memory);
}

uint8_t RomRegion::read_byte(uint16_t addr) {
	return backing_memory[addr - start_addr];
}

void RomRegion::write_byte(uint16_t addr, uint8_t val) {
	printf("Error! Attempted to write to ROM address %x\n", addr);
	panic();
}

DmaRegion::DmaRegion(uint16_t start_addr, uint16_t end_addr, std::function<uint8_t(uint16_t)> read_hook, std::function<void(uint16_t, uint8_t)> write_hook) {
	this->start_addr = start_addr;
	this->end_addr = end_addr;
	this->read_hook = read_hook;
	this->write_hook = write_hook;
	type = DMA;
}

uint8_t DmaRegion::read_byte(uint16_t addr) {
	return read_hook(addr);
}

void DmaRegion::write_byte(uint16_t addr, uint8_t val) {
	write_hook(addr, val);
}

StackRegion::StackRegion(uint16_t start_addr, uint16_t end_addr) {
	this->start_addr = start_addr;
	this->end_addr = end_addr;
	type = STACK;

	size_t len = end_addr - start_addr + 1;
	backing_memory = (uint8_t*)malloc(len);
}

StackRegion::~StackRegion() {
	free(backing_memory);
}

uint8_t StackRegion::read_byte(uint16_t addr) {
	if (addr < start_addr || addr > end_addr) {
		printf("Error! Stack overflow!\n");
		panic();
	}

	return backing_memory[addr - start_addr];
}

void StackRegion::write_byte(uint16_t addr, uint8_t val) {
	if (addr < start_addr || addr > end_addr) {
		printf("Error! Stack overflow!\n");
		panic();
	}

	backing_memory[addr - start_addr] = val;
}



uint8_t read_byte(uint16_t addr) {
	for (auto region : memory_regions) {
		if (region->start_addr <= addr && region->end_addr >= addr)
			return region->read_byte(addr);
	}

	printf("Error! Invalid read at address %x\n", addr);
	panic();
	return -1;
}

void write_byte(uint16_t addr, uint8_t val) {
	for (auto region : memory_regions) {
		if (region->start_addr <= addr && region->end_addr >= addr) {
			region->write_byte(addr, val);
			return;
		}
	}

	printf("Error! Invalid write at address %x\n", addr);
	panic();
}

void push_byte(uint8_t val) {
	std::shared_ptr<MemoryRegion> stack_region = nullptr;
	for (auto region : memory_regions) {
		if (region->type == STACK) {
			stack_region = region;
			break;
		}
	}

	if (!stack_region) {
		printf("Error! Attempted to push byte, but machine has no stack.\n");
		panic();
	}

	stack_region->write_byte(stack_region->start_addr + stack_pointer, val);
	stack_pointer--;
}

uint8_t pop_byte() {
	std::shared_ptr<MemoryRegion> stack_region = nullptr;
	for (auto region : memory_regions) {
		if (region->type == STACK) {
			stack_region = region;
			break;
		}
	}

	if (!stack_region) {
		printf("Error! Attempted to push byte, but machine has no stack.\n");
		panic();
	}

	stack_pointer++;
	uint8_t ret = stack_region->read_byte(stack_region->start_addr + stack_pointer);

	return ret;
}

void push_word(uint16_t val) {
	push_byte(val >> 8);
	push_byte(val & 0xFF);
}

uint16_t pop_word() {
	uint16_t byte1 = pop_byte();
	uint16_t byte2 = pop_byte();

	return byte1 | (byte2 << 8);
}

uint16_t read_word(uint16_t addr) {
	uint16_t ret = read_byte(addr+1);
	ret = (ret << 8) | read_byte(addr);
	return ret;
}

void write_word(uint16_t addr, uint16_t val) {
	write_byte(addr, val & 0xFF);
	write_byte(addr+1, val >> 8);
}

bool is_dirty_page(uint16_t addr) {
	return dirty_pages[addr >> 8];
}

void mark_page_clean(uint16_t addr) {
	dirty_pages[addr >> 8] = false;
}
