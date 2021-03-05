#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"

struct Mapper mappers[1] = {
	{mapper0_cpu_write, mapper0_cpu_read}
};

void cartridge_destroy(struct Cartridge *cart)
{
	cart->mapper = NULL;

	free(cart->prg);
	cart->prg = NULL;

	free(cart->chr);
	cart->chr = NULL;

	cart->prg_banks = 0;
	cart->chr_banks = 0;
}

void write_cpu_byte_cartridge(struct Cartridge *cart, uint16_t addr,
			      uint8_t byte)
{
	uint32_t decoded = cart->mapper->map_cpu_write(cart, addr);
	cart->prg[decoded] = byte;
}

uint8_t read_cpu_byte_cartridge(struct Cartridge *cart, uint16_t addr)
{
	uint32_t decoded = cart->mapper->map_cpu_read(cart, addr);
	return cart->prg[decoded];
}

uint32_t mapper0_cpu_write(struct Cartridge *cart, uint16_t addr)
{
	return addr & (cart->prg_banks == 1 ? 0x3FFF : 0x7FFF);
}

uint32_t mapper0_cpu_read(struct Cartridge *cart, uint16_t addr)
{
	return addr & (cart->prg_banks == 1 ? 0x3FFF : 0x7FFF);
}
