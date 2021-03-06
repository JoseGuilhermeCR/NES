#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "cpu.h"
#include "memory.h"

#define INSTR(x) static void x(struct Cpu *cpu, uint16_t addr)

static uint16_t immediate(struct Cpu *cpu);
static uint16_t zeropage(struct Cpu *cpu);
static uint16_t zeropage_indexed(struct Cpu *cpu, uint8_t index);
static uint16_t absolute(struct Cpu *cpu);
static uint16_t absolute_indexed(struct Cpu *cpu, uint8_t index);
static uint16_t relative(struct Cpu *cpu);
static uint16_t indirect(struct Cpu *cpu);
static uint16_t indexed_indirect(struct Cpu *cpu);
static uint16_t indirect_indexed(struct Cpu *cpu);

INSTR(brk);
INSTR(ora);
INSTR(asl);
INSTR(asl_a);
INSTR(php);
INSTR(bpl);
INSTR(clc);
INSTR(jsr);
INSTR(and);
INSTR(bit);
INSTR(rol);
INSTR(rol_a);
INSTR(plp);
INSTR(bmi);
INSTR(sec);
INSTR(rti);
INSTR(eor);
INSTR(lsr);
INSTR(lsr_a);
INSTR(pha);
INSTR(jmp);
INSTR(bvc);
INSTR(cli);
INSTR(rts);
INSTR(adc);
INSTR(ror);
INSTR(ror_a);
INSTR(pla);
INSTR(bvs);
INSTR(sei);
INSTR(sta);
INSTR(sty);
INSTR(stx);
INSTR(dey);
INSTR(txa);
INSTR(bcc);
INSTR(tya);
INSTR(txs);
INSTR(ldy);
INSTR(lda);
INSTR(ldx);
INSTR(tay);
INSTR(tax);
INSTR(bcs);
INSTR(clv);
INSTR(tsx);
INSTR(cpy);
INSTR(cmp);
INSTR(dec);
INSTR(iny);
INSTR(dex);
INSTR(bne);
INSTR(cld);
INSTR(cpx);
INSTR(sbc);
INSTR(inc);
INSTR(inx);
INSTR(beq);
INSTR(sed);

static void inc_dec_impl(struct Cpu *cpu, uint16_t addr, uint8_t change);
static void cmp_impl(struct Cpu *cpu, uint16_t addr, uint8_t reg);
static void adc_impl(struct Cpu *cpu, uint16_t addr, uint8_t ones_complement);
static void branch(struct Cpu *cpu, uint16_t addr, enum Status status,
		   uint8_t value_needed);

struct Instruction {
	const char *mnemonic;
	uint8_t opcode;
	uint8_t cycles;
	enum AddressingMode adr_mode;
	void (*execute)(struct Cpu* cpu, uint16_t addr);
};

union InstructionOrNothing {
	const void *valid;
	struct Instruction instr;
};

static union AddrDecodeInfo {
	struct {
		uint16_t addr;
	} absolute;

	struct {
		uint16_t absolute_addr;
		uint16_t addr;
		uint8_t index;
	} absolute_indexed;

	struct {
		uint16_t addr;
	} immediate;

	struct {
		uint8_t addr;
	} zeropage;

	struct {
		uint8_t zp_addr;
		uint8_t index;
		uint16_t addr;
	} zeropage_indexed;

	struct {
		uint16_t addr;
	} relative;

	struct {
		uint16_t addr;
		uint16_t iaddr;
	} indirect;

	struct {
		uint16_t addr;
		uint16_t zp_addr;
		uint8_t x;
	} indexed_indirect;

	struct {
		uint16_t addr;
		uint16_t zp_addr;
		uint8_t y;
	} indirect_indexed;
} g_addr_decoded_info;

static const union InstructionOrNothing g_instruction_table[] = {
	{.instr = {"BRK", 0x00, 7, IMPLICIT, brk}},
	{.instr = {"ORA", 0x01, 6, INDEXED_INDIRECT, ora}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"ORA", 0x05, 3, INDEXED_INDIRECT, ora}},
	{.instr = {"ASL", 0x06, 5, ZEROPAGE, asl}},
	{NULL},
	{.instr = {"PHP", 0x08, 3, IMPLICIT, php}},
	{.instr = {"ORA", 0x09, 2, IMMEDIATE, ora}},
	{.instr = {"ASL", 0x0A, 2, ACCUMULATOR, asl_a}},
	{NULL},
	{NULL},
	{.instr = {"ORA", 0x0D, 4, ABSOLUTE, ora}},
	{.instr = {"ASL", 0x0E, 6, ABSOLUTE, asl}},
	{NULL},
	{.instr = {"BPL", 0x10, 2, RELATIVE, bpl}},
	{.instr = {"ORA", 0x11, 5, INDIRECT_INDEXED, ora}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"ORA", 0x15, 4, ZEROPAGE_X, ora}},
	{.instr = {"ASL", 0x16, 6, ZEROPAGE_X, asl}},
	{NULL},
	{.instr = {"CLC", 0x18, 2, IMPLICIT, clc}},
	{.instr = {"ORA", 0x19, 4, ABSOLUTE_Y, ora}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"ORA", 0x1D, 4, ABSOLUTE_X, ora}},
	{.instr = {"ASL", 0x1E, 7, ABSOLUTE_X, asl}},
	{NULL},
	{.instr = {"JSR", 0x20, 6, ABSOLUTE, jsr}},
	{.instr = {"AND", 0x21, 6, INDEXED_INDIRECT, and}},
	{NULL},
	{NULL},
	{.instr = {"BIT", 0x24, 3, ZEROPAGE, bit}},
	{.instr = {"AND", 0x25, 3, ZEROPAGE, and}},
	{.instr = {"ROL", 0x26, 5, ZEROPAGE, rol}},
	{NULL},
	{.instr = {"PLP", 0x28, 4, IMPLICIT, plp}},
	{.instr = {"AND", 0x29, 2, IMMEDIATE, and}},
	{.instr = {"ROL", 0x2A, 2, ACCUMULATOR, rol_a}},
	{NULL},
	{.instr = {"BIT", 0x2C, 4, ABSOLUTE, bit}},
	{.instr = {"AND", 0x2D, 4, ABSOLUTE, and}},
	{.instr = {"AND", 0x2E, 6, ABSOLUTE, rol}},
	{NULL},
	{.instr = {"BMI", 0x30, 2, RELATIVE, bmi}},
	{.instr = {"AND", 0x31, 5, INDIRECT_INDEXED, and}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"AND", 0x35, 4, ZEROPAGE_X, and}},
	{.instr = {"ROL", 0x36, 6, ZEROPAGE_X, rol}},
	{NULL},
	{.instr = {"SEC", 0x38, 2, IMPLICIT, sec}},
	{.instr = {"AND", 0x39, 4, ABSOLUTE_Y, and}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"AND", 0x3D, 4, ABSOLUTE_X, and}},
	{.instr = {"ROL", 0x3E, 7, ABSOLUTE_X, rol}},
	{NULL},
	{.instr = {"RTI", 0x40, 6, IMPLICIT, rti}},
	{.instr = {"EOR", 0x41, 6, INDEXED_INDIRECT, eor}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"EOR", 0x45, 3, ZEROPAGE, eor}},
	{.instr = {"LSR", 0x46, 5, ZEROPAGE, lsr}},
	{NULL},
	{.instr = {"PHA", 0x48, 3, IMPLICIT, pha}},
	{.instr = {"EOR", 0x49, 2, IMMEDIATE, eor}},
	{.instr = {"LSR", 0x4A, 2, ACCUMULATOR, lsr_a}},
	{NULL},
	{.instr = {"JMP", 0x4C, 3, ABSOLUTE, jmp}},
	{.instr = {"EOR", 0x4D, 4, ABSOLUTE, eor}},
	{.instr = {"LSR", 0x4E, 6, ABSOLUTE, eor}},
	{NULL},
	{.instr = {"BVC", 0x50, 2, RELATIVE, bvc}},
	{.instr = {"EOR", 0x51, 5, INDIRECT_INDEXED, eor}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"EOR", 0x55, 4, ZEROPAGE_X, eor}},
	{.instr = {"LSR", 0x56, 6, ZEROPAGE_X, lsr}},
	{NULL},
	{.instr = {"CLI", 0x58, 6, IMPLICIT, cli}},
	{.instr = {"EOR", 0x59, 4, ABSOLUTE_Y, eor}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"EOR", 0x5D, 4, ABSOLUTE_X, eor}},
	{.instr = {"LSR", 0x5E, 7, ABSOLUTE_X, lsr}},
	{NULL},
	{.instr = {"RTS", 0x60, 6, IMPLICIT, rts}},
	{.instr = {"ADC", 0x61, 6, INDEXED_INDIRECT, adc}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"ADC", 0x65, 3, ZEROPAGE, adc}},
	{.instr = {"ROR", 0x66, 5, ZEROPAGE, ror}},
	{NULL},
	{.instr = {"PLA", 0x68, 4, IMPLICIT, pla}},
	{.instr = {"ADC", 0x69, 2, IMMEDIATE, adc}},
	{.instr = {"ROR", 0x6A, 2, ACCUMULATOR, ror_a}},
	{NULL},
	{.instr = {"JMP", 0x6C, 5, INDIRECT, jmp}},
	{.instr = {"ADC", 0x6D, 4, ABSOLUTE, adc}},
	{.instr = {"ROR", 0x6E, 6, ABSOLUTE, ror}},
	{NULL},
	{.instr = {"BVS", 0x70, 2, RELATIVE, bvs}},
	{.instr = {"ADC", 0x71, 5, INDIRECT_INDEXED, adc}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"ADC", 0x75, 4, ZEROPAGE_X, adc}},
	{.instr = {"ROR", 0x76, 6, ZEROPAGE_X, ror}},
	{NULL},
	{.instr = {"SEI", 0x78, 2, IMPLICIT, sei}},
	{.instr = {"ADC", 0x79, 3, ABSOLUTE_Y, adc}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"ADC", 0x7D, 3, ABSOLUTE_X, adc}},
	{.instr = {"ROR", 0x7E, 7, ABSOLUTE_X, ror}},
	{NULL},
	{NULL},
	{.instr = {"STA", 0x81, 6, INDEXED_INDIRECT, sta}},
	{NULL},
	{NULL},
	{.instr = {"STY", 0x84, 3, ZEROPAGE, sty}},
	{.instr = {"STA", 0x85, 3, ZEROPAGE, sta}},
	{.instr = {"STX", 0x86, 3, ZEROPAGE, stx}},
	{NULL},
	{.instr = {"DEY", 0x88, 2, IMPLICIT, dey}},
	{NULL},
	{.instr = {"TXA", 0x8A, 2, IMPLICIT, txa}},
	{NULL},
	{.instr = {"STY", 0x8C, 4, ABSOLUTE, sty}},
	{.instr = {"STA", 0x8D, 4, ABSOLUTE, sta}},
	{.instr = {"STX", 0x8E, 4, ABSOLUTE, stx}},
	{NULL},
	{.instr = {"BCC", 0x90, 2, RELATIVE, bcc}},
	{.instr = {"STA", 0x91, 6, INDIRECT_INDEXED, sta}},
	{NULL},
	{NULL},
	{.instr = {"STY", 0x94, 4, ZEROPAGE_X, sty}},
	{.instr = {"STA", 0x95, 4, ZEROPAGE_X, sta}},
	{.instr = {"STX", 0x96, 4, ZEROPAGE_Y, stx}},
	{NULL},
	{.instr = {"TYA", 0x98, 2, IMPLICIT, tya}},
	{.instr = {"STA", 0x99, 5, ABSOLUTE_Y, sta}},
	{.instr = {"TXS", 0x9A, 2, IMPLICIT, txs}},
	{NULL},
	{NULL},
	{.instr = {"STA", 0x9D, 5, ABSOLUTE_X, sta}},
	{NULL},
	{NULL},
	{.instr = {"LDY", 0xA0, 2, IMMEDIATE, ldy}},
	{.instr = {"LDA", 0xA1, 6, INDEXED_INDIRECT, lda}},
	{.instr = {"LDX", 0xA2, 2, IMMEDIATE, ldx}},
	{NULL},
	{.instr = {"LDY", 0xA4, 2, ZEROPAGE, ldy}},
	{.instr = {"LDA", 0xA5, 3, ZEROPAGE, lda}},
	{.instr = {"LDA", 0xA6, 3, ZEROPAGE, ldx}},
	{NULL},
	{.instr = {"TAY", 0xA8, 2, IMPLICIT, tay}},
	{.instr = {"LDA", 0xA9, 2, IMMEDIATE, lda}},
	{.instr = {"TAX", 0xAA, 2, IMPLICIT, tax}},
	{NULL},
	{.instr = {"LDY", 0xAC, 4, ABSOLUTE, ldy}},
	{.instr = {"LDA", 0xAD, 4, ABSOLUTE, lda}},
	{.instr = {"LDX", 0xAE, 4, ABSOLUTE, ldx}},
	{NULL},
	{.instr = {"BCS", 0xB0, 2, RELATIVE, bcs}},
	{.instr = {"LDA", 0xB1, 5, INDIRECT_INDEXED, lda}},
	{NULL},
	{NULL},
	{.instr = {"LDY", 0xB4, 4, ZEROPAGE_X, ldy}},
	{.instr = {"LDA", 0xB5, 4, ZEROPAGE_X, lda}},
	{.instr = {"LDX", 0xB6, 4, ZEROPAGE_Y, ldx}},
	{NULL},
	{.instr = {"CLV", 0xB8, 2, IMPLICIT, clv}},
	{.instr = {"LDA", 0xB9, 4, ABSOLUTE_Y, lda}},
	{.instr = {"TSA", 0xBA, 2, IMPLICIT, tsx}},
	{NULL},
	{.instr = {"LDY", 0xBC, 4, ABSOLUTE_X, ldy}},
	{.instr = {"LDA", 0xBD, 4, ABSOLUTE_X, lda}},
	{.instr = {"LDX", 0xBE, 4, ABSOLUTE_Y, ldx}},
	{NULL},
	{.instr = {"CPY", 0xC0, 2, IMMEDIATE, cpy}},
	{.instr = {"CMP", 0xC1, 6, INDEXED_INDIRECT, cmp}},
	{NULL},
	{NULL},
	{.instr = {"CPY", 0xC4, 3, ZEROPAGE, cpy}},
	{.instr = {"CMP", 0xC5, 3, ZEROPAGE, cmp}},
	{.instr = {"DEC", 0xC6, 5, ZEROPAGE, dec}},
	{NULL},
	{.instr = {"INY", 0xC8, 2, IMPLICIT, iny}},
	{.instr = {"CMP", 0xC9, 2, IMMEDIATE, cmp}},
	{.instr = {"DEX", 0xCA, 2, IMPLICIT, dex}},
	{NULL},
	{.instr = {"CPY", 0xCC, 4, ABSOLUTE, cpy}},
	{.instr = {"CMP", 0xCD, 4, ABSOLUTE, cmp}},
	{.instr = {"DEC", 0xCE, 6, ABSOLUTE, dec}},
	{NULL},
	{.instr = {"BNE", 0xD0, 2, RELATIVE, bne}},
	{.instr = {"CMP", 0xD1, 5, INDIRECT_INDEXED, cmp}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"CMP", 0xD5, 4, ZEROPAGE_X, cmp}},
	{.instr = {"DEC", 0xD6,	6, ZEROPAGE_X, dec}},
	{NULL},
	{.instr = {"CLD", 0xD8,	2, IMPLICIT, cld}},
	{.instr = {"CMP", 0xD9, 4, ABSOLUTE_Y, cmp}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"CMP", 0xDD, 4, ABSOLUTE_X, cmp}},
	{.instr = {"DEC", 0xDE,	7, ABSOLUTE_X, dec}},
	{NULL},
	{.instr = {"CPX", 0xE0, 2, IMMEDIATE, cpx}},
	{.instr = {"SBC", 0xE1, 6, INDEXED_INDIRECT, sbc}},
	{NULL},
	{NULL},
	{.instr = {"CPX", 0xE4, 3, ZEROPAGE, cpx}},
	{.instr = {"SBC", 0xE5, 3, ZEROPAGE, sbc}},
	{.instr = {"INC", 0xE6, 5, ZEROPAGE, inc}},
	{NULL},
	{.instr = {"INX", 0xE8, 2, IMPLICIT, inx}},
	{.instr = {"SBC", 0xE9, 2, IMMEDIATE, sbc}},
	{.instr = {"NOP", 0xEA, 2, IMPLICIT, NULL}}, /* NOP */
	{NULL},
	{.instr = {"CPX", 0xEC, 4, ABSOLUTE, cpx}},
	{.instr = {"SBC", 0xED, 4, ABSOLUTE, sbc}},
	{.instr = {"INC", 0xEE, 6, ABSOLUTE, inc}},
	{NULL},
	{.instr = {"BEQ", 0xF0, 2, RELATIVE, beq}},
	{.instr = {"SBC", 0xF1, 5, INDIRECT_INDEXED, sbc}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"SBC", 0xF5, 5, ZEROPAGE_X, sbc}},
	{.instr = {"INC", 0xF6, 6, ZEROPAGE_X, inc}},
	{NULL},
	{.instr = {"SED", 0xF8, 2, IMPLICIT, sed}},
	{.instr = {"SBC", 0xF9, 4, ABSOLUTE_Y, sbc}},
	{NULL},
	{NULL},
	{NULL},
	{.instr = {"SBC", 0xFD, 4, ABSOLUTE_X, sbc}},
	{.instr = {"INC", 0xFE, 7, ABSOLUTE_X, inc}},
	{NULL}
};

static void debug_instruction(const struct Memory *mem,
			      uint16_t old_pc,
			      uint8_t opcode,
			      uint8_t op1,
			      uint8_t op2,
			      const struct Instruction *instr)
{
	uint8_t displacement;

	printf("%04X %02X ", old_pc, opcode);

	switch (instr->adr_mode) {
	case IMMEDIATE:
		printf("%s #$%02X",
			instr->mnemonic,
		 	read_cpu_byte(mem,
				g_addr_decoded_info.immediate.addr));
		break;
	case ZEROPAGE:
		printf("%02X %s $%02X = %02X",
			op1,
			instr->mnemonic,
			g_addr_decoded_info.zeropage.addr,
		 	read_cpu_byte(mem,
				g_addr_decoded_info.zeropage.addr));
		break;
	case ZEROPAGE_X:
		
		break;
	case ZEROPAGE_Y:
		
		break;
	case RELATIVE:
		displacement = read_cpu_byte(mem,
					g_addr_decoded_info.relative.addr);
		// Probably need adjustements for "negative" displacements.
		printf("%02X %s $%04X",
			op1,
			instr->mnemonic,
			old_pc + 2 + displacement);
		break;
	case ABSOLUTE:
		printf("%02X %02X %s $%04X",
			op1,
			op2,
			instr->mnemonic,
			g_addr_decoded_info.absolute.addr);
		break;
	case ABSOLUTE_X:
		
		break;
	case ABSOLUTE_Y:
		
		break;
	case INDIRECT:
		
		break;
	case INDEXED_INDIRECT:
		
		break;
	case INDIRECT_INDEXED:
		
		break;
	default:
		printf("%s", instr->mnemonic);
	}

	printf("\t");
}

static void print_stack(const struct Cpu *cpu)
{
	uint16_t i;
	uint8_t byte; 

	printf("---STACK---");
	for (i = 0xFF; i != cpu->regs.sp; --i) {
		if ((i + 1) % 16 == 0) {
			printf("\n%02X: ", i);
		}

		byte = read_cpu_byte(cpu->mem, 0x0100 + i);
		printf("%02X ", byte);
	}
	puts("\n-----------");
}

static void print_registers(const struct Registers *regs)
{
	printf("pc: %04X sp: %02X s: %02X "
		"x: %02X y: %02X a: %02x ",
		regs->pc, regs->sp, regs->s,
		regs->x, regs->y, regs->a);
}

static void push_stack(struct Cpu *cpu, uint8_t byte)
{
	write_cpu_byte(cpu->mem, cpu->regs.sp-- + 0x0100, byte);
}

static uint8_t pop_stack(struct Cpu *cpu)
{
	return read_cpu_byte(cpu->mem, ++cpu->regs.sp + 0x0100);
}

static uint8_t check_status(struct Cpu *cpu, enum Status s)
{
	return (cpu->regs.s & (uint8_t)s) != 0;
}

static void set_status(struct Cpu *cpu, enum Status s, uint8_t active)
{
	if (active)
		cpu->regs.s |= (uint8_t)s;
	else
		cpu->regs.s &= ~((uint8_t)s);

	cpu->regs.s |= 0x20;
}

static void set_zn_flags(struct Cpu *cpu, uint8_t value)
{
	set_status(cpu, NEGATIVE, value & 0x80);
	set_status(cpu, ZERO, value == 0);	
}

static void request_interrupt(struct Cpu *cpu, enum Interrupt i)
{
	if (!check_status(cpu, INTERRUPT_DISABLE))
		cpu->interrupt |= (uint8_t)i;
}

static void handle_interrupt(struct Cpu *cpu)
{
	uint16_t handler;
	uint8_t lo, hi;

	if ((cpu->interrupt & RESET) != 0) {
		handler = 0xFFFC;
		cpu->interrupt &= ~RESET;
	} else if ((cpu->interrupt & NMI) != 0) {
		handler = 0xFFFA;
		cpu->interrupt &= ~NMI;
	} else if ((cpu->interrupt & IRQ) != 0) {
		handler = 0xFFFE;
		cpu->interrupt &= ~IRQ;
	}

	push_stack(cpu, cpu->regs.pc >> 8);
	push_stack(cpu, cpu->regs.pc & 0xFF);
	push_stack(cpu, cpu->regs.s);

	lo = read_cpu_byte(cpu->mem, handler);
	hi = read_cpu_byte(cpu->mem, handler + 1);
	cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;

	cpu->current_cycle = 0;
	cpu->cycles = 7;
}

static uint16_t absolute(struct Cpu *cpu)
{
	uint8_t lo;
	uint8_t hi;
	uint16_t addr;

	lo = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	hi = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	addr = ((uint16_t)hi << 8) | (uint16_t)lo;

	g_addr_decoded_info.absolute.addr = addr;
	return addr;
}

static uint16_t absolute_indexed(struct Cpu *cpu, uint8_t index)
{
	uint8_t lo, hi;
	uint16_t absolute, addr;

	lo = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	hi = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	absolute = ((uint16_t)hi << 8) | (uint16_t)lo;
	addr = absolute + (uint16_t)index;
	
	if ((uint16_t)lo > (addr & 0xFF))
		printf("\npage crossed\n");

	g_addr_decoded_info.absolute_indexed.absolute_addr = absolute;
	g_addr_decoded_info.absolute_indexed.index = index;
	g_addr_decoded_info.absolute_indexed.addr = addr;	
	return addr;
}

static uint16_t immediate(struct Cpu *cpu)
{
	g_addr_decoded_info.immediate.addr = cpu->regs.pc;
	return cpu->regs.pc++;
}

static uint16_t zeropage(struct Cpu *cpu)
{
	uint8_t addr;

	addr = read_cpu_byte(cpu->mem, cpu->regs.pc++);

	g_addr_decoded_info.zeropage.addr = addr;
	return (uint16_t)addr;
}

static uint16_t zeropage_indexed(struct Cpu *cpu, uint8_t index)
{
	uint8_t byte = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	uint16_t addr = (byte + index) % 0xFF;
	
	g_addr_decoded_info.zeropage_indexed.zp_addr = byte;
	g_addr_decoded_info.zeropage_indexed.addr = addr;
	g_addr_decoded_info.zeropage_indexed.index = index;
	return addr;
}

static uint16_t relative(struct Cpu *cpu)
{
	g_addr_decoded_info.relative.addr = cpu->regs.pc;
	return cpu->regs.pc++;
}

static uint16_t indirect(struct Cpu *cpu)
{
	uint8_t lo, hi;
	uint16_t addr, iaddr;

	lo = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	hi = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	addr = ((uint16_t)hi << 8) | (uint16_t)lo;

	lo = read_cpu_byte(cpu->mem, addr);
	hi = read_cpu_byte(cpu->mem, addr + 1);
	iaddr = ((uint16_t)hi << 8) | (uint16_t)lo;

	g_addr_decoded_info.indirect.addr = addr;
	g_addr_decoded_info.indirect.iaddr = iaddr;
	return iaddr;
}

static uint16_t indexed_indirect(struct Cpu *cpu)
{
	uint8_t zp_addr, lo, hi;
	uint16_t addr;

	zp_addr = read_cpu_byte(cpu->mem, cpu->regs.pc++) + cpu->regs.x;

	lo = read_cpu_byte(cpu->mem, zp_addr);
	hi = read_cpu_byte(cpu->mem, zp_addr + 1);
	addr = ((uint16_t)hi << 8) | (uint16_t)lo;

	g_addr_decoded_info.indexed_indirect.x = cpu->regs.x;
	g_addr_decoded_info.indexed_indirect.zp_addr = zp_addr;
	g_addr_decoded_info.indexed_indirect.addr = addr;
	
	return addr;
}

// TODO: page cross 1+ cycle
static uint16_t indirect_indexed(struct Cpu *cpu)
{
	uint8_t zp_addr, lo, hi;
	uint16_t addr;

	zp_addr = read_cpu_byte(cpu->mem, cpu->regs.pc++);

	lo = read_cpu_byte(cpu->mem, zp_addr);
	hi = read_cpu_byte(cpu->mem, zp_addr + 1);
	addr = (((uint16_t)hi << 8) | (uint16_t)lo) + (uint16_t)cpu->regs.y;
	
	if ((uint16_t)lo > (addr & 0xFF))
		printf("\npage crossed\n0");

	g_addr_decoded_info.indirect_indexed.y = cpu->regs.y;
	g_addr_decoded_info.indirect_indexed.zp_addr = zp_addr;
	g_addr_decoded_info.indirect_indexed.addr = addr;
	return addr;
}

/* TODO: Unnoficial opcodes need to be implemented... for now let's try to work with this and try to get the ppu a start as well.
 * Work on a better way to print instructions on the screen. 
 */
uint8_t cpu_emulate(struct Cpu *cpu)
{
	uint8_t opcode, op1, op2;
	uint16_t addr, old_pc;
	const union InstructionOrNothing *possible_instr;
	const struct Instruction *instr;

	/* Is the current instruction in execution? */
	if (cpu->current_cycle < cpu->cycles) {
		++cpu->current_cycle;
		return 0;
	}

	if (cpu->interrupt) {
		handle_interrupt(cpu);
		return 0;
	}

	cpu->cycles = 0;
	cpu->current_cycle = 0;
	old_pc = cpu->regs.pc;

	opcode = read_cpu_byte(cpu->mem, cpu->regs.pc);
	op1 = read_cpu_byte(cpu->mem, cpu->regs.pc + 1);
	op2 = read_cpu_byte(cpu->mem, cpu->regs.pc + 2);

	possible_instr = &g_instruction_table[opcode];

	if (!possible_instr->valid) {
		fprintf(stderr, "Invalid opcode %02x found\n", opcode);
		return 0;
	}
	instr = &possible_instr->instr;

	assert(instr->opcode == opcode);

	++cpu->regs.pc;

	switch (instr->adr_mode) {
	case IMMEDIATE:
		addr = immediate(cpu);
		break;
	case ZEROPAGE:
		addr = zeropage(cpu);
		break;
	case ZEROPAGE_X:
		addr = zeropage_indexed(cpu, cpu->regs.x);
		break;
	case ZEROPAGE_Y:
		addr = zeropage_indexed(cpu, cpu->regs.y);
		break;
	case RELATIVE:
		addr = relative(cpu);
		break;
	case ABSOLUTE:
		addr = absolute(cpu);
		break;
	case ABSOLUTE_X:
		addr = absolute_indexed(cpu, cpu->regs.x);
		break;
	case ABSOLUTE_Y:
		addr = absolute_indexed(cpu, cpu->regs.y);
		break;
	case INDIRECT:
		addr = indirect(cpu);
		break;
	case INDEXED_INDIRECT:
		addr = indexed_indirect(cpu);
		break;
	case INDIRECT_INDEXED:
		addr = indirect_indexed(cpu);
		break;
	default:
		addr = 0;
	}

	debug_instruction(cpu->mem, old_pc, opcode, op1, op2, instr);
	
	cpu->cycles = instr->cycles;
	if (instr->execute)
		instr->execute(cpu, addr);

	//print_registers(&cpu->regs);

	/* Finished an instruction. */
	cpu->total_cycles += cpu->cycles;
	return 1;
}

void cpu_init(struct Cpu *cpu, struct Memory *mem)
{
	struct Registers regs;

	regs.pc = 0xC000;
	regs.sp = 0xFF;
	regs.a  = 0x00;
	regs.x  = 0x00;
	regs.y  = 0x00;
	regs.s  = 0x20;

	cpu->regs = regs;
	cpu->mem = mem;
	cpu->interrupt = 0/*(uint8_t)RESET*/;
	cpu->cycles = 0;
	cpu->current_cycle = 0;
	cpu->total_cycles = 0;
}

INSTR(brk)
{
	(void)addr;

	uint8_t lo;
	uint8_t hi;

	push_stack(cpu, cpu->regs.pc >> 8);
	push_stack(cpu, cpu->regs.pc & 0xFF);
	push_stack(cpu, cpu->regs.s);

	lo = read_cpu_byte(cpu->mem, 0xFFFE);
	hi = read_cpu_byte(cpu->mem, 0xFFFF);
	cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;
	set_status(cpu, BREAK, 1);

	cpu->cycles += 7;
}

INSTR(ora)
{
	cpu->regs.a |= read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(asl)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);
	
	set_status(cpu, CARRY, byte & 0x80);

	byte <<= 1;

	set_zn_flags(cpu, byte);
	write_cpu_byte(cpu->mem, addr, byte);
}

INSTR(asl_a)
{
	(void)addr;

	set_status(cpu, CARRY, cpu->regs.a & 0x80);
	cpu->regs.a <<= 1;
	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(php)
{
	(void)addr;
	push_stack(cpu, cpu->regs.s);
}

INSTR(bpl)
{
	branch(cpu, addr, NEGATIVE, 0);
}

INSTR(clc)
{
	(void)addr;
	set_status(cpu, CARRY, 0);
}

INSTR(jsr)
{
	uint16_t tmp = cpu->regs.pc - 1;
		
	push_stack(cpu, tmp >> 8);
	push_stack(cpu, tmp & 0xFF);
	cpu->regs.pc = addr;
}

INSTR(and)
{
	cpu->regs.a &= read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(bit)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);

	set_status(cpu, ZERO, (byte & cpu->regs.a) == 0);
	set_status(cpu, OVERFLOW, byte & 0x40);
	set_status(cpu, NEGATIVE, byte & 0x80);
}

INSTR(rol)
{
	uint8_t byte, carry;

	byte = read_cpu_byte(cpu->mem, addr);
	carry = check_status(cpu, CARRY);

	set_status(cpu, CARRY, byte & 0x80);
	byte = (byte << 1) | carry;

	set_zn_flags(cpu, byte);
	write_cpu_byte(cpu->mem, addr, byte);
}

INSTR(rol_a)
{
	(void)addr;

	uint8_t carry = check_status(cpu, CARRY);

	set_status(cpu, CARRY, cpu->regs.a & 0x80);
	cpu->regs.a = (cpu->regs.a << 1) | carry;

	set_zn_flags(cpu, cpu->regs.a);
	cpu->cycles += 2;
}

INSTR(plp)
{
	(void)addr;
	cpu->regs.s = pop_stack(cpu);
}

INSTR(bmi)
{
	branch(cpu, addr, NEGATIVE, 1);
}

INSTR(sec)
{
	(void)addr;
	set_status(cpu, CARRY, 1);
}

INSTR(rti)
{
	(void)addr;
	uint8_t lo, hi;

	cpu->regs.s = pop_stack(cpu);
	lo = pop_stack(cpu);
	hi = pop_stack(cpu);
	cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;
}

INSTR(eor)
{
	cpu->regs.a ^= read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(lsr)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);

	set_status(cpu, CARRY, byte & 0x01);

	byte >>= 1;

	set_zn_flags(cpu, byte);
	write_cpu_byte(cpu->mem, addr, byte);
}

INSTR(lsr_a)
{
	(void)addr;

	set_status(cpu, CARRY, cpu->regs.a & 0x01);

	cpu->regs.a >>= 1;

	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(pha)
{
	(void)addr;

	push_stack(cpu, cpu->regs.a);
}

INSTR(jmp)
{
	// TODO: Maybe wrong?
	cpu->regs.pc = addr;
}

INSTR(bvc)
{
	branch(cpu, addr, OVERFLOW, 0);
}

INSTR(cli)
{
	(void)addr;
	set_status(cpu, INTERRUPT_DISABLE, 0);
}

INSTR(rts)
{
	(void)addr;

	uint8_t lo = pop_stack(cpu);
	uint8_t hi = pop_stack(cpu);
	cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;
	// TODO: Since we save pc - 1, we return to the wrong position. Maybe save pc? or just keep adding 1...
	cpu->regs.pc += 1;
}

INSTR(adc)
{
	adc_impl(cpu, addr, 0);
}

INSTR(ror)
{
	uint8_t byte, carry;

	byte = read_cpu_byte(cpu->mem, addr);
	carry = check_status(cpu, CARRY);

	set_status(cpu, CARRY, byte & 0x01);
	byte = (byte >> 1) | (carry << 7);

	set_zn_flags(cpu, byte);
	write_cpu_byte(cpu->mem, addr, byte);
}

INSTR(ror_a)
{
	(void)addr;

	uint8_t carry;
	
	carry = check_status(cpu, CARRY);

	set_status(cpu, CARRY, cpu->regs.a & 0x01);
	cpu->regs.a = (cpu->regs.a >> 1) | (carry << 7);

	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(pla)
{
	(void)addr;

	cpu->regs.a = pop_stack(cpu);
	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(bvs)
{
	branch(cpu, addr, OVERFLOW, 1);
}

INSTR(sei)
{
	(void)addr;
	set_status(cpu, INTERRUPT_DISABLE, 1);
}

INSTR(sta)
{
	write_cpu_byte(cpu->mem, addr, cpu->regs.a);
}

INSTR(sty)
{
	write_cpu_byte(cpu->mem, addr, cpu->regs.y);
}

INSTR(stx)
{
	write_cpu_byte(cpu->mem, addr, cpu->regs.x);
}

INSTR(dey)
{
	(void)addr;

	--cpu->regs.y;
	set_zn_flags(cpu, cpu->regs.y);
}

INSTR(txa)
{
	(void)addr;

	cpu->regs.a = cpu->regs.x;
	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(bcc)
{
	branch(cpu, addr, CARRY, 0);	
}

INSTR(tya)
{
	(void)addr;

	cpu->regs.a = cpu->regs.y;
	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(txs)
{
	(void)addr;

	cpu->regs.sp = cpu->regs.x;
}

INSTR(ldy)
{
	cpu->regs.y = read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, cpu->regs.y);
}

INSTR(lda)
{
	cpu->regs.a = read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, cpu->regs.a);
}

INSTR(ldx)
{
	cpu->regs.x = read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, cpu->regs.x);
}

INSTR(tay)
{
	(void)addr;

	cpu->regs.y = cpu->regs.a;
	set_zn_flags(cpu, cpu->regs.y);
}

INSTR(tax)
{
	(void)addr;

	cpu->regs.x = cpu->regs.a;
	set_zn_flags(cpu, cpu->regs.x);
}

INSTR(bcs)
{
	branch(cpu, addr, CARRY, 1);
}

INSTR(clv)
{
	(void)addr;

	set_status(cpu, OVERFLOW, 0);
}

INSTR(tsx)
{
	(void)addr;

	cpu->regs.x = cpu->regs.sp;
	set_zn_flags(cpu, cpu->regs.x);
}

INSTR(cpy)
{
	cmp_impl(cpu, addr, cpu->regs.y);
}

INSTR(cmp)
{
	cmp_impl(cpu, addr, cpu->regs.a);
}

INSTR(dec)
{
	inc_dec_impl(cpu, addr, 0xFF);
}

INSTR(iny)
{
	(void)addr;

	++cpu->regs.y;
	set_zn_flags(cpu, cpu->regs.y);
}

INSTR(dex)
{
	(void)addr;

	--cpu->regs.x;
	set_zn_flags(cpu, cpu->regs.x);
}

INSTR(bne)
{
	branch(cpu, addr, ZERO, 0);
}

INSTR(cld)
{
	(void)addr;

	set_status(cpu, DECIMAL, 0);
}

INSTR(cpx)
{
	cmp_impl(cpu, addr, cpu->regs.x);
}

INSTR(sbc)
{
	adc_impl(cpu, addr, 1);
}

INSTR(inc)
{
	inc_dec_impl(cpu, addr, 0x01);
}

INSTR(inx)
{
	(void)addr;

	++cpu->regs.x;
	set_zn_flags(cpu, cpu->regs.x);
}

INSTR(beq)
{
	branch(cpu, addr, ZERO, 1);
}

INSTR(sed)
{
	(void)addr;
	
	set_status(cpu, DECIMAL, 1);
}

static void inc_dec_impl(struct Cpu *cpu, uint16_t addr, uint8_t change)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr) + change;
	
	write_cpu_byte(cpu->mem, addr, byte);

	set_zn_flags(cpu, byte);
}

static void cmp_impl(struct Cpu *cpu, uint16_t addr, uint8_t reg)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);

	set_status(cpu, CARRY, reg >= byte);
	set_status(cpu, ZERO, reg == byte);
	set_status(cpu, NEGATIVE, ((reg - byte) & 0x80) != 0);
}

static void adc_impl(struct Cpu *cpu, uint16_t addr, uint8_t ones_complement)
{
	uint8_t byte, a, overflow, carry;

	byte = read_cpu_byte(cpu->mem, addr);

	if (ones_complement)
		byte = ~byte;

	a = cpu->regs.a + byte + check_status(cpu, CARRY);
	overflow = (byte & 0x80) == (cpu->regs.a & 0x80) && (byte & 0x80) != (a & 0x80);
	carry = (((uint16_t)cpu->regs.a + (uint16_t)byte) & 0xFF00) != 0;

	set_status(cpu, OVERFLOW, overflow);
	set_zn_flags(cpu, a);
	set_status(cpu, CARRY, carry);

	cpu->regs.a = a;
}

static void branch(struct Cpu *cpu, uint16_t addr, enum Status status,
		   uint8_t value_needed)
{
	uint8_t displacement, old_lo;

	displacement = read_cpu_byte(cpu->mem, addr);

	if (check_status(cpu, status) == value_needed) {
		old_lo = (uint8_t)(cpu->regs.pc & 0xFF);
		/* TODO: Maybe fix? displacement seems to work for now. */
		if (displacement & 0x80)
			cpu->regs.pc += (int8_t)displacement;
		else
			cpu->regs.pc += displacement;

		if (old_lo != (uint8_t)(cpu->regs.pc & 0xFF))
			cpu->cycles += 2;
		else
			cpu->cycles += 1;
	}
}
