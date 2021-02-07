#ifndef __NES_H__
#define __NES_H__

#include <SDL2/SDL.h>
#include "cpu.h"
#include "ppu.h"
#include "memory.h"

const uint32_t SCALE = 4;
const uint32_t OFFSET = 20;
const SDL_Rect NES_RECT = {0, 0, 256 * SCALE, 240 * SCALE};
const SDL_Rect INSTRUCTION_RECT = {256 * SCALE + OFFSET, 0, 500, 240 * (SCALE - 1)};
const SDL_Rect CPU_RECT = {256 * SCALE + OFFSET, 240 * (SCALE - 1), 500, 240};

struct nes {
	uint8_t debug;

	SDL_Window *window;
	SDL_Renderer *renderer;

	struct cpu cpu;
	struct ppu ppu;
	struct memory mem;

	uint8_t paused;
	uint8_t running;
};

#endif
