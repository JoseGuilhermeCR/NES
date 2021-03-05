#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stdint.h>

struct Cartridge;

struct Memory {
	uint8_t cpu_ram[2048];
	uint8_t ppu_regs[8];
	uint8_t ppu_ram[2000];
	struct Cartridge *cart;
};

void memory_init(struct Memory *mem, struct Cartridge *cart);

void write_cpu_byte(struct Memory *mem, uint16_t addr, uint8_t byte);
uint8_t read_cpu_byte(const struct Memory *mem, uint16_t addr);

void write_ppu_byte(struct Memory *mem, uint16_t addr, uint8_t byte);
uint8_t read_ppu_byte(const struct Memory *mem, uint16_t addr);

#endif
