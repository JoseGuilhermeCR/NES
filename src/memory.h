#ifndef MEMORY_H_
#define MEMORY_H_

#include <bits/stdint-uintn.h>
#include <stdint.h>

#define CPU_RAM_SIZE 2048
#define PPU_REGS_SIZE 8
#define PPU_RAM_SIZE 2000

#define PPUCTRL_BASE_NAMETABLE_ADDR_BITS      0x03
#define PPUCTRL_VRAM_ADDR_INCREMENT_BIT       0x04
#define PPUCTRL_SPRITE_PATTER_TABLE_ADDR_BIT  0x08
#define PPUCTRL_BG_PATTERN_TABLE_ADDR_BIT     0x10
#define PPUCTRL_SPRITE_SIZE_BIT               0x20
#define PPUCTRL_MASTER_SLAVE_SELECT_BIT       0x40
#define PPUCTRL_GENERATE_NMI_AT_VBLANK_BIT    0x80

#define PPUMASK_GREYSCALE_BIT                0x01
#define PPUMASK_BG_LEFTMOST_8PIXELS_BIT      0x02
#define PPUMASK_SPRITES_LEFTMOST_8PIXELS_BIT 0x04
#define PPUMASK_BG_BIT                       0x08
#define PPUMASK_SPRITES_BIT                  0x10
#define PPUMASK_EMPHASIZE_RED                0x20
#define PPUMASK_EMPHASIZE_GREEN              0x40
#define PPUMASK_EMPHASIZE_BLUE               0x80

#define PPUSTATUS_SPRITE_OVERFLOW_BIT        0x20
#define PPUSTATUS_SPRITE_0_HIT_BIT           0x40
#define PPUSTATUS_VERTICAL_BLANK_STARTED_BIT 0x80

typedef struct _Cartridge Cartridge;

typedef struct _Memory {
    uint8_t cpuRam[CPU_RAM_SIZE];
    uint8_t ppuRegs[PPU_REGS_SIZE];
    uint8_t ppuRam[PPU_RAM_SIZE];

    uint8_t ppustatusRead;
    Cartridge *cart;
    uint64_t *totalCycles;
} Memory;

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

void MemoryInit(Memory *mem, Cartridge *cart, uint64_t *totalCycles);
void MemoryClearReadFlags(Memory *mem);

void WriteCpuByte(Memory *mem, uint16_t addr, uint8_t byte);
uint8_t ReadCpuByte(Memory *mem, uint16_t addr);

void WritePpuByte(Memory *mem, uint16_t addr, uint8_t byte);
uint8_t ReadPpuByte(Memory *mem, uint16_t addr);

void SetPpuRegisterBit(Memory *mem, uint16_t addr, uint8_t bit, uint8_t active);
uint8_t GetPpuRegisterBit(Memory *mem, uint16_t addr, uint8_t bit);

#endif
