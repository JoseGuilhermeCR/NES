#include "nes.h"
#include "cartridge.h"
#include "SDL_FontCache.h"

#include <stdio.h>
#include <stdlib.h>

const uint32_t KIB_16 = 16 * 1024;
const uint32_t KIB_8 = 8 * 1024;

void debug_cpu(struct nes *nes, FC_Font *font)
{
	FC_Draw(
			font,
			nes->renderer,
			CPU_RECT.x,
			CPU_RECT.y + FC_GetHeight(font, "CPU STATUS"),
			"PC: %04X\nSTACK POINTER: %02X STATUS: %02X\n"
			"ACCUMULATOR: %02X X: %02X Y: %02X\n\n\n"
			"IRQ: %02X NMI: %02X RESET: %02X\n"
			"CURRENT_CYCLE: %i NEEDED_CYCLES: %i",
			nes->cpu.regs.pc, nes->cpu.regs.sp, nes->cpu.regs.s,
			nes->cpu.regs.a, nes->cpu.regs.x, nes->cpu.regs.y,
			nes->cpu.interrupt & (uint8_t)IRQ,
			nes->cpu.interrupt & (uint8_t)NMI,
			nes->cpu.interrupt & (uint8_t)RESET,
			nes->cpu.current_cycle, nes->cpu.cycles
	);
}

void debug_instruction(struct nes *nes, FC_Font *font)
{

}

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
	SDL_Init(SDL_INIT_VIDEO);
	struct nes nes;
	struct cartridge cart;

	nes.paused = 0;
	nes.running = 1;
	nes.debug = 1;

	uint32_t window_width = NES_RECT.w;
	uint32_t window_height = NES_RECT.h;
	if (nes.debug)
		window_width += OFFSET + INSTRUCTION_RECT.w;

	nes.window = SDL_CreateWindow(
			"NES",
			SDL_WINDOWPOS_CENTERED,
			SDL_WINDOWPOS_CENTERED,
			window_width,
			window_height,
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

	FC_Font *font = NULL;
	if (nes.debug) {
		font = FC_CreateFont();
		FC_LoadFont(
				font,
				nes.renderer,
				"fonts/DroidSans-Bold.ttf",
				20,
				FC_MakeColor(255, 255, 0, 255),
				TTF_STYLE_NORMAL
			   );
	}

	file_to_cart(&cart, "test_roms/nestest.nes");

	init_memory(&nes.mem, &cart);
	init_cpu(&nes.cpu, &nes.mem);
	init_ppu(&nes.ppu, &nes.mem);

//	nes.cpu.regs.pc = 0xC000;
//	nes.cpu.interrupt = 0;

	while (nes.running) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				nes.running = 0;
			else if (e.type == SDL_KEYDOWN)
				if (e.key.keysym.sym == SDLK_p)
					nes.paused = !nes.paused;
		}

		if (!nes.paused) {
			emulate_cpu(&nes.cpu);

			emulate_ppu(&nes.ppu);
			emulate_ppu(&nes.ppu);
			emulate_ppu(&nes.ppu);
		}

		SDL_SetRenderDrawColor(nes.renderer, 0, 0, 0, 255);
		SDL_RenderClear(nes.renderer);

		SDL_SetRenderDrawColor(nes.renderer, 128, 128, 128, 255);
		SDL_RenderFillRect(nes.renderer, &NES_RECT);

		if (nes.debug) {
			// TODO: Move stuff that only need to be drawn once outside of loop.
			SDL_SetRenderDrawColor(nes.renderer, 0, 0, 128, 255);
			SDL_RenderFillRect(nes.renderer, &INSTRUCTION_RECT);
			SDL_SetRenderDrawColor(nes.renderer, 0, 128, 128, 255);
			SDL_RenderFillRect(nes.renderer, &CPU_RECT);

			FC_Draw(
					font,
					nes.renderer,
					INSTRUCTION_RECT.x,
					0,
					"LDA $#40\nDE81  95 00     STA $00,X @ 55 = FF"
			);
			// Only Drawn Once
			FC_Draw(
					font,
					nes.renderer,
					CPU_RECT.x + CPU_RECT.w / 2 - FC_GetWidth(font, "CPU STATUS") / 2,
					CPU_RECT.y,
					"CPU STATUS"
		        );
			debug_cpu(&nes, font);
		}
		SDL_RenderPresent(nes.renderer);

		SDL_Delay(16);
	}

	if (nes.debug)
		FC_FreeFont(font);

	SDL_DestroyRenderer(nes.renderer);
	SDL_DestroyWindow(nes.window);
	SDL_Quit();

	destroy_cartridge(&cart);

	return 0;
}
