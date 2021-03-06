#ifndef CPU_H_
#define CPU_H_

#include <stdint.h>

typedef struct _Memory Memory;

typedef enum _STATUS {
    CARRY             = 0x01,
    ZERO              = 0x02,
    INTERRUPT_DISABLE = 0x04,
    DECIMAL           = 0x08,
    BREAK             = 0x10,
    OVERFLOW          = 0x40,
    NEGATIVE          = 0x80
} STATUS;

typedef enum _INTERRUPT {
    IRQ   = 0x01,
    NMI   = 0x02,
    RESET = 0x04
} INTERRUPT;

typedef enum _ADDRESSING_MODE {
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
} ADDRESSING_MODE;

typedef struct _Registers {
    uint16_t pc; // Program Counter
    uint8_t  sp; // Stack Pointer
    uint8_t   a; // Accumulator
    uint8_t   x; // X Index Register
    uint8_t   y; // Y Index Register
    uint8_t   s; // Status register
} Registers;

typedef struct _Cpu {
    Registers regs;
    Memory *mem;

    uint8_t interrupt;
    uint8_t cycles;
    uint8_t currentCycle;

    uint64_t *totalCycles;
} Cpu;

void CpuInit(Cpu *cpu, Memory *mem, uint64_t *totalCycles);
uint8_t CpuEmulate(Cpu *cpu);
void CpuRequestInterrupt(Cpu *cpu, INTERRUPT i);

#endif
