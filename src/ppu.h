#ifndef PPU_H_
#define PPU_H_

#include <stdint.h>

typedef struct _Memory Memory;

typedef struct _Ppu {
    Memory *mem;

    uint8_t oddFrame;
    uint16_t scanline;
    uint16_t cycle;
    uint64_t *totalCycles;

    uint8_t needsNmi;
} Ppu;

void PpuInit(Ppu *ppu, Memory *mem, uint64_t *totalCycles);
void PpuEmulate(Ppu *ppu);

#endif
