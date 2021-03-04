#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

struct cartridge;

struct memory {
	uint8_t cpu_ram[2048];
	uint8_t ppu_regs[8];
	uint8_t ppu_ram[2000];
	struct cartridge *cart;
};

void init_memory(struct memory *mem, struct cartridge *cart);

void write_cpu_byte(struct memory *mem, uint16_t addr, uint8_t byte);
uint8_t read_cpu_byte(struct memory *mem, uint16_t addr);

void write_ppu_byte(struct memory *mem, uint16_t addr, uint8_t byte);
uint8_t read_ppu_byte(struct memory *mem, uint16_t addr);

#endif
