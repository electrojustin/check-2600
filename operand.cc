#include "operand.h"

#include "memory.h"
#include "registers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum IndexMode {
	no_indexing,
	x_indexing,
	y_indexing,
};

class Relative : public Operand {
	int8_t offset;

public:
	Relative(int8_t offset) {
		this->offset = offset;
	}

	int get_val() override {
		// Note that relative refers to relative to the next instruction.
		// The program counter is supposed to already be pointer there.
		return program_counter + offset + get_insn_len();
	}

	void set_val(int val) override {
		printf("Error! Relatives do not support set!\n");
		panic();
	}

	int get_insn_len() override {
		return 2;
	}

	int get_cycle_penalty() override {
		return 0;
	}

	std::string to_string() override {
		char buf[256];
		snprintf(buf, 256, "0x%04x", get_val());
		return std::string(buf);
	}
};

class IndirectX : public Operand {
	uint8_t zero_page_addr;

public:
	IndirectX(uint8_t zero_page_addr) {
		this->zero_page_addr = zero_page_addr;
	}

	int get_val() override {
		uint16_t absolute_addr = read_word(zero_page_addr + index_x);
		return read_byte(absolute_addr);
	}

	void set_val(int val) override {
		uint16_t absolute_addr = read_word(zero_page_addr + index_x);
		write_byte(absolute_addr, val);
	}

	int get_insn_len() override {
		return 2;
	}

	int get_cycle_penalty() override {
		return 6;
	}

	std::string to_string() override {
		char buf[256];
		snprintf(buf, 256, "(0x%04x,X)", get_val());
		return std::string(buf);
	}
};

class IndirectY : public Operand {
	uint8_t zero_page_addr;
	bool extra_cycle;

public:
	IndirectY(uint8_t zero_page_addr, bool extra_cycle=false) {
		this->zero_page_addr = zero_page_addr;
		this->extra_cycle = extra_cycle;
	}

	int get_val() override {
		uint16_t absolute_addr = read_word(zero_page_addr) + index_y;
		return read_byte(absolute_addr);
	}

	void set_val(int val) override {
		uint16_t absolute_addr = read_word(zero_page_addr) + index_y;
		write_byte(absolute_addr, val);
	}

	int get_insn_len() override {
		return 2;
	}

	int get_cycle_penalty() override {
		if (extra_cycle || ((uint16_t)zero_page_addr) + index_y > PAGE_SIZE) {
			return 6;
		} else {
			return 5;
		}
	}

	std::string to_string() override {
		char buf[256];
		snprintf(buf, 256, "(0x%04x),Y", get_val());
		return std::string(buf);
	}
};

class Indirect : public Operand {
	uint16_t absolute_addr;

public:
	Indirect(uint16_t absolute_addr) {
		this->absolute_addr = absolute_addr;
	}

	int get_val() override {
		uint16_t indirect_addr = read_word(absolute_addr);
		return read_word(indirect_addr);
	}

	void set_val(int val) override {
		uint16_t indirect_addr = read_word(absolute_addr);
		write_word(indirect_addr, val);
	}

	int get_insn_len() override {
		return 2;
	}

	int get_cycle_penalty() override {
		return 6;
	}

	std::string to_string() override {
		char buf[256];
		snprintf(buf, 256, "(0x%04x)", get_val());
		return std::string(buf);
	}
};

class Immediate : public Operand {
	uint8_t immediate;

public:
	Immediate(uint8_t immediate) {
		this->immediate = immediate;
	}

	int get_val() override {
		return immediate;
	}

	void set_val(int val) override {
		printf("Error! Immediates do not support set!\n");
		panic();
	}

	int get_insn_len() override {
		return 2;
	}

	int get_cycle_penalty() override {
		return 2;
	}

	std::string to_string() override {
		char buf[256];
		snprintf(buf, 256, "#0x%04x", get_val());
		return std::string(buf);
	}
};

class ZeroPage : public Operand {
	uint8_t zero_page_addr;
	uint8_t index;
	bool is_indexed;
	enum IndexMode index_mode;

	uint16_t get_addr() {
		switch(index_mode) {
			case no_indexing:
				return zero_page_addr;
			case x_indexing:
				return zero_page_addr + index_x;
			case y_indexing:
				return zero_page_addr + index_y;
		}		
	}

public:
	ZeroPage(uint8_t zero_page_addr, enum IndexMode index_mode) {
		this->zero_page_addr = zero_page_addr;
		this->index_mode = index_mode;
	}

	int get_val() override {
		return read_byte(get_addr());
	}

	void set_val(int val) override {
		write_byte(get_addr(), val);
	}

	int get_insn_len() override {
		return 2;
	}

	int get_cycle_penalty() override {
		return index_mode != IndexMode::no_indexing ? 4 : 3;
	}

	std::string to_string() override {
		char buf[256];
		snprintf(buf, 256, "(0x%02x)", get_val());
		return std::string(buf);
	}
};

class Absolute : public Operand {
	uint16_t addr;
	bool extra_cycle;
	bool should_read_word;
	enum IndexMode index_mode;

	uint16_t get_addr() {
		switch(index_mode) {
			case no_indexing:
				return addr;
			case x_indexing:
				return addr + index_x;
			case y_indexing:
				return addr + index_y;
		}		
	}

public:
	Absolute(uint16_t addr, IndexMode index_mode, bool extra_cycle, bool should_read_word=false) {
		this->addr = addr;
		this->index_mode = index_mode;
		this->extra_cycle = extra_cycle;
		this->should_read_word = should_read_word;
	}

	int get_val() override {
		if (!should_read_word) {
			return read_byte(get_addr());
		} else {
			return read_word(get_addr());
		}
	}

	void set_val(int val) override {
		write_byte(get_addr(), val);
	}

	int get_insn_len() override {
		return 3;
	}

	int get_cycle_penalty() override {
		uint16_t base_page = addr & (~(PAGE_SIZE-1));
		uint16_t indexed_page = (get_addr()) & (~(PAGE_SIZE-1));
		if (extra_cycle || base_page != indexed_page) {
			return 5;
		} else {
			return 4;
		}
	}

	std::string to_string() override {
		char buf[256];
		snprintf(buf, 256, "0x%04x", get_val());
		return std::string(buf);
	}
};

class Implied : public Operand {
public:
	Implied() {}

	int get_val() override {
		return 1;
	}

	void set_val(int val) override {
		printf("Error! Implied operands do not support set!\n");
		panic();
	}

	int get_insn_len() override {
		return 1;
	}

	int get_cycle_penalty() override {
		return 0;
	}

	std::string to_string() override {
		return std::string();
	}
};

std::shared_ptr<Operand> create_operand(uint16_t addr, uint8_t opcode, uint8_t byte1, uint8_t byte2) {
	uint8_t high_nibble = opcode >> 4;
	uint8_t low_nibble = opcode & 0xF;
	uint16_t abs_word = ((uint16_t)byte2) << 8 | byte1;

	switch (low_nibble) {
		case 0:
			if (high_nibble & 1) {
				return std::make_shared<Relative>(byte1);
			} else if (high_nibble == 2) {
				return std::make_shared<Absolute>(addr+1, IndexMode::no_indexing, false, true);
			} else if (high_nibble == 0xA || high_nibble == 0xC || high_nibble == 0xE) {
				return std::make_shared<Immediate>(byte1);
			} else if (!high_nibble || high_nibble == 0x4 || high_nibble == 0x6) {
				return std::make_shared<Implied>();
			}
			break;
		case 1:
			if (high_nibble & 1) {
				return std::make_shared<IndirectY>(byte1, opcode == 0x91);
			} else {
				return std::make_shared<IndirectX>(byte1);
			}
		case 2:
			if (high_nibble == 0xA)
				return std::make_shared<Immediate>(byte1);
			break;
		case 4:
			if (high_nibble == 2 || ((high_nibble & 0x8) && !(high_nibble & 1))) {
				return std::make_shared<ZeroPage>(byte1, IndexMode::no_indexing);
			} else if (high_nibble == 0x9 || high_nibble == 0xB) {
				return std::make_shared<ZeroPage>(byte1, IndexMode::x_indexing);
			}
			break;
		case 5:
			if (high_nibble & 1) {
				return std::make_shared<ZeroPage>(byte1, IndexMode::x_indexing);
			} else {
				return std::make_shared<ZeroPage>(byte1, IndexMode::no_indexing);
			}
		case 6:
			if (high_nibble & 1) {
				if (high_nibble == 0x9 || high_nibble == 0xB) {
					return std::make_shared<ZeroPage>(byte1, IndexMode::y_indexing);
				} else {
					return std::make_shared<ZeroPage>(byte1, IndexMode::x_indexing);
				}
			} else {
				return std::make_shared<ZeroPage>(byte1, IndexMode::no_indexing);
			}
		case 8:
			return std::make_shared<Implied>();
		case 9:
			if (high_nibble & 1) {
				return std::make_shared<Absolute>(abs_word, IndexMode::y_indexing, opcode == 0x99);
			} else {
				if (high_nibble == 0x8) {
					break;
				} else {
					return std::make_shared<Immediate>(byte1);
				}
			}
		case 0xA:
			if (!(high_nibble & 0x1) || high_nibble == 0x9 || high_nibble == 0xB)
				return std::make_shared<Implied>();
			break;
		case 0xC:
			if (high_nibble && !(high_nibble & 0x1) && high_nibble != 0x6) {
				return std::make_shared<Absolute>(addr+1, IndexMode::no_indexing, false, true);
			} else if (high_nibble == 0x6) {
				return std::make_shared<Indirect>(addr+1);
			} else if (high_nibble == 0xB) {
				return std::make_shared<Absolute>(abs_word, IndexMode::x_indexing, false);
			}
			break;
		case 0xD:
			if (high_nibble & 1) {
				return std::make_shared<Absolute>(abs_word, IndexMode::x_indexing, opcode == 0x9D);
			} else {
				return std::make_shared<Absolute>(abs_word, IndexMode::no_indexing, false);
			}
		case 0xE:
			if (high_nibble == 0x9) {
				break;
			} else if (high_nibble == 0xB) {
				return std::make_shared<Absolute>(abs_word, IndexMode::y_indexing, false);
			} else if (high_nibble & 0x1) {
				return std::make_shared<Absolute>(abs_word, IndexMode::x_indexing, true);
			} else {
				return std::make_shared<Absolute>(abs_word, IndexMode::no_indexing, false);
			}
		default:
			break;
	}

	printf("Cannot create operand for opcode %x!\n", opcode);
	panic();
	return nullptr;
}
