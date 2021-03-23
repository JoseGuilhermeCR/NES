#ifndef NES_H_
#define NES_H_

#include <SDL2/SDL.h>
#include "cpu.h"
#include "ppu.h"
#include "memory.h"

typedef struct _NesWindow {
    uint32_t scale;
    uint32_t width;
    uint32_t height;

    SDL_Window *window;
    SDL_Renderer *renderer;
} NesWindow;

typedef struct _Nes {
    uint8_t debug;

    NesWindow nesWindow;
    Cpu cpu;
    Ppu ppu;
    Memory mem;

    uint8_t paused;
    uint8_t running;

    uint64_t totalCycles;
} Nes;

void NesInit(Nes *nes);
void NesEmulate(Nes *nes);
void NesDestroy(Nes *nes);

void NesWindowInit(NesWindow *window);
void NesWindowDestroy(NesWindow *window);

#endif
