#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"

#define NROM_128_MASK 0x3FFF
#define NROM_256_MASK 0x7FFF

Mapper mappers[] = {
    {Mapper0CpuWrite, Mapper0CpuRead}
};

void CartridgeDestroy(Cartridge *cart) {
    cart->mapper = NULL;

    free(cart->prg);
    cart->prg = NULL;

    free(cart->chr);
    cart->chr = NULL;

    cart->prgBanks = 0;
    cart->chrBanks = 0;
}

void WriteCpuByteCartridge(Cartridge *cart, uint16_t addr,
                  uint8_t byte) {
    uint32_t decoded = cart->mapper->mapCpuWrite(cart, addr);
    cart->prg[decoded] = byte;
}

uint8_t ReadCpuByteCartridge(Cartridge *cart, uint16_t addr) {
    uint32_t decoded = cart->mapper->mapCpuRead(cart, addr);
    return cart->prg[decoded];
}

uint32_t Mapper0CpuWrite(Cartridge *cart, uint16_t addr) {
    return addr & (cart->prgBanks == 1 ? NROM_128_MASK : NROM_256_MASK);
}

uint32_t Mapper0CpuRead(Cartridge *cart, uint16_t addr) {
    return addr & (cart->prgBanks == 1 ? NROM_128_MASK : NROM_256_MASK);
}
