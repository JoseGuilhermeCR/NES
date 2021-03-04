#pragma once

#include <cstdint>

namespace Nes {
	class Memory;
	class Window;

	enum class PpuRegisters {
		PPUCTRL   = 0x2000,
		PPUMASK   = 0x2001,
		PPUSTATUS = 0x2002,
		OAMADDR   = 0x2003,
		OAMDATA   = 0x2004,
		PPUSCROLL = 0x2005,
		PPUADDR   = 0x2006,
		PPUDATA   = 0x2007,
		OAMDMA    = 0x4014
	};

	class Ppu {
		public:
			Ppu(Memory& memory, Window& window);

			void emulate();
		private:
			void pre_render_scanline();
			void visible_scanlines();
			void vertical_blanking_lines();
			void post_render_scanline();

			Window& _window;
			Memory& _memory;

			uint16_t _cycle;
			uint16_t _scanline;
			bool _odd_frame;
	};
}
