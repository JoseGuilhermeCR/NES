#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"
#include "nes.h"
#include "ppu.h"

// TODO: Refactor cartridge logic.

#define KIB_16 16 * 1024
#define KIB_8  8  * 1024

static Cartridge *FileToCart(const char *filename) {
    Cartridge *cart = malloc(sizeof(Cartridge));

    FILE *rom = fopen(filename, "r");

    /* Just read nestest for now. */
    cart->mapper = &mappers[0];

    cart->prgBanks = 1; /* In 16KiB */
    cart->chrBanks = 1; /* In 8KiB */
    cart->prg = malloc(cart->prgBanks * KIB_16);
    cart->chr = malloc(cart->chrBanks * KIB_8);

    fseek(rom, 16, SEEK_SET);
    fread(cart->prg, 1, cart->prgBanks * KIB_16, rom);
    fread(cart->chr, 1, cart->chrBanks * KIB_8, rom);

    fclose(rom);

    return cart;
}

void NesInit(Nes *nes) {
    nes->paused = 0;
    nes->running = 1;
    nes->debug = 1;
    nes->totalCycles = 0;

    NesWindowInit(&nes->nesWindow);

    Cartridge *cart = FileToCart("test_roms/nestest.nes");

    MemoryInit(&nes->mem, cart, &nes->totalCycles);
    CpuInit(&nes->cpu, &nes->mem, &nes->totalCycles);
    PpuInit(&nes->ppu, &nes->mem, &nes->totalCycles);

}

void NesEmulate(Nes *nes) {
    uint8_t finishedInstruction;
    
    while (nes->running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                nes->running = 0;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_p)
                    nes->paused = !nes->paused;
            }
        }

        if (!nes->paused) {
            MemoryClearReadFlags(&nes->mem);
            
            finishedInstruction = CpuEmulate(&nes->cpu);

            PpuEmulate(&nes->ppu);
            PpuEmulate(&nes->ppu);
            PpuEmulate(&nes->ppu);

            if (nes->ppu.needsNmi) {
                CpuRequestInterrupt(&nes->cpu, NMI);
                nes->ppu.needsNmi = 0;
            }

            if (finishedInstruction)
                printf("PPU: %i, %i CYC:%li\n", nes->ppu.scanline, nes->ppu.cycle, nes->totalCycles);  
        }

        SDL_SetRenderDrawColor(nes->nesWindow.renderer, 0, 0, 0, 255);
        SDL_RenderClear(nes->nesWindow.renderer);

        //SDL_SetRenderDrawColor(nes.renderer, 128, 128, 128, 255);
        //SDL_RenderFillRect(nes.renderer, &NES_RECT);
        SDL_RenderPresent(nes->nesWindow.renderer);

        //SDL_Delay(16);
    }
}

void NesDestroy(Nes *nes) {
    NesWindowDestroy(&nes->nesWindow);
    CartridgeDestroy(nes->mem.cart);
    free(nes->mem.cart);
}

void NesWindowInit(NesWindow *window) {
    SDL_Init(SDL_INIT_VIDEO);

    window->scale = 4;
    window->width = 256 * window->scale;
    window->height = 240 * window->scale;

    window->window = SDL_CreateWindow(
            "NES",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            window->width,
            window->height,
            0
    );

    window->renderer = SDL_CreateRenderer(
            window->window,
            -1,
            SDL_RENDERER_ACCELERATED
    );

}

void NesWindowDestroy(NesWindow *window) {
    SDL_DestroyRenderer(window->renderer);
    SDL_DestroyWindow(window->window);
    SDL_Quit();
}

int32_t main() {
    Nes nes;

    NesInit(&nes);
    NesEmulate(&nes);
    NesDestroy(&nes);

    return 0;
}
