#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>

#define CPU_RAM_SIZE 2048
#define PPU_REGS_SIZE 8
#define PPU_RAM_SIZE 2000

typedef struct _Cartridge Cartridge;

typedef struct _Memory {
    uint8_t cpuRam[CPU_RAM_SIZE];
    uint8_t ppuRegs[PPU_REGS_SIZE];
    uint8_t ppuRam[PPU_RAM_SIZE];
    Cartridge *cart;
} Memory;

void MemoryInit(Memory *mem, Cartridge *cart);

void WriteCpuByte(Memory *mem, uint16_t addr, uint8_t byte);
uint8_t ReadCpuByte(const Memory *mem, uint16_t addr);

void WritePpuByte(Memory *mem, uint16_t addr, uint8_t byte);
uint8_t ReadPpuByte(const Memory *mem, uint16_t addr);

#endif
