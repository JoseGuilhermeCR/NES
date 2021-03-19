#ifndef PPU_H_
#define PPU_H_

#include <stdint.h>

typedef struct _Memory Memory;

typedef enum _PpuRegisters {
    PPUCTRL   = 0x2000,
    PPUMASK   = 0x2001,
    PPUSTATUS = 0x2002,
    OAMADDR   = 0x2003,
    OAMDATA   = 0x2004,
    PPUSCROLL = 0x2005,
    PPUADDR   = 0x2006,
    PPUDATA   = 0x2007,
    OAMDMA    = 0x4014
} PpuRegisters;

typedef struct _Ppu {
    Memory *mem;

    uint8_t oddFrame;
    uint16_t scanline;
    uint16_t cycle;
} Ppu;

void PpuInit(Ppu *ppu, Memory *mem);
void PpuEmulate(Ppu *ppu);

#endif
