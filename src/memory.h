#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

struct cartridge;

struct memory {
	uint8_t ram[2048];
	uint8_t ppu_regs[8];
	struct cartridge *cart;
};

void init_memory(struct memory *mem, struct cartridge *cart);

void write_byte(struct memory *mem, uint16_t addr, uint8_t byte);
uint8_t read_byte(struct memory *mem, uint16_t addr);

#endif
