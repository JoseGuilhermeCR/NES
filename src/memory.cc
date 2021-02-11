#include "memory.h"
#include "cartridge.h"

namespace Nes {
	Memory::Memory()
		:
		_cart(nullptr)
	{
	}

	void Memory::attach_cartridge(std::unique_ptr<Cartridge> &cart) {
		_cart = std::move(cart);
	}

	bool Memory::is_cartridge_attached() const {
		return _cart != nullptr;
	}

	void Memory::write_cpu_byte(uint16_t addr, uint8_t byte)
	{
		if (addr <= 0x1FFF) {
			_cpuram[addr & 0x7FF] = byte;
		} else if (addr >= 0x2000 && addr <= 0x3FFF) {
			_ppuregs[addr & 0x7] = byte;
		} else if (addr >= 0x4000 && addr <= 0x4017) {
		} else if (addr >= 0x4020) {
			_cart->write_cpu_byte(addr, byte);
		}
	}

	uint8_t Memory::read_cpu_byte(uint16_t addr)
	{
		if (addr <= 0x1FFF) {
			return _cpuram[addr & 0x7FF];
		} else if (addr >= 0x2000 && addr <= 0x3FFF) {
			return _ppuregs[addr & 0x7];
		} else if (addr >= 0x4000 && addr <= 0x4017) {
			return 0;
		} else if (addr >= 0x4020) {
			return _cart->read_cpu_byte(addr);
		}

		return 0;
	}

	void Memory::write_ppu_byte(uint16_t addr, uint8_t byte)
	{
		if (addr <= 0x1FFF) {

		} else if (addr >= 0x2000 && addr <= 0x2FFF) {
		} else if (addr >= 0x3000 && addr <= 0x3EFF) {
		} else if (addr >= 0x3F00 && addr <= 0x3FFF) {
		}
	}

	uint8_t Memory::read_ppu_byte(uint16_t addr)
	{
		if (addr <= 0x1FFF) {				
		} else if (addr >= 0x2000 && addr <= 0x2FFF) {
		} else if (addr >= 0x3000 && addr <= 0x3EFF) {
		} else if (addr >= 0x3F00 && addr <= 0x3FFF) {
		}
		return 0;
	}
}
