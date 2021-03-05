#ifndef __PPU_H__
#define __PPU_H__

#include <stdint.h>

struct Memory;

enum PpuRegisters {
	PPUCTRL   = 0x2000,
	PPUMASK   = 0x2001,
	PPUSTATUS = 0x2002,
	OAMADDR   = 0x2003,
	OAMDATA   = 0x2004,
	PPUSCROLL = 0x2005,
	PPUADDR   = 0x2006,
	PPUDATA   = 0x2007,
	OAMDMA    = 0x4014
};

struct Ppu {
	struct Memory *mem;

	uint8_t odd_frame;
	uint16_t scanline;
	uint16_t cycle;
};

void ppu_init(struct Ppu *ppu, struct Memory *mem);
void ppu_emulate(struct Ppu *ppu);

#endif
