#include "cpu.h"
#include "ppu.h"
#include "memory.h"
#include "cartridge.h"

#include <stdio.h>
#include <stdlib.h>

const uint32_t KIB_16 = 16 * 1024;
const uint32_t KIB_8 = 8 * 1024;

static void file_to_cart(struct cartridge *cart, const char *filename)
{
	FILE *rom = fopen(filename, "r");

	/* Just read nestest for now. */
	cart->mapper = &mappers[0];

	cart->prg_banks = 1; /* In 16KiB */
	cart->chr_banks = 1; /* In 8KiB */
	cart->prg = malloc(cart->prg_banks * KIB_16);
	cart->chr = malloc(cart->chr_banks * KIB_8);

	fseek(rom, 16, SEEK_SET);
	fread(cart->prg, 1, cart->prg_banks * KIB_16, rom);
	fread(cart->chr, 1, cart->chr_banks * KIB_8, rom);

	fclose(rom);
}

int main()
{
	struct cpu cpu;
	struct ppu ppu;
	struct memory mem;
	struct cartridge cart;

	file_to_cart(&cart, "test_roms/nestest.nes");

	init_memory(&mem, &cart);
	init_cpu(&cpu, &mem);
	init_ppu(&ppu, &mem);

	for (;;) {
		emulate_cpu(&cpu);
	}

	destroy_cartridge(&cart);
}
