#include "operand.h"

#include "memory.h"
#include "registers.h"

#include <stdio.h>
#include <stdlib.h>

class Relative : public Operand {
	int8_t offset;

public:
	Relative(int8_t offset) {
		this->offset = offset;
	}

	int get_val() override {
		return program_counter + offset;
	}

	void set_val(int val) override {
		printf("Error! Relatives do not support set!\n");
		panic();
	}

	int get_insn_len() override {
		return 2;
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
};

class IndirectY : public Operand {
	uint8_t zero_page_addr;

public:
	IndirectY(uint8_t zero_page_addr) {
		this->zero_page_addr = zero_page_addr;
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
};

class Indirect : public Operand {
	uint8_t zero_page_addr;

public:
	Indirect(uint8_t zero_page_addr) {
		this->zero_page_addr = zero_page_addr;
	}

	int get_val() override {
		uint16_t absolute_addr = read_word(zero_page_addr);
		return read_byte(absolute_addr);
	}

	void set_val(int val) override {
		uint16_t absolute_addr = read_word(zero_page_addr);
		write_byte(absolute_addr, val);
	}

	int get_insn_len() override {
		return 2;
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
};

class ZeroPage : public Operand {
	uint8_t zero_page_addr;
	uint8_t index;

public:
	ZeroPage(uint8_t zero_page_addr, uint8_t index) {
		this->zero_page_addr = zero_page_addr;
		this->index = index;
	}

	int get_val() override {
		return read_byte(zero_page_addr + index);
	}

	void set_val(int val) override {
		write_byte(zero_page_addr + index, val);
	}

	int get_insn_len() override {
		return 2;
	}
};

class Absolute : public Operand {
	uint16_t addr;
	uint8_t index;

public:
	Absolute(uint16_t addr, uint8_t index) {
		this->addr = addr;
		this->index = index;
	}

	int get_val() override {
		return read_byte(addr + index);
	}

	void set_val(int val) override {
		write_byte(addr + index, val);
	}

	int get_insn_len() override {
		return 3;
	}
};

class Implied : public Operand {
public:
	Implied() {}

	int get_val() override {
		printf("Error! Implied operands do not support get!\n");
		panic();
	}

	void set_val(int val) override {
		printf("Error! Implied operands do not support set!\n");
		panic();
	}

	int get_insn_len() override {
		return 1;
	}
};

class Accumulator : public Operand {
public:
	Accumulator() {}

	int get_val() override {
		return acc;
	}

	void set_val(int val) override {
		acc = val;
	}

	int get_insn_len() override {
		return 1;
	}
};

std::shared_ptr<Operand> create_operand(uint8_t opcode, uint8_t byte1, uint8_t byte2) {
	uint8_t high_nibble = opcode >> 4;
	uint8_t low_nibble = opcode & 0xF;
	uint16_t abs_word = ((uint16_t)byte2) << 8 | byte1;

	switch (low_nibble) {
		case 0:
			if (high_nibble & 1) {
				return std::make_shared<Relative>(byte1);
			} else if (high_nibble == 2) {
				return std::make_shared<Absolute>(abs_word, 0);
			} else if (high_nibble == 0xA || high_nibble == 0xC || high_nibble == 0xE) {
				return std::make_shared<Immediate>(byte1);
			} else if (!high_nibble || high_nibble == 0x4 || high_nibble == 0x6) {
				return std::make_shared<Implied>();
			}
			break;
		case 1:
			if (high_nibble & 1) {
				return std::make_shared<IndirectY>(byte1);
			} else {
				return std::make_shared<IndirectX>(byte1);
			}
		case 2:
			if (high_nibble == 0xA)
				return std::make_shared<Immediate>(byte1);
			break;
		case 4:
			if (high_nibble == 2 || ((high_nibble & 0x8) && !(high_nibble & 1))) {
				return std::make_shared<ZeroPage>(byte1, 0);
			} else if (high_nibble == 0x9 || high_nibble == 0xB) {
				return std::make_shared<ZeroPage>(byte1, index_x);
			}
			break;
		case 5:
			if (high_nibble & 1) {
				return std::make_shared<ZeroPage>(byte1, index_x);
			} else {
				return std::make_shared<ZeroPage>(byte1, 0);
			}
		case 6:
			if (high_nibble & 1) {
				if (high_nibble == 0x9 || high_nibble == 0xB) {
					return std::make_shared<ZeroPage>(byte1, index_y);
				} else {
					return std::make_shared<ZeroPage>(byte1, index_x);
				}
			} else {
				return std::make_shared<ZeroPage>(byte1, 0);
			}
		case 8:
			return std::make_shared<Implied>();
		case 9:
			if (high_nibble & 1) {
				return std::make_shared<Absolute>(abs_word, index_y);
			} else {
				if (high_nibble == 0x8) {
					break;
				} else {
					return std::make_shared<Immediate>(byte1);
				}
			}
		case 0xA:
			if (!high_nibble || high_nibble == 0x2 || high_nibble == 0x4 || high_nibble == 0x6) {
				return std::make_shared<Accumulator>();
			} else if (!(high_nibble & 1) || high_nibble == 0x9 || high_nibble == 0x0B) {
				return std::make_shared<Implied>();
			}
			break;
		case 0xC:
			if (high_nibble && !(high_nibble & 0x1) && high_nibble != 0x6) {
				return std::make_shared<Absolute>(abs_word, 0);
			} else if (high_nibble == 0x6) {
				return std::make_shared<Indirect>(byte1);
			} else if (high_nibble == 0xB) {
				return std::make_shared<Absolute>(abs_word, index_x);
			}
			break;
		case 0xD:
			if (high_nibble & 1) {
				return std::make_shared<Absolute>(abs_word, index_x);
			} else {
				return std::make_shared<Absolute>(abs_word, 0);
			}
		case 0xE:
			if (high_nibble == 0x9) {
				break;
			} else if (high_nibble & 0x1) {
				return std::make_shared<Absolute>(abs_word, index_x);
			} else {
				return std::make_shared<Absolute>(abs_word, 0);
			}
		default:
			break;
	}

	printf("Cannot create operand for opcode %x!\n", opcode);
	panic();
	return nullptr;
}
