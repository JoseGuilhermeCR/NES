#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"
#include "nes.h"

// TODO: Refactor main function and cartridge logic.

static const uint32_t KIB_16 = 16 * 1024;
static const uint32_t KIB_8 = 8 * 1024;

static void
file_to_cart(struct Cartridge *cart, const char *filename)
{
	FILE *rom;
	
	rom = fopen(filename, "r");

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

int32_t
main()
{
	struct nes nes;
	struct Cartridge cart;
	uint8_t finished_instruction;
	SDL_Event e;

	SDL_Init(SDL_INIT_VIDEO);

	nes.paused = 0;
	nes.running = 1;
	nes.debug = 1;

	nes.window = SDL_CreateWindow(
			"NES",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			WIDTH,
			HEIGHT,
			0
	);

	if (!nes.window) {
		printf("Could not create window: %s\n", SDL_GetError());
		return 1;
	}

	nes.renderer = SDL_CreateRenderer(
			nes.window,
			-1,
			SDL_RENDERER_ACCELERATED
	);

	file_to_cart(&cart, "test_roms/nestest.nes");

	memory_init(&nes.mem, &cart);
	cpu_init(&nes.cpu, &nes.mem);
	ppu_init(&nes.ppu, &nes.mem);

//	nes.cpu.regs.pc = 0xC000;
//	nes.cpu.interrupt = 0;

	while (nes.running) {
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				nes.running = 0;
			else if (e.type == SDL_KEYDOWN)
				if (e.key.keysym.sym == SDLK_p)
					nes.paused = !nes.paused;
		}

		if (!nes.paused) {
			finished_instruction = cpu_emulate(&nes.cpu);

			ppu_emulate(&nes.ppu);
			ppu_emulate(&nes.ppu);
			ppu_emulate(&nes.ppu);

			if (finished_instruction)
				printf("PPU: %i, %i CYC:%li\n", nes.ppu.scanline, nes.ppu.cycle, nes.cpu.total_cycles);
				
		}

		SDL_SetRenderDrawColor(nes.renderer, 0, 0, 0, 255);
		SDL_RenderClear(nes.renderer);

		//SDL_SetRenderDrawColor(nes.renderer, 128, 128, 128, 255);
		//SDL_RenderFillRect(nes.renderer, &NES_RECT);
		SDL_RenderPresent(nes.renderer);

		SDL_Delay(16);
	}

	SDL_DestroyRenderer(nes.renderer);
	SDL_DestroyWindow(nes.window);
	SDL_Quit();

	cartridge_destroy(&cart);

	return 0;
}
