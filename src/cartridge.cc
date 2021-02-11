#include "cartridge.h"

#include <fstream>

namespace Nes {
	Cartridge::Cartridge(std::string filename)
		:
		_prg_banks(1),
		_chr_banks(1),
		_prg(),
		_chr(),
		_mapper(nullptr)
	{
		std::ifstream rom(filename, std::ios::binary);

		// Just nestest for now
		_prg.resize(_prg_banks * KIB_16);
		_chr.resize(_chr_banks * KIB_8);

		_mapper = std::make_unique<Mapper0>(_prg_banks, _chr_banks);

		rom.seekg(16);
		rom.read(reinterpret_cast<char *>(_prg.data()), _prg_banks * KIB_16);
		rom.read(reinterpret_cast<char *>(_chr.data()), _chr_banks * KIB_8);
	}
	
	void Cartridge::write_cpu_byte(uint16_t addr, uint8_t byte)
	{
		uint32_t decoded = _mapper->map_cpu_write(addr);
		_prg.at(decoded) = byte;
	}

	uint8_t Cartridge::read_cpu_byte(uint16_t addr)
	{
		uint32_t decoded = _mapper->map_cpu_read(addr);
		return _prg.at(decoded);
	}

	void Cartridge::write_ppu_byte(uint16_t addr, uint8_t byte) {
		uint32_t decoded = _mapper->map_ppu_write(addr);
		_chr.at(decoded) = byte;
	}
	
	uint8_t Cartridge::read_ppu_byte(uint16_t addr) {
		uint32_t decoded = _mapper->map_ppu_read(addr);
		return _chr.at(decoded);
	}


	Mapper::Mapper(uint8_t prg_banks, uint8_t chr_banks)
		:
		_prg_banks(prg_banks),
		_chr_banks(chr_banks)
	{
	}

	Mapper0::Mapper0(uint8_t prg_banks, uint8_t chr_banks)
		: Mapper(prg_banks, chr_banks)
	{
	}

	uint32_t Mapper0::map_cpu_write(uint16_t addr) {
		return addr & (_prg_banks == 1 ? 0x3FFF : 0x7FFF);
	}

	uint32_t Mapper0::map_cpu_read(uint16_t addr) {
		return addr & (_prg_banks == 1 ? 0x3FFF : 0x7FFF);
	}

	uint32_t Mapper0::map_ppu_write(uint16_t addr) {
		return addr;
	}

	uint32_t Mapper0::map_ppu_read(uint16_t addr) {
		return addr;
	}
}
