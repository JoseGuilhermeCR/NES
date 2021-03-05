#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

struct Memory;

enum Status {
	CARRY             = 0x01,
	ZERO              = 0x02,
	INTERRUPT_DISABLE = 0x04,
	DECIMAL           = 0x08,
	BREAK             = 0x10,
	OVERFLOW          = 0x40,
	NEGATIVE          = 0x80
};

enum Interrupt {
	IRQ   = 0x01,
	NMI   = 0x02,
	RESET = 0x04
};

enum AddressingMode {
	IMPLICIT = 1,
	IMMEDIATE,
	ACCUMULATOR,
	ZEROPAGE,
	ZEROPAGE_X,
	ZEROPAGE_Y,
	RELATIVE,
	ABSOLUTE,
	ABSOLUTE_X,
	ABSOLUTE_Y,
	INDIRECT,
	INDEXED_INDIRECT,
	INDIRECT_INDEXED
};

struct Registers {
	uint16_t pc; /* Program Counter */
	uint8_t  sp; /* Stack Pointer */
	uint8_t   a; /* Accumulator */
	uint8_t   x; /* X Index Register */
	uint8_t   y; /* Y Index Register */
	uint8_t   s; /* Status register */
};

struct Cpu {
	struct Registers regs;
	struct Memory *mem;

	uint8_t interrupt;
	uint8_t cycles;
	uint8_t current_cycle;

	uint64_t total_cycles;
};

void cpu_init(struct Cpu *cpu, struct Memory *mem);
uint8_t cpu_emulate(struct Cpu *cpu);

#endif
