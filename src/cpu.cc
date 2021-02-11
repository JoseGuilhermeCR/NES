#include "cpu.h"
#include "memory.h"

#include <iostream>

namespace Nes {
	void Registers::set_status(Status status, bool active) {
		if (active)
			s |= static_cast<uint8_t>(status);
		else
			s &= ~(static_cast<uint8_t>(status));

		s |= 0x20;
	}

	bool Registers::check_status(Status status) const {
		return (s & static_cast<uint8_t>(status)) != 0;
	}

	void Registers::set_zn_flags(uint8_t value) {
		set_status(Status::NEGATIVE, value & 0x80);
		set_status(Status::ZERO, value == 0);	
	}

	Cpu::Cpu(Memory &memory)
		:
		_mem(memory),
		_interrupt(static_cast<uint8_t>(Interrupt::RESET)),
		_cycles(),
		_current_cycle()
	{
		_regs.pc = 0x0000;
		_regs.sp = 0xFF;
		_regs.a  = 0x00;
		_regs.x  = 0x00;
		_regs.y  = 0x00;
		_regs.s  = 0x20;
	}

	void Cpu::push_stack(uint8_t byte) {
		_mem.write_cpu_byte(_regs.sp-- + 0x0100, byte);	
	}

	uint8_t Cpu::pop_stack() {
		return _mem.read_cpu_byte(++_regs.sp + 0x0100);
	}

	void Cpu::request_interrupt(Interrupt i) {
		if (!_regs.check_status(Status::INTERRUPT_DISABLE))
			_interrupt |= static_cast<uint8_t>(i);
	}

	void Cpu::handle_interrupt() {
		uint16_t handler;

		if ((_interrupt & static_cast<uint8_t>(Interrupt::RESET)) != 0) {
			handler = 0xFFFC;
			_interrupt &= ~static_cast<uint8_t>(Interrupt::RESET);
		} else if ((_interrupt & static_cast<uint8_t>(Interrupt::NMI)) != 0) {
			handler = 0xFFFA;
			_interrupt &= ~static_cast<uint8_t>(Interrupt::NMI);
		} else if ((_interrupt & static_cast<uint8_t>(Interrupt::IRQ)) != 0) {
			handler = 0xFFFE;
			_interrupt &= ~static_cast<uint8_t>(Interrupt::IRQ);
		}

		push_stack(_regs.pc >> 8);
		push_stack(_regs.pc & 0xFF);
		push_stack(_regs.s);

		uint8_t lo = _mem.read_cpu_byte(handler);
		uint8_t hi = _mem.read_cpu_byte(handler + 1);
		_regs.pc = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);

		_current_cycle = 0;
		_cycles = 7;
	}

	uint16_t Cpu::absolute() {
		uint8_t lo = _mem.read_cpu_byte(_regs.pc++);
		uint8_t hi = _mem.read_cpu_byte(_regs.pc++);
		uint16_t addr = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);

		return addr;
	}

	uint16_t Cpu::absolute_indexed(uint8_t index) {
		uint8_t lo = _mem.read_cpu_byte(_regs.pc++);
		uint8_t hi = _mem.read_cpu_byte(_regs.pc++);
		uint16_t absolute = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
		uint16_t addr = absolute + static_cast<uint16_t>(index);

		if (static_cast<uint16_t>(lo) > (addr & 0xFF))
			std::cout << "page cross: absolute indexed\n";

		return addr;
	}

	uint16_t Cpu::immediate() {
		return _regs.pc++;
	}

	uint16_t Cpu::zeropage() {
		uint8_t addr = _mem.read_cpu_byte(_regs.pc++);
		return static_cast<uint16_t>(addr);
	}

	uint16_t Cpu::zeropage_indexed(uint8_t index) {
		uint8_t byte = _mem.read_cpu_byte(_regs.pc++);
		uint16_t addr = (byte + index) % 0xFF;
		return addr;
	}

	uint8_t Cpu::relative() {
		uint8_t byte = _mem.read_cpu_byte(_regs.pc++);
		return byte;
	}

	uint16_t Cpu::indirect() {
		uint8_t lo = _mem.read_cpu_byte(_regs.pc++);
		uint8_t hi = _mem.read_cpu_byte(_regs.pc++);
		uint16_t addr = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);

		lo = _mem.read_cpu_byte(addr);
		hi = _mem.read_cpu_byte(addr + 1);
		uint16_t iaddr = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);

		return iaddr;
	}

	uint16_t Cpu::indexed_indirect() {
		uint8_t zp_addr = _mem.read_cpu_byte(_regs.pc++) + _regs.x;

		uint8_t lo = _mem.read_cpu_byte(zp_addr);
		uint8_t hi = _mem.read_cpu_byte(zp_addr + 1);
		uint16_t addr = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);

		return addr;
	}

	// TODO: page cross 1+ cycle
	uint16_t Cpu::indirect_indexed() {
		uint8_t zp_addr = _mem.read_cpu_byte(_regs.pc++);

		uint8_t lo = _mem.read_cpu_byte(zp_addr);
		uint8_t hi = _mem.read_cpu_byte(zp_addr + 1);
		uint16_t addr = ((static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo)) + static_cast<uint16_t>(_regs.y);

		if (static_cast<uint16_t>(lo) > (addr & 0xFF))
			std::cout << "page cross: indirect indexed\n";
		return addr;
	}

	// TODO: Unnoficial opcodes need to be implemented... for now let's try to work with this and try to get the ppu a start as well.
	// Work on a better way to print instructions on the screen.
	void Cpu::emulate() {
		if (_current_cycle < _cycles) {
			++_current_cycle;
			//std::cout << static_cast<uint16_t>(_current_cycle) << " of " << static_cast<uint16_t>(_cycles) << '\n';
			return;
		}

		if (_interrupt) {
			handle_interrupt();
			return;
		}

		_cycles = 0;
		_current_cycle = 0;

		const uint8_t opcode = _mem.read_cpu_byte(_regs.pc);
		// Not a good idea, but leave it here for now.
		const uint8_t op1 = _mem.read_cpu_byte(_regs.pc + 1);
		const uint8_t op2 = _mem.read_cpu_byte(_regs.pc + 2);

		//std::cout << std::hex;
//		std::cout << static_cast<uint16_t>(opcode) << ' ' << static_cast<uint16_t>(op1) << ' ' << static_cast<uint16_t>(op2) << '\n';
//		std::cout << std::dec;

		++_regs.pc;

		switch (opcode) {
			case 0x69:
				adc(immediate(), 0, 0);
				break;
			case 0x65:
				adc(zeropage(), 1, 0);
				break;
			case 0x75:
				adc(zeropage_indexed(_regs.x), 2, 0);
				break;
			case 0x6D:
				adc(absolute(), 2, 0);
				break;
			case 0x7D:
				adc(absolute_indexed(_regs.x), 2, 0);
				break;
			case 0x79:
				adc(absolute_indexed(_regs.y), 2, 0);
				break;
			case 0x61:
				adc(indexed_indirect(), 4, 0);
				break;
			case 0x71:
				adc(indirect_indexed(), 3, 0);
				break;
			case 0xE9:
				adc(immediate(), 0, 1);
				break;
			case 0xE5:
				adc(zeropage(), 1, 1);
				break;
			case 0xF5:
				adc(zeropage_indexed(_regs.x), 2, 1);
				break;
			case 0xED:
				adc(absolute(), 2, 1);
				break;
			case 0xFD:
				adc(absolute_indexed(_regs.x), 2, 1);
				break;
			case 0xF9:
				adc(absolute_indexed(_regs.y), 2, 1);
				break;
			case 0xE1:
				adc(indexed_indirect(), 4, 1);
				break;
			case 0xF1:
				adc(indirect_indexed(), 3, 1);
				break;
				// JMP
			case 0x4C:
				_regs.pc = absolute();
				_cycles += 3;
				break;
			case 0x6C:
				_regs.pc = indirect();
				_cycles += 5;
				break;
				// BRK
			case 0x00: {
					   push_stack(_regs.pc >> 8);
					   push_stack(_regs.pc & 0xFF);
					   push_stack(_regs.s);

					   uint8_t lo = _mem.read_cpu_byte(0xFFFE);
					   uint8_t hi = _mem.read_cpu_byte(0xFFFF);
					   _regs.pc = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
					   _regs.set_status(Status::BREAK, 1);
					   _cycles += 7;
				   }
				   break;
				   // RTI
			case 0x40: {	
					   _regs.s = pop_stack();
					   uint8_t lo = pop_stack();
					   uint8_t hi = pop_stack();
					   _regs.pc = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
					   _cycles += 6;
				   }
				   break;
			case 0xA2:
				   load(immediate(), _regs.x, 0);
				   break;
			case 0xA6:
				   load(zeropage(), _regs.x, 1);
				   break;
			case 0xB6:
				   load(zeropage_indexed(_regs.y), _regs.x, 2);
				   break;
			case 0xAE:
				   load(absolute(), _regs.x, 2);
				   break;
			case 0xBE:
				   load(absolute_indexed(_regs.y), _regs.x, 2);
				   break;
			case 0xA0:
				   load(immediate(), _regs.y, 0);
				   break;
			case 0xA4:
				   load(zeropage(), _regs.y, 1);
				   break;
			case 0xB4:
				   load(zeropage_indexed(_regs.x), _regs.y, 2);
				   break;
			case 0xAC:
				   load(absolute(), _regs.y, 2);
				   break;
			case 0xBC:
				   load(absolute_indexed(_regs.x), _regs.y, 2);
				   break;
			case 0xA9:
				   load(immediate(), _regs.a, 0);
				   break;
			case 0xA5:
				   load(zeropage(), _regs.a, 1);
				   break;
			case 0xB5:
				   load(zeropage_indexed(_regs.x), _regs.a, 2);
				   break;
			case 0xAD:
				   load(absolute(), _regs.a, 2);
				   break;
			case 0xBD:
				   load(absolute_indexed(_regs.x), _regs.a, 2);
				   break;
			case 0xB9:
				   load(absolute_indexed(_regs.y), _regs.a, 2);
				   break;
			case 0xA1:
				   load(indexed_indirect(), _regs.a, 4);
				   break;
			case 0xB1:
				   load(indirect_indexed(), _regs.a, 3);
				   break;
			case 0x86:
				   store(zeropage(), _regs.x, 0);
				   break;
			case 0x96:
				   store(zeropage_indexed(_regs.y), _regs.x, 1);
				   break;
			case 0x8E:
				   store(absolute(), _regs.x, 1);
				   break;
			case 0x84:
				   store(zeropage(), _regs.y, 0);
				   break;
			case 0x94:
				   store(zeropage_indexed(_regs.x), _regs.y, 1);
				   break;
			case 0x8C:
				   store(absolute(), _regs.y, 1);
				   break;
			case 0x85:
				   store(zeropage(), _regs.a, 0);
				   break;
			case 0x95:
				   store(zeropage_indexed(_regs.x), _regs.a, 1);
				   break;
			case 0x8D:
				   store(absolute(), _regs.a, 1);
				   break;
			case 0x9D:
				   store(absolute_indexed(_regs.x), _regs.a, 2);
				   break;
			case 0x99:
				   store(absolute_indexed(_regs.y), _regs.a, 2);
				   break;
			case 0x81:
				   store(indexed_indirect(), _regs.a, 3);
				   break;
			case 0x91:
				   store(indirect_indexed(), _regs.a, 3);
				   break;
				   // JSR
			case 0x20: {
					   uint16_t addr = absolute();
					   uint16_t tmp = _regs.pc - 1;
					   push_stack(tmp >> 8);
					   push_stack(tmp & 0xFF);
					   _regs.pc = addr;
					   _cycles += 6;
				   }
				   break;
				   // RTS
			case 0x60: {
					   uint8_t lo = pop_stack();
					   uint8_t hi = pop_stack();
					   _regs.pc = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
					   // TODO: Since we save pc - 1, we return to the wrong position. Maybe save pc? or just keep adding 1...
					   _regs.pc += 1;
					   _cycles += 6;
				   }
				   break;
				   // NOP
			case 0xEA:
				   _cycles += 2;
				   break;
			case 0x38:
				   _regs.set_status(Status::CARRY, 1);
				   _cycles += 2;
				   break;
			case 0x18:
				   _regs.set_status(Status::CARRY, 0);
				   _cycles += 2;
				   break;
			case 0xB0:
				   branch(relative(), Status::CARRY, 1);
				   break;
			case 0x90:
				   branch(relative(), Status::CARRY, 0);
				   break;
			case 0xF0:
				   branch(relative(), Status::ZERO, 1);
				   break;
			case 0x30:
				   branch(relative(), Status::NEGATIVE, 1);
				   break;
			case 0xD0:
				   branch(relative(), Status::ZERO, 0);
				   break;
			case 0x10:
				   branch(relative(), Status::NEGATIVE, 0);
				   break;
			case 0x50:
				   branch(relative(), Status::OVERFLOW, 0);
				   break;
			case 0x70:
				   branch(relative(), Status::OVERFLOW, 1);
				   break;
			case 0x29:
				   ana(immediate(), 0);
				   break;
			case 0x25:
				   ana(zeropage(), 1);
				   break;
			case 0x35:
				   ana(zeropage_indexed(_regs.x), 2);
				   break;
			case 0x2D:
				   ana(absolute(), 2);
				   break;
			case 0x3D:
				   ana(absolute_indexed(_regs.x), 2);
				   break;
			case 0x39:
				   ana(absolute_indexed(_regs.y), 2);
				   break;
			case 0x21:
				   ana(indexed_indirect(), 4);
				   break;
			case 0x31:
				   ana(indirect_indexed(), 3);
				   break;
			case 0x09:
				   ora(immediate(), 0);
				   break;
			case 0x05:
				   ora(zeropage(), 1);
				   break;
			case 0x15:
				   ora(zeropage_indexed(_regs.x), 2);
				   break;
			case 0x0D:
				   ora(absolute(), 2);
				   break;
			case 0x1D:
				   ora(absolute_indexed(_regs.x), 2);
				   break;
			case 0x19:
				   ora(absolute_indexed(_regs.y), 2);
				   break;
			case 0x01:
				   ora(indexed_indirect(), 4);
				   break;
			case 0x11:
				   ora(indirect_indexed(), 3);
				   break;
			case 0x49:
				   eor(immediate(), 0);
				   break;
			case 0x45:
				   eor(zeropage(), 1);
				   break;
			case 0x55:
				   eor(zeropage_indexed(_regs.x), 2);
				   break;
			case 0x4D:
				   eor(absolute(), 2);
				   break;
			case 0x5D:
				   eor(absolute_indexed(_regs.x), 2);
				   break;
			case 0x59:
				   eor(absolute_indexed(_regs.y), 2);
				   break;
			case 0x41:
				   eor(indexed_indirect(), 4);
				   break;
			case 0x51:
				   eor(indirect_indexed(), 3);
				   break;
			case 0xE6:
				   inc_dec(zeropage(), 0x1, 0);
				   break;
			case 0xF6:
				   inc_dec(zeropage_indexed(_regs.x), 0x1, 1);
				   break;
			case 0xEE:
				   inc_dec(absolute(), 0x1, 1);
				   break;
			case 0xFE:
				   inc_dec(absolute_indexed(_regs.x), 0x1, 2);
				   break;
			case 0xC6:
				   inc_dec(zeropage(), 0xFF, 0);
				   break;
			case 0xD6:
				   inc_dec(zeropage_indexed(_regs.x), 0xFF, 1);
				   break;
			case 0xCE:
				   inc_dec(absolute(), 0xFF, 1);
				   break;
			case 0xDE:
				   inc_dec(absolute_indexed(_regs.x), 0xFF, 2);
				   break;
			case 0xE8:
				   ++_regs.x;
				   _regs.set_zn_flags(_regs.x);
				   _cycles += 2;
				   break;
			case 0xC8:
				   ++_regs.y;
				   _regs.set_zn_flags(_regs.y);
				   _cycles += 2;
				   break;
			case 0x88:
				   --_regs.y;
				   _regs.set_zn_flags(_regs.y);
				   _cycles += 2;
				   break;
			case 0xCA:
				   --_regs.x;
				   _regs.set_zn_flags(_regs.x);
				   _cycles += 2;
				   break;

			case 0x24:
				   bit(zeropage(), 0);
				   break;
			case 0x2C:
				   bit(absolute(), 1);
				   break;
			case 0x78:
				   _regs.set_status(Status::INTERRUPT_DISABLE, 1);
				   _cycles += 2;
				   break;
			case 0xF8:
				   _regs.set_status(Status::DECIMAL, 1);
				   _cycles += 2;
				   break;
			case 0xD8:
				   _regs.set_status(Status::DECIMAL, 0);
				   _cycles += 2;
				   break;
			case 0xB8:
				   _regs.set_status(Status::OVERFLOW, 0);
				   _cycles += 2;
				   break;
			case 0x08:
				   push_stack(_regs.s);
				   _cycles += 3;
				   break;
			case 0x28:
				   _regs.s = pop_stack();
				   _cycles += 4;
				   break;
			case 0x68:
				   _regs.a = pop_stack();
				   _regs.set_zn_flags(_regs.a);
				   _cycles += 4;
				   break;
			case 0x48:
				   push_stack(_regs.a);
				   _cycles += 3;
				   break;
			case 0xC9:
				   cmp(immediate(), _regs.a, 0);
				   break;
			case 0xC5:
				   cmp(zeropage(), _regs.a, 1);
				   break;
			case 0xD5:
				   cmp(zeropage_indexed(_regs.x), _regs.a, 2);
				   break;
			case 0xCD:
				   cmp(absolute(), _regs.a, 2);
				   break;
			case 0xDD:
				   cmp(absolute_indexed(_regs.x), _regs.a, 2);
				   break;
			case 0xD9:
				   cmp(absolute_indexed(_regs.y), _regs.a, 2);
				   break;
			case 0xC1:
				   cmp(indexed_indirect(), _regs.a, 4);
				   break;
			case 0xD1:
				   cmp(indirect_indexed(), _regs.a, 3);
				   break;
			case 0xE0:
				   cmp(immediate(), _regs.x, 0);
				   break;
			case 0xE4:
				   cmp(zeropage(), _regs.x, 1);
				   break;
			case 0xEC:
				   cmp(absolute(), _regs.x, 2);
				   break;
			case 0xC0:
				   cmp(immediate(), _regs.y, 0);
				   break;
			case 0xC4:
				   cmp(zeropage(), _regs.y, 1);
				   break;
			case 0xCC:
				   cmp(absolute(), _regs.y, 2);
				   break;
			case 0xA8:
				   _regs.y = _regs.a;
				   _regs.set_zn_flags(_regs.y);
				   _cycles += 2;
				   break;
			case 0xBA:
				   _regs.x = _regs.sp;
				   _regs.set_zn_flags(_regs.x);
				   _cycles += 2;
				   break;
			case 0x8A:
				   _regs.a = _regs.x;
				   _regs.set_zn_flags(_regs.a);
				   _cycles += 2;
				   break;
			case 0x9A:
				   _regs.sp = _regs.x;
				   _cycles += 2;
				   break;
			case 0x98:
				   _regs.a = _regs.y;
				   _regs.set_zn_flags(_regs.a);
				   _cycles += 2;
				   break;
			case 0xAA:
				   _regs.x = _regs.a;
				   _regs.set_zn_flags(_regs.x);
				   _cycles += 2;
				   break;
			case 0x4A:
				   lsr_a();
				   break;
			case 0x46:
				   lsr(zeropage(), 0);
				   break;
			case 0x56:
				   lsr(zeropage_indexed(_regs.x), 1);
				   break;
			case 0x4E:
				   lsr(absolute(), 1);
				   break;
			case 0x5E:
				   lsr(absolute_indexed(_regs.x), 2);
				   break;
			case 0x0A:
				   asl_a();
				   break;
			case 0x06:
				   asl(zeropage(), 0);
				   break;
			case 0x16:
				   asl(zeropage_indexed(_regs.x), 1);
				   break;
			case 0x0E:
				   asl(absolute(), 1);
				   break;
			case 0x1E:
				   asl(absolute_indexed(_regs.x), 2);
				   break;
			case 0x6A:
				   ror_a();
				   break;
			case 0x66:
				   ror(zeropage(), 0);
				   break;
			case 0x76:
				   ror(zeropage_indexed(_regs.x), 1);
				   break;
			case 0x6E:
				   ror(absolute(), 1);
				   break;
			case 0x7E:
				   ror(absolute_indexed(_regs.x), 2);
				   break;
			case 0x2A:
				   rol_a();
				   break;
			case 0x26:
				   rol(zeropage(), 0);
				   break;
			case 0x36:
				   rol(zeropage_indexed(_regs.x), 1);
				   break;
			case 0x2E:
				   rol(absolute(), 1);
				   break;
			case 0x3E:
				   rol(absolute_indexed(_regs.x), 2);
				   break;
		}
	}

	void Cpu::load(uint16_t addr, uint8_t &reg, uint8_t extra_cycles)
	{
		reg = _mem.read_cpu_byte(addr);
		_regs.set_zn_flags(reg);
		_cycles += 2 + extra_cycles;
	}

	void Cpu::branch(uint8_t displacement, Status status, bool value_needed)
	{
		_cycles += 2;
		if (_regs.check_status(status) == value_needed) {
			uint8_t old_lo = static_cast<uint8_t>(_regs.pc & 0xFF);
			// TODO: Maybe fix? displacement seems to work for now.
			if (displacement & 0x80)
				_regs.pc += static_cast<int8_t>(displacement);
			else
				_regs.pc += displacement;

			if (old_lo != static_cast<uint8_t>(_regs.pc & 0xFF))
				_cycles += 2;
			else
				_cycles += 1;
		}
	}

	void Cpu::store(uint16_t addr, uint8_t reg, uint8_t extra_cycles)
	{
		_mem.write_cpu_byte(addr, reg);
		_cycles += 3 + extra_cycles;
	}

	void Cpu::adc(uint16_t addr, uint8_t extra_cycles, uint8_t ones_complement)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);

		if (ones_complement)
			byte = ~byte;

		uint8_t a = _regs.a + byte + _regs.check_status(Status::CARRY);
		bool overflow = (byte & 0x80) == (_regs.a & 0x80) && (byte & 0x80) != (a & 0x80);
		bool carry = ((static_cast<uint16_t>(_regs.a) + static_cast<uint16_t>(byte)) & 0xFF00) != 0;

		_regs.set_status(Status::OVERFLOW, overflow);
		_regs.set_zn_flags(a);
		_regs.set_status(Status::CARRY, carry);

		_regs.a = a;
		_cycles += 2 + extra_cycles;
	}

	void Cpu::ana(uint16_t addr, uint8_t extra_cycles)
	{
		_regs.a &= _mem.read_cpu_byte(addr);
		_regs.set_zn_flags(_regs.a);
		_cycles += 2 + extra_cycles;
	}

	void Cpu::ora(uint16_t addr, uint8_t extra_cycles)
	{
		_regs.a |= _mem.read_cpu_byte(addr);
		_regs.set_zn_flags(_regs.a);
		_cycles += 2 + extra_cycles;
	}

	void Cpu::eor(uint16_t addr, uint8_t extra_cycles)
	{
		_regs.a ^= _mem.read_cpu_byte(addr);
		_regs.set_zn_flags(_regs.a);
		_cycles += 2 + extra_cycles;
	}

	void Cpu::inc_dec(uint16_t addr, uint8_t change, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr) + change;
		_mem.write_cpu_byte(addr, byte);

		_regs.set_zn_flags(byte);
		_cycles += 5 + extra_cycles;
	}

	void Cpu::bit(uint16_t addr, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);

		_regs.set_status(Status::ZERO, (byte & _regs.a) == 0);
		_regs.set_status(Status::OVERFLOW, byte & 0x40);
		_regs.set_status(Status::NEGATIVE, byte & 0x80);

		_cycles += 3 + extra_cycles;
	}

	void Cpu::cmp(uint16_t addr, uint8_t reg, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);

		_regs.set_status(Status::CARRY, reg >= byte);
		_regs.set_status(Status::ZERO, reg == byte);
		_regs.set_status(Status::NEGATIVE, ((reg - byte) & 0x80) != 0);

		_cycles += 2 + extra_cycles;
	}

	void Cpu::lsr(uint16_t addr, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);
		_regs.set_status(Status::CARRY, byte & 0x01);

		byte >>= 1;

		_regs.set_zn_flags(byte);
		_mem.write_cpu_byte(addr, byte);
		_cycles += 5 + extra_cycles;
	}

	void Cpu::lsr_a()
	{
		_regs.set_status(Status::CARRY, _regs.a & 0x01);

		_regs.a >>= 1;

		_regs.set_zn_flags(_regs.a);
		_cycles += 2;
	}

	void Cpu::asl(uint16_t addr, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);
		_regs.set_status(Status::CARRY, byte & 0x80);

		byte <<= 1;

		_regs.set_zn_flags(byte);
		_mem.write_cpu_byte(addr, byte);
		_cycles += 5 + extra_cycles;
	}

	void Cpu::asl_a()
	{
		_regs.set_status(Status::CARRY, _regs.a & 0x80);

		_regs.a <<= 1;

		_regs.set_zn_flags(_regs.a);
		_cycles += 2;
	}

	void Cpu::ror(uint16_t addr, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);
		uint8_t carry = _regs.check_status(Status::CARRY);

		_regs.set_status(Status::CARRY, byte & 0x01);
		byte = (byte >> 1) | (carry << 7);

		_regs.set_zn_flags(byte);
		_mem.write_cpu_byte(addr, byte);
		_cycles += 5 + extra_cycles;
	}

	void Cpu::ror_a()
	{
		uint8_t carry = _regs.check_status(Status::CARRY);

		_regs.set_status(Status::CARRY, _regs.a & 0x01);
		_regs.a = (_regs.a >> 1) | (carry << 7);

		_regs.set_zn_flags(_regs.a);
		_cycles += 2;
	}

	void Cpu::rol(uint16_t addr, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);
		uint8_t carry = _regs.check_status(Status::CARRY);

		_regs.set_status(Status::CARRY, byte & 0x80);
		byte = (byte << 1) | carry;

		_regs.set_zn_flags(byte);
		_mem.write_cpu_byte(addr, byte);
		_cycles += 5 + extra_cycles;
	}

	void Cpu::rol_a()
	{
		uint8_t carry = _regs.check_status(Status::CARRY);

		_regs.set_status(Status::CARRY, _regs.a & 0x80);
		_regs.a = (_regs.a << 1) | carry;

		_regs.set_zn_flags(_regs.a);
		_cycles += 2;
	}
}
