#ifndef __NES_H__
#define __NES_H__

#include <SDL2/SDL.h>
#include "cpu.h"
#include "ppu.h"
#include "memory.h"

struct nes {
	uint8_t debug;

	SDL_Window *window;
	SDL_Renderer *renderer;

	struct Cpu cpu;
	struct Ppu ppu;
	struct Memory mem;

	uint8_t paused;
	uint8_t running;
};

const uint32_t SCALE = 4;
const uint32_t WIDTH = 256 * SCALE; 
const uint32_t HEIGHT = 240 * SCALE;

#endif
