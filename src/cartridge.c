#include <stdio.h>
#include <stdlib.h>

#include "cartridge.h"

#define NROM_128_MASK 0x3FFF
#define NROM_256_MASK 0x7FFF

Mapper mappers[] = {
    {Mapper0CpuWrite, Mapper0CpuRead, Mapper0PpuWrite, Mapper0PpuRead}
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

void WriteCpuByteCartridge(Cartridge *cart, uint16_t addr, uint8_t byte) {
    uint32_t decoded = cart->mapper->mapCpuWrite(cart, addr);
    cart->prg[decoded] = byte;
}

uint8_t ReadCpuByteCartridge(Cartridge *cart, uint16_t addr) {
    uint32_t decoded = cart->mapper->mapCpuRead(cart, addr);
    return cart->prg[decoded];
}

void WritePpuByteCartridge(Cartridge *cart, uint16_t addr, uint8_t byte) {
    uint32_t decoded = cart->mapper->mapPpuRead(cart, addr);
    cart->chr[decoded] = byte;
}

uint8_t ReadPpuByteCartridge(Cartridge *cart, uint16_t addr) {
    uint32_t decoded = cart->mapper->mapPpuRead(cart, addr);
    return cart->chr[decoded];
}

uint32_t Mapper0CpuWrite(Cartridge *cart, uint16_t addr) {
    return addr & (cart->prgBanks == 1 ? NROM_128_MASK : NROM_256_MASK);
}

uint32_t Mapper0CpuRead(Cartridge *cart, uint16_t addr) {
    return addr & (cart->prgBanks == 1 ? NROM_128_MASK : NROM_256_MASK);
}

// Possible FIXME in these two functions.
uint32_t Mapper0PpuWrite(Cartridge *cart, uint16_t addr) {
    return addr;
}

uint32_t Mapper0PpuRead(Cartridge *cart, uint16_t addr) {
    return addr;
}
