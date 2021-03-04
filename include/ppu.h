#pragma once

#include <cstdint>

namespace Nes {
	class Memory;
	class Window;

	enum class PpuRegisters {
		ppuctrl   = 0x2000,
		ppumask   = 0x2001,
		ppustatus = 0x2002,
		oamaddr   = 0x2003,
		oamdata   = 0x2004,
		ppuscroll = 0x2005,
		ppuaddr   = 0x2006,
		ppudata   = 0x2007,
		oamdma    = 0x4014
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
