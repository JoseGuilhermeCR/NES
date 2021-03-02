#pragma once

#include <string>
#include <memory>
#include <vector>

namespace Nes {
	constexpr uint32_t KIB_16 = 16 * 1024;
	constexpr uint32_t KIB_8 = 8 * 1024;

	class Mapper {
		public:
			Mapper(uint8_t prg_banks, uint8_t chr_banks);
			virtual uint32_t map_cpu_write(uint16_t addr) = 0;
			virtual uint32_t map_ppu_write(uint16_t addr) = 0;
			virtual uint32_t map_cpu_read(uint16_t addr) = 0;
			virtual uint32_t map_ppu_read(uint16_t addr) = 0;
		protected:
			uint8_t _prg_banks;
			uint8_t _chr_banks;
	};

	class Mapper0 : public Mapper {
		public:
			Mapper0(uint8_t prg_banks, uint8_t chr_banks);
			uint32_t map_cpu_write(uint16_t addr) override;
			uint32_t map_ppu_write(uint16_t addr) override;
			uint32_t map_cpu_read(uint16_t addr) override;
			uint32_t map_ppu_read(uint16_t addr) override;
	};


	class Cartridge {
		public:
			Cartridge(std::string filename);

			void write_cpu_byte(uint16_t addr, uint8_t byte);
			uint8_t read_cpu_byte(uint16_t addr);
			void write_ppu_byte(uint16_t addr, uint8_t byte);
			uint8_t read_ppu_byte(uint16_t addr);
		private:
			uint8_t _prg_banks;
			uint8_t _chr_banks;
			std::vector<uint8_t> _prg;
			std::vector<uint8_t> _chr;
			std::unique_ptr<Mapper> _mapper;
	};
}
