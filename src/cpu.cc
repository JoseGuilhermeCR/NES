#include "cpu.h"
#include "memory.h"

#include <iostream>
#include <cassert>

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
		set_status(Status::negative, value & 0x80);
		set_status(Status::zero, value == 0);	
	}

	bool Interrupt::needs_handle() const {
		return _value != 0;
	}

	void Interrupt::set(InterruptType it, bool active) {
		if (active)
			_value |= static_cast<uint8_t>(it);
		else
			_value &= ~(static_cast<uint8_t>(it));
	}

	std::optional<uint16_t> Interrupt::get_handler() {
		if ((_value & static_cast<uint8_t>(InterruptType::reset)) != 0) {
			set(InterruptType::reset, false);
			return 0xFFFC;
		} else if ((_value & static_cast<uint8_t>(InterruptType::nmi)) != 0) {
			set(InterruptType::nmi, false);
			return 0xFFFA;
		} else if ((_value & static_cast<uint8_t>(InterruptType::irq)) != 0) {
			set(InterruptType::irq, false);
			return 0xFFFE;
		}

		return std::nullopt;
	}

	Cpu::Cpu(Memory &memory)
		:
		_mem(memory),
		_interrupt(),
		_cycles(),
		_current_cycle()
	{
		_regs.pc = 0x0000;
		_regs.sp = 0xFF;
		_regs.a  = 0x00;
		_regs.x  = 0x00;
		_regs.y  = 0x00;
		_regs.s  = 0x20;

		// On startup, reset is active.
		_interrupt.set(InterruptType::reset, true);
	}

	void Cpu::push_stack(uint8_t byte) {
		_mem.write_cpu_byte(_regs.sp-- + 0x0100, byte);	
	}

	uint8_t Cpu::pop_stack() {
		return _mem.read_cpu_byte(++_regs.sp + 0x0100);
	}

	void Cpu::request_interrupt(InterruptType it) {
		if (!_regs.check_status(Status::interrupt_disable))
			_interrupt.set(it, true);
	}

	void Cpu::handle_interrupt() {
		uint16_t handler = _interrupt.get_handler().value();

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

	uint16_t Cpu::absolute_x() {
		return absolute_indexed(_regs.x);
	}

	uint16_t Cpu::absolute_y() {
		return absolute_indexed(_regs.y);
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

	uint16_t Cpu::zeropage_x() {
		return zeropage_indexed(_regs.x);
	}

	uint16_t Cpu::zeropage_y() {
		return zeropage_indexed(_regs.y);
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
		constexpr bool debug = true;

		if (_current_cycle < _cycles) {
			++_current_cycle;
			return;
		}

		if (_interrupt.needs_handle()) {
			handle_interrupt();
			return;
		}

		_cycles = 0;
		_current_cycle = 0;

		const uint8_t opcode = _mem.read_cpu_byte(_regs.pc);

		if (debug) {
			const uint8_t op1 = _mem.read_cpu_byte(_regs.pc + 1);
			const uint8_t op2 = _mem.read_cpu_byte(_regs.pc + 2);
			std::cout << std::hex;
			std::cout << static_cast<uint16_t>(opcode) << ' ' << static_cast<uint16_t>(op1) << ' ' << static_cast<uint16_t>(op2) << '\n';
			std::cout << std::dec;
		}

		
		const std::optional<Instruction>& possible_instr = s_instruction_table.at(opcode);

		if (possible_instr) {
			const Instruction& instr = possible_instr.value();
			assert(opcode == instr.opcode);

			instr.execute(*this, 0);

			_cycles += instr.cycles;
		} else {
			std::cout << std::hex << "Unknown opcode: " << opcode << std::dec << '\n';
		}

		++_regs.pc;
	}

	void Cpu::load(uint16_t addr, uint8_t& reg, uint8_t extra_cycles)
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

		uint8_t a = _regs.a + byte + _regs.check_status(Status::carry);
		bool overflow = (byte & 0x80) == (_regs.a & 0x80) && (byte & 0x80) != (a & 0x80);
		bool carry = ((static_cast<uint16_t>(_regs.a) + static_cast<uint16_t>(byte)) & 0xFF00) != 0;

		_regs.set_status(Status::overflow, overflow);
		_regs.set_zn_flags(a);
		_regs.set_status(Status::carry, carry);

		_regs.a = a;
		_cycles += 2 + extra_cycles;
	}

	void Cpu::ana(uint16_t addr, uint8_t extra_cycles)
	{
		_regs.a &= _mem.read_cpu_byte(addr);
		_regs.set_zn_flags(_regs.a);
		_cycles += 2 + extra_cycles;
	}

	void Cpu::ora(uint16_t addr)
	{
		_regs.a |= _mem.read_cpu_byte(addr);
		_regs.set_zn_flags(_regs.a);
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

		_regs.set_status(Status::zero, (byte & _regs.a) == 0);
		_regs.set_status(Status::overflow, byte & 0x40);
		_regs.set_status(Status::negative, byte & 0x80);

		_cycles += 3 + extra_cycles;
	}

	void Cpu::cmp(uint16_t addr, uint8_t reg, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);

		_regs.set_status(Status::carry, reg >= byte);
		_regs.set_status(Status::zero, reg == byte);
		_regs.set_status(Status::negative, ((reg - byte) & 0x80) != 0);

		_cycles += 2 + extra_cycles;
	}

	void Cpu::lsr(uint16_t addr, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);
		_regs.set_status(Status::carry, byte & 0x01);

		byte >>= 1;

		_regs.set_zn_flags(byte);
		_mem.write_cpu_byte(addr, byte);
		_cycles += 5 + extra_cycles;
	}

	void Cpu::lsr_a()
	{
		_regs.set_status(Status::carry, _regs.a & 0x01);

		_regs.a >>= 1;

		_regs.set_zn_flags(_regs.a);
		_cycles += 2;
	}

	void Cpu::asl(uint16_t addr, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);
		_regs.set_status(Status::carry, byte & 0x80);

		byte <<= 1;

		_regs.set_zn_flags(byte);
		_mem.write_cpu_byte(addr, byte);
		_cycles += 5 + extra_cycles;
	}

	void Cpu::asl_a()
	{
		_regs.set_status(Status::carry, _regs.a & 0x80);

		_regs.a <<= 1;

		_regs.set_zn_flags(_regs.a);
		_cycles += 2;
	}

	void Cpu::ror(uint16_t addr, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);
		uint8_t carry = _regs.check_status(Status::carry);

		_regs.set_status(Status::carry, byte & 0x01);
		byte = (byte >> 1) | (carry << 7);

		_regs.set_zn_flags(byte);
		_mem.write_cpu_byte(addr, byte);
		_cycles += 5 + extra_cycles;
	}

	void Cpu::ror_a()
	{
		uint8_t carry = _regs.check_status(Status::carry);

		_regs.set_status(Status::carry, _regs.a & 0x01);
		_regs.a = (_regs.a >> 1) | (carry << 7);

		_regs.set_zn_flags(_regs.a);
		_cycles += 2;
	}

	void Cpu::rol(uint16_t addr, uint8_t extra_cycles)
	{
		uint8_t byte = _mem.read_cpu_byte(addr);
		uint8_t carry = _regs.check_status(Status::carry);

		_regs.set_status(Status::carry, byte & 0x80);
		byte = (byte << 1) | carry;

		_regs.set_zn_flags(byte);
		_mem.write_cpu_byte(addr, byte);
		_cycles += 5 + extra_cycles;
	}

	void Cpu::rol_a()
	{
		uint8_t carry = _regs.check_status(Status::carry);

		_regs.set_status(Status::carry, _regs.a & 0x80);
		_regs.a = (_regs.a << 1) | carry;

		_regs.set_zn_flags(_regs.a);
		_cycles += 2;
	}

	void Cpu::brk(uint16_t) {
		push_stack(_regs.pc >> 8);
		push_stack(_regs.pc & 0xFF);
		push_stack(_regs.s);

		uint8_t lo = _mem.read_cpu_byte(0xFFFE);
		uint8_t hi = _mem.read_cpu_byte(0xFFFF);
		_regs.pc = (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
		_regs.set_status(Status::brk, 1);
		_cycles += 7;
	}

	Instruction::Instruction(
			uint8_t opcode,
			uint8_t cycles,
			ADRMode addr_mode,
			std::function<void (Cpu&, uint16_t)> execute
			)
		:
		opcode(opcode),
		cycles(cycles),
		addr_mode(addr_mode),
		execute(execute)
	{
	}

	const std::vector<std::optional<Instruction>> Cpu::s_instruction_table {
		Instruction(0x00, 7, ADRMode::implicit, &Cpu::brk),
		Instruction(0x01, 6, ADRMode::indexed_indirect, &Cpu::ora)
	};
}
