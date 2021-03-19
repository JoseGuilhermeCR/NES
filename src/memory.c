#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "cartridge.h"

#define RAM_ADDRESS_END       0x1FFF
#define REAL_RAM_END          0x7FF
#define PPU_ADDRESS_BEG       0x2000
#define PPU_ADDRESS_END       0x3FFF
#define REAL_PPU_END          0x7
#define AUDIO_IO_ADDRESS_BEG  0x4000
#define AUDIO_IO_ADDRESS_END  0x4017
#define CARTRIDGE_ADDRESS_BEG 0x4020

void MemoryInit(Memory *mem, Cartridge *cart) {
    memset(mem->cpuRam, 0, CPU_RAM_SIZE);
    memset(mem->ppuRam, 0, PPU_RAM_SIZE);
    memset(mem->ppuRegs, 0, PPU_REGS_SIZE);
    mem->cart = cart;
}

void WriteCpuByte(Memory *mem, uint16_t addr, uint8_t byte) {
    if (addr <= RAM_ADDRESS_END) {
        mem->cpuRam[addr & REAL_RAM_END] = byte;
    } else if (addr >= PPU_ADDRESS_BEG && addr <= PPU_ADDRESS_END) {
        mem->ppuRegs[addr & REAL_PPU_END] = byte;
    } else if (addr >= AUDIO_IO_ADDRESS_BEG && addr <= AUDIO_IO_ADDRESS_END) {
    } else if (addr >= CARTRIDGE_ADDRESS_BEG) {
        WriteCpuByteCartridge(mem->cart, addr, byte);
    }
}

uint8_t ReadCpuByte(const  Memory *mem, uint16_t addr) {
    if (addr <= RAM_ADDRESS_END) {
        return mem->cpuRam[addr & REAL_RAM_END];
    } else if (addr >= PPU_ADDRESS_BEG && addr <= PPU_ADDRESS_END) {
        return mem->ppuRegs[addr & REAL_PPU_END];
    } else if (addr >= AUDIO_IO_ADDRESS_BEG && addr <= AUDIO_IO_ADDRESS_END) {
        return 0;
    } else if (addr >= CARTRIDGE_ADDRESS_BEG) {
        return ReadCpuByteCartridge(mem->cart, addr);
    }
    
    printf("ERROR read_byte got to end of function\n");
    return 0;
}

// TODO: DEFINES FOR PPU MEMORY SPACE.
void WritePpuByte(Memory *mem, uint16_t addr, uint8_t byte) {
    if (addr <= 0x1FFF) {
                
    } else if (addr >= 0x2000 && addr <= 0x2FFF) {
    } else if (addr >= 0x3000 && addr <= 0x3EFF) {
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
    }
}

uint8_t ReadPpuByte(const Memory *mem, uint16_t addr)
{
    if (addr <= 0x1FFF) {				
    } else if (addr >= 0x2000 && addr <= 0x2FFF) {
    } else if (addr >= 0x3000 && addr <= 0x3EFF) {
    } else if (addr >= 0x3F00 && addr <= 0x3FFF) {
    }
    return 0;
}
