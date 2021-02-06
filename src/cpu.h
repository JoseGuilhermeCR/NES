#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

struct memory;

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

struct registers {
	uint16_t pc; /* Program Counter */
	uint8_t  sp; /* Stack Pointer */
	uint8_t   a; /* Accumulator */
	uint8_t   x; /* X Index Register */
	uint8_t   y; /* Y Index Register */
	uint8_t   s; /* Status register */
};

struct cpu {
	struct registers regs;
	struct memory *mem;

	uint8_t interrupt;
	uint8_t cycles;
};

void init_cpu(struct cpu *cpu, struct memory *mem);
void emulate_cpu(struct cpu *cpu);

#endif
