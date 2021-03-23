#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "cartridge.h"

#define RAM_ADDR_END       0x1FFF
#define REAL_RAM_END       0x07FF
#define PPU_ADDR_BEG       0x2000
#define PPU_ADDR_END       0x3FFF
#define REAL_PPU_END       0x0007
#define AUDIO_IO_ADDR_BEG  0x4000
#define AUDIO_IO_ADDR_END  0x4017
#define CARTRIDGE_ADDR_BEG 0x4020

#define PATTERN_TABLE_ADDR_END 0x1FFF
#define NAMETABLE_ADDR_BEG     0x2000
#define NAMETABLE_ADDR_END     0x2FFF
#define NAMETABLE_MIRROR_BEG   0x3000
#define NAMETABLE_MIRROR_END   0x3EFF
#define PALETTE_ADDR_BEG       0x3F00
#define PALETTE_ADDR_END       0x3FFF

void MemoryInit(Memory *mem, Cartridge *cart, uint64_t *totalCycles) {
    memset(mem->cpuRam, 0, CPU_RAM_SIZE);
    memset(mem->ppuRam, 0, PPU_RAM_SIZE);
    memset(mem->ppuRegs, 0, PPU_REGS_SIZE);

    mem->ppustatusRead = 0;
    mem->cart = cart;
    mem->totalCycles = totalCycles;
}

void MemoryClearReadFlags(Memory *mem) {
    mem->ppustatusRead = 0;
}

void WriteCpuByte(Memory *mem, uint16_t addr, uint8_t byte) {
    if (addr <= RAM_ADDR_END) {
        mem->cpuRam[addr & REAL_RAM_END] = byte;
    } else if (addr >= PPU_ADDR_BEG && addr <= PPU_ADDR_END) {
        mem->ppuRegs[addr & REAL_PPU_END] = byte;
    } else if (addr >= AUDIO_IO_ADDR_BEG && addr <= AUDIO_IO_ADDR_END) {
    } else if (addr >= CARTRIDGE_ADDR_BEG) {
        WriteCpuByteCartridge(mem->cart, addr, byte);
    }
}

uint8_t ReadCpuByte(Memory *mem, uint16_t addr) {
    if (addr <= RAM_ADDR_END) {
        return mem->cpuRam[addr & REAL_RAM_END];
    } else if (addr >= PPU_ADDR_BEG && addr <= PPU_ADDR_END) {
        mem->ppustatusRead = addr == PPUSTATUS;

        return mem->ppuRegs[addr & REAL_PPU_END];
    } else if (addr >= AUDIO_IO_ADDR_BEG && addr <= AUDIO_IO_ADDR_END) {
        return 0;
    } else if (addr >= CARTRIDGE_ADDR_BEG) {
        return ReadCpuByteCartridge(mem->cart, addr);
    }
    
    printf("ERROR read_byte got to end of function\n");
    return 0;
}

// TODO: Finish this.
void WritePpuByte(Memory *mem, uint16_t addr, uint8_t byte) {
    if (addr <= PATTERN_TABLE_ADDR_END) {
        WritePpuByteCartridge(mem->cart, addr, byte);;
    } else if (addr >= NAMETABLE_ADDR_BEG && addr <= NAMETABLE_ADDR_END) {
        mem->ppuRam[addr] = byte;
    } else if (addr >= NAMETABLE_MIRROR_BEG && addr <= NAMETABLE_MIRROR_END) {
        // TODO:
        //mem->ppuRam[addr] = byte;
    } else if (addr >= PALETTE_ADDR_BEG && addr <= PALETTE_ADDR_BEG) {
        //mem->ppuRam[addr] = byte;
    }
}

uint8_t ReadPpuByte(Memory *mem, uint16_t addr) {
    if (addr <= PATTERN_TABLE_ADDR_END) {
        return ReadPpuByteCartridge(mem->cart, addr);
    } else if (addr >= NAMETABLE_ADDR_BEG && addr <= NAMETABLE_ADDR_END) {
        return mem->ppuRam[addr];
    } else if (addr >= NAMETABLE_MIRROR_BEG && addr <= NAMETABLE_MIRROR_END) {
        // TODO:
        //return mem->ppuRam[addr];
    } else if (addr >= PALETTE_ADDR_BEG && addr <= PALETTE_ADDR_BEG) {
        return mem->ppuRam[addr];
    }
    return 0;
}

void SetPpuRegisterBit(Memory *mem, uint16_t addr, uint8_t bit, uint8_t active) {
    // After power/reset, writes to this register are ignored for about 30,000 cycles. 
    if (addr == PPUCTRL && *(mem->totalCycles) <= 30000)
        return;

    if (active)
        mem->ppuRegs[addr & REAL_PPU_END] |= bit;
    else
        mem->ppuRegs[addr & REAL_PPU_END] &= ~bit;
}

uint8_t GetPpuRegisterBit(Memory *mem, uint16_t addr, uint8_t bit) {
    return (mem->ppuRegs[addr & REAL_PPU_END] & bit) != 0;
}