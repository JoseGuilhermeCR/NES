#include "ppu.h"
#include "memory.h"

#include <iostream>

namespace Nes {
	Ppu::Ppu(Memory& memory, Window& window)
		:
		_window(window),
		_memory(memory),
		_cycle(0),
		_scanline(261),
		_odd_frame(false)
	{
	}

	void Ppu::emulate() {
		if (_scanline <= 239) {
			visible_scanlines();
		} else if (_scanline == 240) {
			post_render_scanline();
		} else if (_scanline <= 260) {
			vertical_blanking_lines();
		} else { // scanline == 261
			pre_render_scanline();
		}

		if (++_cycle == 341) {
			std::cout << "Next scanline\n";
			_scanline = (_scanline + 1) % 262;
			_cycle = 0;
		}
	}

	void Ppu::pre_render_scanline() {
		
	}

	void Ppu::visible_scanlines() {
		// Cycle 0 is idle
		if (_cycle >= 1 && _cycle <= 256) {

		}
	}

	void Ppu::vertical_blanking_lines() {
		std::cout << "vertical_blanking_lines\n";
	}

	void Ppu::post_render_scanline() {
		std::cout << "post_render_scanline\n";
	}
}
