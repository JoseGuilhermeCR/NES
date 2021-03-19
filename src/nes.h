#ifndef NES_H_
#define NES_H_

#include <SDL2/SDL.h>
#include "cpu.h"
#include "ppu.h"
#include "memory.h"

typedef struct _Nes {
    uint8_t debug;

    SDL_Window *window;
    SDL_Renderer *renderer;

    Cpu cpu;
    Ppu ppu;
    Memory mem;

    uint8_t paused;
    uint8_t running;

    uint64_t totalCycles;
} Nes;

const uint32_t SCALE = 4;
const uint32_t WIDTH = 256 * SCALE; 
const uint32_t HEIGHT = 240 * SCALE;

#endif
