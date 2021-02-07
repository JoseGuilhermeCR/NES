#include "memory.h"
#include "cartridge.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void init_memory(struct memory *mem, struct cartridge *cart)
{
	memset(mem->ram, 0, 2048);
	mem->cart = cart;
}

void write_byte(struct memory *mem, uint16_t addr, uint8_t byte)
{
	if (addr <= 0x1FFF) {
//		printf("write_byte on ram: %04X\n", addr);
		mem->ram[addr & 0x7FF] = byte;
	} else if (addr >= 0x2000 && addr <= 0x3FFF) {
//		printf("write_byte on ppu_regs: %04X\n", addr);
		mem->ppu_regs[addr & 0x7] = byte;
	} else if (addr >= 0x4000 && addr <= 0x4017) {
//		printf("write_byte on apu/io_regs: %04X\n", addr);
	} else if (addr >= 0x4020) {
//		printf("write_byte on cartridge: %04X\n", addr);
		write_cpu_byte_cartridge(mem->cart, addr, byte);
	}
}

uint8_t read_byte(struct memory *mem, uint16_t addr)
{
	if (addr <= 0x1FFF) {
//		printf("read_byte on ram: %04X\n", addr);
		return mem->ram[addr & 0x7FF];
	} else if (addr >= 0x2000 && addr <= 0x3FFF) {
//		printf("read_byte on ppu_regs: %04X\n", addr);
		return mem->ppu_regs[addr & 0x7];
	} else if (addr >= 0x4000 && addr <= 0x4017) {
//		printf("read_byte on apu/io_regs: %04X\n", addr);
		return 0;
	} else if (addr >= 0x4020) {
//		printf("read_byte on cartridge: %04X\n", addr);
		return read_cpu_byte_cartridge(mem->cart, addr);
	}
	
	printf("ERROR read_byte got to end of function\n");
	return 0;
}
