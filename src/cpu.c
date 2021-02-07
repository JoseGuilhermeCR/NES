#include "cpu.h"
#include "memory.h"

#include <stdio.h>
#include <stdlib.h>

static void print_stack(const struct cpu *cpu)
{
	printf("---STACK---");
	for (uint16_t i = 0xFF; i != cpu->regs.sp; --i) {
		if ((i + 1) % 16 == 0) {
			printf("\n%02X: ", i);
		}

		uint8_t byte = read_cpu_byte(cpu->mem, 0x0100 + i);
		printf("%02X ", byte);
	}
	puts("\n-----------");
}

static void print_registers(const struct registers *regs)
{
	printf("pc: %04X sp: %02X s: %02X "
		"x: %02X y: %02X a: %02x\n",
		regs->pc, regs->sp, regs->s,
		regs->x, regs->y, regs->a);
}

static void push_stack(struct cpu *cpu, uint8_t byte)
{
	write_cpu_byte(cpu->mem, cpu->regs.sp-- + 0x0100, byte);
}

static uint8_t pop_stack(struct cpu *cpu)
{
	return read_cpu_byte(cpu->mem, ++cpu->regs.sp + 0x0100);
}

static uint8_t check_status(struct cpu *cpu, enum Status s)
{
	return (cpu->regs.s & (uint8_t)s) != 0;
}

static void set_status(struct cpu *cpu, enum Status s, uint8_t active)
{
	if (active)
		cpu->regs.s |= (uint8_t)s;
	else
		cpu->regs.s &= ~((uint8_t)s);

	cpu->regs.s |= 0x20;
}

static void set_zn_flags(struct cpu *cpu, uint8_t value)
{
	set_status(cpu, NEGATIVE, value & 0x80);
	set_status(cpu, ZERO, value == 0);	
}

static void request_interrupt(struct cpu *cpu, enum Interrupt i)
{
	if (!check_status(cpu, INTERRUPT_DISABLE))
		cpu->interrupt |= (uint8_t)i;
}

static void handle_interrupt(struct cpu *cpu)
{
	uint16_t handler;

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

	uint8_t lo = read_cpu_byte(cpu->mem, handler);
	uint8_t hi = read_cpu_byte(cpu->mem, handler + 1);
	cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;

	cpu->current_cycle = 0;
	cpu->cycles = 7;
}

static uint16_t absolute(struct cpu *cpu)
{
	uint8_t lo = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	uint8_t hi = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	uint16_t addr = ((uint16_t)hi << 8) | (uint16_t)lo;
	printf("absolute -> addr: %04X ", addr);
	return addr;
}

static uint16_t absolute_indexed(struct cpu *cpu, uint8_t index)
{
	uint8_t lo = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	uint8_t hi = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	uint16_t absolute = ((uint16_t)hi << 8) | (uint16_t)lo;
	uint16_t addr = absolute + (uint16_t)index;

	printf("absolute_indexed -> absolute: %04X index: %02X addr: %04X ", absolute, index, addr);
	if ((uint16_t)lo > (addr & 0xFF))
		printf("page crossed ");

	return addr;
}

static uint16_t immediate(struct cpu *cpu)
{
	printf("immediate -> addr: %02X ", cpu->regs.pc);
	return cpu->regs.pc++;
}

static uint16_t zeropage(struct cpu *cpu)
{
	uint8_t addr = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	printf("zeropage -> addr: %02X ", addr);
	return (uint16_t)addr;
}

static uint16_t zeropage_indexed(struct cpu *cpu, uint8_t index)
{
	uint8_t byte = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	uint16_t addr = (byte + index) % 0xFF;
	printf("zeropage_indexed -> byte: %02X index: %02X addr: %04X ", byte, index, addr);
	return addr;
}

static uint8_t relative(struct cpu *cpu)
{
	uint8_t byte = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	printf("relative -> byte: %02X ", byte);
	return byte;
}

static uint16_t indirect(struct cpu *cpu)
{
	uint8_t lo = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	uint8_t hi = read_cpu_byte(cpu->mem, cpu->regs.pc++);
	uint16_t addr = ((uint16_t)hi << 8) | (uint16_t)lo;

	lo = read_cpu_byte(cpu->mem, addr);
	hi = read_cpu_byte(cpu->mem, addr + 1);
	uint16_t iaddr = ((uint16_t)hi << 8) | (uint16_t)lo;

	printf("indirect -> addr: %04X indirect: %04X ", addr, iaddr);
	return iaddr;
}

static uint16_t indexed_indirect(struct cpu *cpu)
{
	uint8_t zp_addr = read_cpu_byte(cpu->mem, cpu->regs.pc++) + cpu->regs.x;

	uint8_t lo = read_cpu_byte(cpu->mem, zp_addr);
	uint8_t hi = read_cpu_byte(cpu->mem, zp_addr + 1);
	uint16_t addr = ((uint16_t)hi << 8) | (uint16_t)lo;

	printf("indexed_indirect -> zeropage_addr: %02x addr: %04x ", zp_addr, addr);
	return addr;
}

// TODO: page cross 1+ cycle
static uint16_t indirect_indexed(struct cpu *cpu)
{
	uint8_t zp_addr = read_cpu_byte(cpu->mem, cpu->regs.pc++);

	uint8_t lo = read_cpu_byte(cpu->mem, zp_addr);
	uint8_t hi = read_cpu_byte(cpu->mem, zp_addr + 1);
	uint16_t addr = (((uint16_t)hi << 8) | (uint16_t)lo) + (uint16_t)cpu->regs.y;

	printf("indirect_indexed -> zeropage_addr: %02x addr: %04x ", zp_addr, addr);
	if ((uint16_t)lo > (addr & 0xFF))
		printf("page crossed ");
	return addr;
}

static void load(struct cpu *cpu, uint16_t addr, uint8_t *reg, uint8_t extra_cycles)
{
	*reg = read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, *reg);
	cpu->cycles += 2 + extra_cycles;
}

static void branch(struct cpu *cpu, uint8_t displacement, enum Status status, uint8_t value_needed)
{
	cpu->cycles += 2;
	if (check_status(cpu, status) == value_needed) {
		printf("\nUnsigned: %02X Signed: %i\n", displacement, (int8_t)displacement);

		uint8_t old_lo = (uint8_t)(cpu->regs.pc & 0xFF);
		// TODO: Maybe fix? displacement seems to work for now.
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

static void store(struct cpu *cpu, uint16_t addr, uint8_t reg, uint8_t extra_cycles)
{
	write_cpu_byte(cpu->mem, addr, reg);
	cpu->cycles += 3 + extra_cycles;
}

static void adc(struct cpu *cpu, uint16_t addr, uint8_t extra_cycles, uint8_t ones_complement)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);

	if (ones_complement)
		byte = ~byte;

	uint8_t a = cpu->regs.a + byte + check_status(cpu, CARRY);
	uint8_t overflow = (byte & 0x80) == (cpu->regs.a & 0x80) && (byte & 0x80) != (a & 0x80);
	uint8_t carry = (((uint16_t)cpu->regs.a + (uint16_t)byte) & 0xFF00) != 0;

	set_status(cpu, OVERFLOW, overflow);
	set_zn_flags(cpu, a);
	set_status(cpu, CARRY, carry);

	cpu->regs.a = a;
	cpu->cycles += 2 + extra_cycles;
}

static void and(struct cpu *cpu, uint16_t addr, uint8_t extra_cycles)
{
	cpu->regs.a &= read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, cpu->regs.a);
	cpu->cycles += 2 + extra_cycles;
}

static void ora(struct cpu *cpu, uint16_t addr, uint8_t extra_cycles)
{
	cpu->regs.a |= read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, cpu->regs.a);
	cpu->cycles += 2 + extra_cycles;
}

static void eor(struct cpu *cpu, uint16_t addr, uint8_t extra_cycles)
{
	cpu->regs.a ^= read_cpu_byte(cpu->mem, addr);
	set_zn_flags(cpu, cpu->regs.a);
	cpu->cycles += 2 + extra_cycles;
}

static void inc_dec(struct cpu *cpu, uint16_t addr, uint8_t change, uint8_t extra_cycles)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr) + change;
	write_cpu_byte(cpu->mem, addr, byte);

	set_zn_flags(cpu, byte);
	cpu->cycles += 5 + extra_cycles;
}

static void bit(struct cpu *cpu, uint16_t addr, uint8_t extra_cycles)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);

	set_status(cpu, ZERO, (byte & cpu->regs.a) == 0);
	set_status(cpu, OVERFLOW, byte & 0x40);
	set_status(cpu, NEGATIVE, byte & 0x80);

	cpu->cycles += 3 + extra_cycles;
}

static void cmp(struct cpu *cpu, uint16_t addr, uint8_t reg, uint8_t extra_cycles)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);

	set_status(cpu, CARRY, reg >= byte);
	set_status(cpu, ZERO, reg == byte);
	set_status(cpu, NEGATIVE, ((reg - byte) & 0x80) != 0);

	cpu->cycles += 2 + extra_cycles;
}

static void lsr(struct cpu *cpu, uint16_t addr, uint8_t extra_cycles)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);
	set_status(cpu, CARRY, byte & 0x01);

	byte >>= 1;

	set_zn_flags(cpu, byte);
	write_cpu_byte(cpu->mem, addr, byte);
	cpu->cycles += 5 + extra_cycles;
}

static void lsr_a(struct cpu *cpu)
{
	set_status(cpu, CARRY, cpu->regs.a & 0x01);

	cpu->regs.a >>= 1;

	set_zn_flags(cpu, cpu->regs.a);
	cpu->cycles += 2;
}

static void asl(struct cpu *cpu, uint16_t addr, uint8_t extra_cycles)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);
	set_status(cpu, CARRY, byte & 0x80);

	byte <<= 1;

	set_zn_flags(cpu, byte);
	write_cpu_byte(cpu->mem, addr, byte);
	cpu->cycles += 5 + extra_cycles;
}

static void asl_a(struct cpu *cpu)
{
	set_status(cpu, CARRY, cpu->regs.a & 0x80);

	cpu->regs.a <<= 1;

	set_zn_flags(cpu, cpu->regs.a);
	cpu->cycles += 2;
}

static void ror(struct cpu *cpu, uint16_t addr, uint8_t extra_cycles)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);
	uint8_t carry = check_status(cpu, CARRY);

	set_status(cpu, CARRY, byte & 0x01);
	byte = (byte >> 1) | (carry << 7);

	set_zn_flags(cpu, byte);
	write_cpu_byte(cpu->mem, addr, byte);
	cpu->cycles += 5 + extra_cycles;
}

static void ror_a(struct cpu *cpu)
{
	uint8_t carry = check_status(cpu, CARRY);

	set_status(cpu, CARRY, cpu->regs.a & 0x01);
	cpu->regs.a = (cpu->regs.a >> 1) | (carry << 7);

	set_zn_flags(cpu, cpu->regs.a);
	cpu->cycles += 2;
}

static void rol(struct cpu *cpu, uint16_t addr, uint8_t extra_cycles)
{
	uint8_t byte = read_cpu_byte(cpu->mem, addr);
	uint8_t carry = check_status(cpu, CARRY);

	set_status(cpu, CARRY, byte & 0x80);
	byte = (byte << 1) | carry;

	set_zn_flags(cpu, byte);
	write_cpu_byte(cpu->mem, addr, byte);
	cpu->cycles += 5 + extra_cycles;
}

static void rol_a(struct cpu *cpu)
{
	uint8_t carry = check_status(cpu, CARRY);

	set_status(cpu, CARRY, cpu->regs.a & 0x80);
	cpu->regs.a = (cpu->regs.a << 1) | carry;

	set_zn_flags(cpu, cpu->regs.a);
	cpu->cycles += 2;
}

// TODO: Unnoficial opcodes need to be implemented... for now let's try to work with this and try to get the ppu a start as well.
// Work on a better way to print instructions on the screen.
void emulate_cpu(struct cpu *cpu)
{
//	static uint32_t cyc = 0;

	if (cpu->current_cycle < cpu->cycles) {
		++cpu->current_cycle;
		printf("%i of %i\n", cpu->current_cycle, cpu->cycles);
		return;
	}

	if (cpu->interrupt) {
		handle_interrupt(cpu);
		return;
	}

	cpu->cycles = 0;
	cpu->current_cycle = 0;

	const uint8_t opcode = read_cpu_byte(cpu->mem, cpu->regs.pc);
	// Not a good idea, but leave it here for now.
	const uint8_t op1 = read_cpu_byte(cpu->mem, cpu->regs.pc + 1);
	const uint8_t op2 = read_cpu_byte(cpu->mem, cpu->regs.pc + 2);

	printf("%02X %02X %02X -> ", opcode, op1, op2);

	++cpu->regs.pc;

	switch (opcode) {
		case 0x69:
			adc(cpu, immediate(cpu), 0, 0);
			break;
		case 0x65:
			adc(cpu, zeropage(cpu), 1, 0);
			break;
		case 0x75:
			adc(cpu, zeropage_indexed(cpu, cpu->regs.x), 2, 0);
			break;
		case 0x6D:
			adc(cpu, absolute(cpu), 2, 0);
			break;
		case 0x7D:
			adc(cpu, absolute_indexed(cpu, cpu->regs.x), 2, 0);
			break;
		case 0x79:
			adc(cpu, absolute_indexed(cpu, cpu->regs.y), 2, 0);
			break;
		case 0x61:
			adc(cpu, indexed_indirect(cpu), 4, 0);
			break;
		case 0x71:
			adc(cpu, indirect_indexed(cpu), 3, 0);
			break;
		case 0xE9:
			adc(cpu, immediate(cpu), 0, 1);
			break;
		case 0xE5:
			adc(cpu, zeropage(cpu), 1, 1);
			break;
		case 0xF5:
			adc(cpu, zeropage_indexed(cpu, cpu->regs.x), 2, 1);
			break;
		case 0xED:
			adc(cpu, absolute(cpu), 2, 1);
			break;
		case 0xFD:
			adc(cpu, absolute_indexed(cpu, cpu->regs.x), 2, 1);
			break;
		case 0xF9:
			adc(cpu, absolute_indexed(cpu, cpu->regs.y), 2, 1);
			break;
		case 0xE1:
			adc(cpu, indexed_indirect(cpu), 4, 1);
			break;
		case 0xF1:
			adc(cpu, indirect_indexed(cpu), 3, 1);
			break;
			// JMP
		case 0x4C:
			cpu->regs.pc = absolute(cpu);
			cpu->cycles += 3;
			break;
		case 0x6C:
			cpu->regs.pc = indirect(cpu);
			cpu->cycles += 5;
			break;
			// BRK
		case 0x00: {
				   push_stack(cpu, cpu->regs.pc >> 8);
				   push_stack(cpu, cpu->regs.pc & 0xFF);
				   push_stack(cpu, cpu->regs.s);

				   uint8_t lo = read_cpu_byte(cpu->mem, 0xFFFE);
				   uint8_t hi = read_cpu_byte(cpu->mem, 0xFFFF);
				   cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;
				   set_status(cpu, BREAK, 1);
				   cpu->cycles += 7;
			   }
			break;
			// RTI
		case 0x40: {	
				   cpu->regs.s = pop_stack(cpu);
				   uint8_t lo = pop_stack(cpu);
				   uint8_t hi = pop_stack(cpu);
				   cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;
				   cpu->cycles += 6;
			   }
			break;
		case 0xA2:
			load(cpu, immediate(cpu), &cpu->regs.x, 0);
			break;
		case 0xA6:
			load(cpu, zeropage(cpu), &cpu->regs.x, 1);
			break;
		case 0xB6:
			load(cpu, zeropage_indexed(cpu, cpu->regs.y), &cpu->regs.x, 2);
			break;
		case 0xAE:
			load(cpu, absolute(cpu), &cpu->regs.x, 2);
			break;
		case 0xBE:
			load(cpu, absolute_indexed(cpu, cpu->regs.y), &cpu->regs.x, 2);
			break;
		case 0xA0:
			load(cpu, immediate(cpu), &cpu->regs.y, 0);
			break;
		case 0xA4:
			load(cpu, zeropage(cpu), &cpu->regs.y, 1);
			break;
		case 0xB4:
			load(cpu, zeropage_indexed(cpu, cpu->regs.x), &cpu->regs.y, 2);
			break;
		case 0xAC:
			load(cpu, absolute(cpu), &cpu->regs.y, 2);
			break;
		case 0xBC:
			load(cpu, absolute_indexed(cpu, cpu->regs.x), &cpu->regs.y, 2);
			break;
		case 0xA9:
			load(cpu, immediate(cpu), &cpu->regs.a, 0);
			break;
		case 0xA5:
			load(cpu, zeropage(cpu), &cpu->regs.a, 1);
			break;
		case 0xB5:
			load(cpu, zeropage_indexed(cpu, cpu->regs.x), &cpu->regs.a, 2);
			break;
		case 0xAD:
			load(cpu, absolute(cpu), &cpu->regs.a, 2);
			break;
		case 0xBD:
			load(cpu, absolute_indexed(cpu, cpu->regs.x), &cpu->regs.a, 2);
			break;
		case 0xB9:
			load(cpu, absolute_indexed(cpu, cpu->regs.y), &cpu->regs.a, 2);
			break;
		case 0xA1:
			load(cpu, indexed_indirect(cpu), &cpu->regs.a, 4);
			break;
		case 0xB1:
			load(cpu, indirect_indexed(cpu), &cpu->regs.a, 3);
			break;
		case 0x86:
			store(cpu, zeropage(cpu), cpu->regs.x, 0);
			break;
		case 0x96:
			store(cpu, zeropage_indexed(cpu, cpu->regs.y), cpu->regs.x, 1);
			break;
		case 0x8E:
			store(cpu, absolute(cpu), cpu->regs.x, 1);
			break;
		case 0x84:
			store(cpu, zeropage(cpu), cpu->regs.y, 0);
			break;
		case 0x94:
			store(cpu, zeropage_indexed(cpu, cpu->regs.x), cpu->regs.y, 1);
			break;
		case 0x8C:
			store(cpu, absolute(cpu), cpu->regs.y, 1);
			break;
		case 0x85:
			store(cpu, zeropage(cpu), cpu->regs.a, 0);
			break;
		case 0x95:
			store(cpu, zeropage_indexed(cpu, cpu->regs.x), cpu->regs.a, 1);
			break;
		case 0x8D:
			store(cpu, absolute(cpu), cpu->regs.a, 1);
			break;
		case 0x9D:
			store(cpu, absolute_indexed(cpu, cpu->regs.x), cpu->regs.a, 2);
			break;
		case 0x99:
			store(cpu, absolute_indexed(cpu, cpu->regs.y), cpu->regs.a, 2);
			break;
		case 0x81:
			store(cpu, indexed_indirect(cpu), cpu->regs.a, 3);
			break;
		case 0x91:
			store(cpu, indirect_indexed(cpu), cpu->regs.a, 3);
			break;
			// JSR
		case 0x20: {
				   uint16_t addr = absolute(cpu);
				   uint16_t tmp = cpu->regs.pc - 1;
				   push_stack(cpu, tmp >> 8);
				   push_stack(cpu, tmp & 0xFF);
				   cpu->regs.pc = addr;
				   cpu->cycles += 6;
			   }
			   break;
			   // RTS
		case 0x60: {
				   uint8_t lo = pop_stack(cpu);
				   uint8_t hi = pop_stack(cpu);
				   cpu->regs.pc = ((uint16_t)hi << 8) | (uint16_t)lo;
				   // TODO: Since we save pc - 1, we return to the wrong position. Maybe save pc? or just keep adding 1...
				   cpu->regs.pc += 1;
				   cpu->cycles += 6;
			   }
			   break;
			   // NOP
		case 0xEA:
			   cpu->cycles += 2;
			   break;
		case 0x38:
			   set_status(cpu, CARRY, 1);
			   cpu->cycles += 2;
			   break;
		case 0x18:
			   set_status(cpu, CARRY, 0);
			   cpu->cycles += 2;
			   break;
		case 0xB0:
			   branch(cpu, relative(cpu), CARRY, 1);
			   break;
		case 0x90:
			   branch(cpu, relative(cpu), CARRY, 0);
			   break;
		case 0xF0:
			   branch(cpu, relative(cpu), ZERO, 1);
			   break;
		case 0x30:
			   branch(cpu, relative(cpu), NEGATIVE, 1);
			   break;
		case 0xD0:
			   branch(cpu, relative(cpu), ZERO, 0);
			   break;
		case 0x10:
			   branch(cpu, relative(cpu), NEGATIVE, 0);
			   break;
		case 0x50:
			   branch(cpu, relative(cpu), OVERFLOW, 0);
			   break;
		case 0x70:
			   branch(cpu, relative(cpu), OVERFLOW, 1);
			   break;
		case 0x29:
			   and(cpu, immediate(cpu), 0);
			   break;
		case 0x25:
			   and(cpu, zeropage(cpu), 1);
			   break;
		case 0x35:
			   and(cpu, zeropage_indexed(cpu, cpu->regs.x), 2);
			   break;
		case 0x2D:
			   and(cpu, absolute(cpu), 2);
			   break;
		case 0x3D:
			   and(cpu, absolute_indexed(cpu, cpu->regs.x), 2);
			   break;
		case 0x39:
			   and(cpu, absolute_indexed(cpu, cpu->regs.y), 2);
			   break;
		case 0x21:
			   and(cpu, indexed_indirect(cpu), 4);
			   break;
		case 0x31:
			   and(cpu, indirect_indexed(cpu), 3);
			   break;
		case 0x09:
			   ora(cpu, immediate(cpu), 0);
			   break;
		case 0x05:
			   ora(cpu, zeropage(cpu), 1);
			   break;
		case 0x15:
			   ora(cpu, zeropage_indexed(cpu, cpu->regs.x), 2);
			   break;
		case 0x0D:
			   ora(cpu, absolute(cpu), 2);
			   break;
		case 0x1D:
			   ora(cpu, absolute_indexed(cpu, cpu->regs.x), 2);
			   break;
		case 0x19:
			   ora(cpu, absolute_indexed(cpu, cpu->regs.y), 2);
			   break;
		case 0x01:
			   ora(cpu, indexed_indirect(cpu), 4);
			   break;
		case 0x11:
			   ora(cpu, indirect_indexed(cpu), 3);
			   break;
		case 0x49:
			   eor(cpu, immediate(cpu), 0);
			   break;
		case 0x45:
			   eor(cpu, zeropage(cpu), 1);
			   break;
		case 0x55:
			   eor(cpu, zeropage_indexed(cpu, cpu->regs.x), 2);
			   break;
		case 0x4D:
			   eor(cpu, absolute(cpu), 2);
			   break;
		case 0x5D:
			   eor(cpu, absolute_indexed(cpu, cpu->regs.x), 2);
			   break;
		case 0x59:
			   eor(cpu, absolute_indexed(cpu, cpu->regs.y), 2);
			   break;
		case 0x41:
			   eor(cpu, indexed_indirect(cpu), 4);
			   break;
		case 0x51:
			   eor(cpu, indirect_indexed(cpu), 3);
			   break;
		case 0xE6:
			   inc_dec(cpu, zeropage(cpu), 0x1, 0);
			   break;
		case 0xF6:
			   inc_dec(cpu, zeropage_indexed(cpu, cpu->regs.x), 0x1, 1);
			   break;
		case 0xEE:
			   inc_dec(cpu, absolute(cpu), 0x1, 1);
			   break;
		case 0xFE:
			   inc_dec(cpu, absolute_indexed(cpu, cpu->regs.x), 0x1, 2);
			   break;
		case 0xC6:
			   inc_dec(cpu, zeropage(cpu), 0xFF, 0);
			   break;
		case 0xD6:
			   inc_dec(cpu, zeropage_indexed(cpu, cpu->regs.x), 0xFF, 1);
			   break;
		case 0xCE:
			   inc_dec(cpu, absolute(cpu), 0xFF, 1);
			   break;
		case 0xDE:
			   inc_dec(cpu, absolute_indexed(cpu, cpu->regs.x), 0xFF, 2);
			   break;
		case 0xE8:
			   ++cpu->regs.x;
			   set_zn_flags(cpu, cpu->regs.x);
			   cpu->cycles += 2;
			   break;
		case 0xC8:
			   ++cpu->regs.y;
			   set_zn_flags(cpu, cpu->regs.y);
			   cpu->cycles += 2;
			   break;
		case 0x88:
			   --cpu->regs.y;
			   set_zn_flags(cpu, cpu->regs.y);
			   cpu->cycles += 2;
			   break;
		case 0xCA:
			   --cpu->regs.x;
			   set_zn_flags(cpu, cpu->regs.x);
			   cpu->cycles += 2;
			   break;

		case 0x24:
			   bit(cpu, zeropage(cpu), 0);
			   break;
		case 0x2C:
			   bit(cpu, absolute(cpu), 1);
			   break;
		case 0x78:
			   set_status(cpu, INTERRUPT_DISABLE, 1);
			   cpu->cycles += 2;
			   break;
		case 0xF8:
			   set_status(cpu, DECIMAL, 1);
			   cpu->cycles += 2;
			   break;
		case 0xD8:
			   set_status(cpu, DECIMAL, 0);
			   cpu->cycles += 2;
			   break;
		case 0xB8:
			   set_status(cpu, OVERFLOW, 0);
			   cpu->cycles += 2;
			   break;
		case 0x08:
			   push_stack(cpu, cpu->regs.s);
			   cpu->cycles += 3;
			   break;
		case 0x28:
			   cpu->regs.s = pop_stack(cpu);
			   cpu->cycles += 4;
			   break;
		case 0x68:
			   cpu->regs.a = pop_stack(cpu);
			   set_zn_flags(cpu, cpu->regs.a);
			   cpu->cycles += 4;
			   break;
		case 0x48:
			   push_stack(cpu, cpu->regs.a);
			   cpu->cycles += 3;
			   break;
		case 0xC9:
			   cmp(cpu, immediate(cpu), cpu->regs.a, 0);
			   break;
		case 0xC5:
			   cmp(cpu, zeropage(cpu), cpu->regs.a, 1);
			   break;
		case 0xD5:
			   cmp(cpu, zeropage_indexed(cpu, cpu->regs.x), cpu->regs.a, 2);
			   break;
		case 0xCD:
			   cmp(cpu, absolute(cpu), cpu->regs.a, 2);
			   break;
		case 0xDD:
			   cmp(cpu, absolute_indexed(cpu, cpu->regs.x), cpu->regs.a, 2);
			   break;
		case 0xD9:
			   cmp(cpu, absolute_indexed(cpu, cpu->regs.y), cpu->regs.a, 2);
			   break;
		case 0xC1:
			   cmp(cpu, indexed_indirect(cpu), cpu->regs.a, 4);
			   break;
		case 0xD1:
			   cmp(cpu, indirect_indexed(cpu), cpu->regs.a, 3);
			   break;
		case 0xE0:
			   cmp(cpu, immediate(cpu), cpu->regs.x, 0);
			   break;
		case 0xE4:
			   cmp(cpu, zeropage(cpu), cpu->regs.x, 1);
			   break;
		case 0xEC:
			   cmp(cpu, absolute(cpu), cpu->regs.x, 2);
			   break;
		case 0xC0:
			   cmp(cpu, immediate(cpu), cpu->regs.y, 0);
			   break;
		case 0xC4:
			   cmp(cpu, zeropage(cpu), cpu->regs.y, 1);
			   break;
		case 0xCC:
			   cmp(cpu, absolute(cpu), cpu->regs.y, 2);
			   break;
		case 0xA8:
			   cpu->regs.y = cpu->regs.a;
			   set_zn_flags(cpu, cpu->regs.y);
			   cpu->cycles += 2;
			   break;
		case 0xBA:
			   cpu->regs.x = cpu->regs.sp;
			   set_zn_flags(cpu, cpu->regs.x);
			   cpu->cycles += 2;
			   break;
		case 0x8A:
			   cpu->regs.a = cpu->regs.x;
			   set_zn_flags(cpu, cpu->regs.a);
			   cpu->cycles += 2;
			   break;
		case 0x9A:
			   cpu->regs.sp = cpu->regs.x;
			   cpu->cycles += 2;
			   break;
		case 0x98:
			   cpu->regs.a = cpu->regs.y;
			   set_zn_flags(cpu, cpu->regs.a);
			   cpu->cycles += 2;
			   break;
		case 0xAA:
			   cpu->regs.x = cpu->regs.a;
			   set_zn_flags(cpu, cpu->regs.x);
			   cpu->cycles += 2;
			   break;
		case 0x4A:
			   lsr_a(cpu);
			   break;
		case 0x46:
			   lsr(cpu, zeropage(cpu), 0);
			   break;
		case 0x56:
			   lsr(cpu, zeropage_indexed(cpu, cpu->regs.x), 1);
			   break;
		case 0x4E:
			   lsr(cpu, absolute(cpu), 1);
			   break;
		case 0x5E:
			   lsr(cpu, absolute_indexed(cpu, cpu->regs.x), 2);
			   break;
		case 0x0A:
			   asl_a(cpu);
			   break;
		case 0x06:
			   asl(cpu, zeropage(cpu), 0);
			   break;
		case 0x16:
			   asl(cpu, zeropage_indexed(cpu, cpu->regs.x), 1);
			   break;
		case 0x0E:
			   asl(cpu, absolute(cpu), 1);
			   break;
		case 0x1E:
			   asl(cpu, absolute_indexed(cpu, cpu->regs.x), 2);
			   break;
		case 0x6A:
			   ror_a(cpu);
			   break;
		case 0x66:
			   ror(cpu, zeropage(cpu), 0);
			   break;
		case 0x76:
			   ror(cpu, zeropage_indexed(cpu, cpu->regs.x), 1);
			   break;
		case 0x6E:
			   ror(cpu, absolute(cpu), 1);
			   break;
		case 0x7E:
			   ror(cpu, absolute_indexed(cpu, cpu->regs.x), 2);
			   break;
		case 0x2A:
			   rol_a(cpu);
			   break;
		case 0x26:
			   rol(cpu, zeropage(cpu), 0);
			   break;
		case 0x36:
			   rol(cpu, zeropage_indexed(cpu, cpu->regs.x), 1);
			   break;
		case 0x2E:
			   rol(cpu, absolute(cpu), 1);
			   break;
		case 0x3E:
			   rol(cpu, absolute_indexed(cpu, cpu->regs.x), 2);
			   break;
	}

	//cyc += cpu->cycles;
	//cpu->cycles = 0;
	//printf("CYC: %i ", cyc);
	print_registers(&cpu->regs);
}

void init_cpu(struct cpu *cpu, struct memory *mem)
{
	struct registers regs;

	regs.pc = 0x0000;
	regs.sp = 0xFF;
	regs.a  = 0x00;
	regs.x  = 0x00;
	regs.y  = 0x00;
	regs.s  = 0x20;

	cpu->regs = regs;
	cpu->mem = mem;
	cpu->interrupt = (uint8_t)RESET;
	cpu->cycles = 0;
	cpu->current_cycle = 0;
}
