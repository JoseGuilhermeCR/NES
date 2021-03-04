#ifndef __CARTRIDGE_H__
#define __CARTRIDGE_H__

#include <stdint.h>

struct cartridge {
	uint8_t prg_banks;
	uint8_t chr_banks;
	uint8_t *prg;
	uint8_t *chr;
	struct mapper *mapper;
};

void write_cpu_byte_cartridge(struct cartridge *cart, uint16_t addr, uint8_t byte);
uint8_t read_cpu_byte_cartridge(struct cartridge *cart, uint16_t addr);

void destroy_cartridge(struct cartridge *cart);

struct mapper {
	uint32_t (*map_cpu_write)(struct cartridge*, uint16_t);
	uint32_t (*map_cpu_read)(struct cartridge*, uint16_t);
};

uint32_t mapper0_cpu_write(struct cartridge*, uint16_t);
uint32_t mapper0_cpu_read(struct cartridge*, uint16_t);

extern struct mapper mappers[1];

#endif
