#ifndef __CARTRIDGE_H__
#define __CARTRIDGE_H__

#include <stdint.h>

struct Cartridge {
	uint8_t prg_banks;
	uint8_t chr_banks;
	uint8_t *prg;
	uint8_t *chr;
	struct Mapper *mapper;
};

struct Mapper {
	uint32_t (*map_cpu_write)(struct Cartridge*, uint16_t);
	uint32_t (*map_cpu_read)(struct Cartridge*, uint16_t);
};

void write_cpu_byte_cartridge(struct Cartridge *cart, uint16_t addr,
			      uint8_t byte);
uint8_t read_cpu_byte_cartridge(struct Cartridge *cart, uint16_t addr);

void cartridge_destroy(struct Cartridge *cart);

uint32_t mapper0_cpu_write(struct Cartridge*, uint16_t);
uint32_t mapper0_cpu_read(struct Cartridge*, uint16_t);

extern struct Mapper mappers[1];

#endif
