#pragma once

#include <unordered_map>
#include <string>
#include <SDL2/SDL.h>

#include "cpu.h"
#include "ppu.h"
#include "memory.h"

namespace Nes {
	enum class Input {
		Pause
	};

	class Window {
		public:
			Window(uint32_t width, uint32_t height);
			~Window();

			void update();
			void render();

			bool is_open() const;
			SDL_Renderer *get_renderer();
		private:
			SDL_Window *_window;
			SDL_Renderer *_renderer;

			bool _open;
			uint32_t _width;
			uint32_t _height;
			
			std::unordered_map<Input, bool> _input;
	};

	class Emulator {
		public:
			Emulator();

			bool load_rom(std::string filename);
			void run();
		private:
			Window _window;

			Memory _memory;
			Ppu _ppu;
			Cpu _cpu;
	};
}
