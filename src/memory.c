#include "memory.h"
#include "cartridge.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void init_memory(struct memory *mem, struct cartridge *cart)
{
	memset(mem->cpu_ram, 0, 2048);
	memset(mem->ppu_ram, 0, 2000);
	memset(mem->ppu_regs, 0, 8);
	mem->cart = cart;
}

void write_cpu_byte(struct memory *mem, uint16_t addr, uint8_t byte)
{
	if (addr <= 0x1FFF) {
		mem->cpu_ram[addr & 0x7FF] = byte;
	} else if (addr >= 0x2000 && addr <= 0x3FFF) {
		mem->ppu_regs[addr & 0x7] = byte;
	} else if (addr >= 0x4000 && addr <= 0x4017) {
	} else if (addr >= 0x4020) {
		write_cpu_byte_cartridge(mem->cart, addr, byte);
	}
}

uint8_t read_cpu_byte(struct memory *mem, uint16_t addr)
{
	if (addr <= 0x1FFF) {
		return mem->cpu_ram[addr & 0x7FF];
	} else if (addr >= 0x2000 && addr <= 0x3FFF) {
		return mem->ppu_regs[addr & 0x7];
	} else if (addr >= 0x4000 && addr <= 0x4017) {
		return 0;
	} else if (addr >= 0x4020) {
		return read_cpu_byte_cartridge(mem->cart, addr);
	}
	
	printf("ERROR read_byte got to end of function\n");
	return 0;
}

void write_ppu_byte(struct memory *mem, uint16_t addr, uint8_t byte)
{
	if (addr <= 0x1FFF) {
				
	} else if (addr >= 0x2000 && addr <= 0x2FFF) {
	} else if (addr >= 0x3000 && addr <= 0x3EFF) {
	} else if (addr >= 0x3F00 && addr <= 0x3FFF) {
	}
}

uint8_t read_ppu_byte(struct memory *mem, uint16_t addr)
{
	if (addr <= 0x1FFF) {				
	} else if (addr >= 0x2000 && addr <= 0x2FFF) {
	} else if (addr >= 0x3000 && addr <= 0x3EFF) {
	} else if (addr >= 0x3F00 && addr <= 0x3FFF) {
	}
	return 0;
}
