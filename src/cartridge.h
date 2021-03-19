#ifndef CARTRIDGE_H_
#define CARTRIDGE_H_

#include <stdint.h>

#include "memory.h"

typedef struct _Mapper Mapper;

typedef struct _Cartridge {
    uint8_t prgBanks;
    uint8_t chrBanks;
    uint8_t *prg;
    uint8_t *chr;
    Mapper *mapper;
} Cartridge;

typedef struct _Mapper {
    uint32_t (*mapCpuWrite)(Cartridge*, uint16_t);
    uint32_t (*mapCpuRead)(Cartridge*, uint16_t);
} Mapper;

void WriteCpuByteCartridge(Cartridge *cart, uint16_t addr,
                  uint8_t byte);
uint8_t ReadCpuByteCartridge(Cartridge *cart, uint16_t addr);

void CartridgeDestroy(Cartridge *cart);

uint32_t Mapper0CpuWrite(Cartridge *, uint16_t);
uint32_t Mapper0CpuRead(Cartridge *, uint16_t);

extern Mapper mappers[1];

#endif
