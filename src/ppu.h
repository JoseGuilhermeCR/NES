#ifndef PPU_H_
#define PPU_H_

#include <stdint.h>

#define OAM_ENTRY_NUM 64

typedef struct _Memory Memory;

typedef struct _OAMEntry {
    uint8_t y;
    uint8_t tileNumber;
    uint8_t spriteAttr;
    uint8_t x;
} OAMEntry;

typedef struct _Ppu {
    Memory *mem;
    OAMEntry oamMemory[OAM_ENTRY_NUM];

    uint8_t oddFrame;
    uint16_t scanline;
    uint16_t cycle;
    uint64_t *totalCycles;

    uint8_t needsNmi;
} Ppu;

void PpuInit(Ppu *ppu, Memory *mem, uint64_t *totalCycles);
void PpuEmulate(Ppu *ppu);

#endif
