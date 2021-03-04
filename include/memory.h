#pragma once

#include <array>
#include <memory>

#include "cartridge.h"

namespace Nes {

	class Memory {
		public:
			Memory();

			void attach_cartridge(std::unique_ptr<Cartridge>& cart);

			bool is_cartridge_attached() const;

			void write_cpu_byte(uint16_t addr, uint8_t byte);
			uint8_t read_cpu_byte(uint16_t addr);

			void write_ppu_byte(uint16_t addr, uint8_t byte);
			uint8_t read_ppu_byte(uint16_t addr);
		private:
			std::array<uint8_t, 2048> _cpuram;
			std::array<uint8_t, 8> _ppuregs;
			std::array<uint8_t, 2048> _ppuram;
			std::unique_ptr<Cartridge> _cart;
	};
}
