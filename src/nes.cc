#include "nes.h"

namespace Nes {
	Window::Window(uint32_t width, uint32_t height)
		:
		_window(nullptr),
		_renderer(nullptr),
		_open(false),
		_width(width),
		_height(height),
		_input()
	{
		SDL_Init(SDL_INIT_VIDEO);

		_window = SDL_CreateWindow(
				"NES",
				SDL_WINDOWPOS_CENTERED,
				SDL_WINDOWPOS_CENTERED,
				_width,
				_height,
				0
				);

		_renderer = SDL_CreateRenderer(
				_window,
				-1,
				SDL_RENDERER_ACCELERATED
				);

		_input.emplace(Input::Pause, false);

		_open = true;
	}

	Window::~Window() {
		SDL_DestroyRenderer(_renderer);
		SDL_DestroyWindow(_window);
		SDL_Quit();
	}

	void Window::update() {
		for (auto &[_, value] : _input) {
			value = false;
		}

		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				_open = false;
			else if (e.type == SDL_KEYDOWN)
				if (e.key.keysym.sym == SDLK_p)
					_input[Input::Pause] = true;
		}

	}

	void Window::render() {
		SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
		SDL_RenderClear(_renderer);

		//SDL_SetRenderDrawColor(_renderer, 128, 128, 128, 255);
		//SDL_RenderFillRect(_renderer, &NES_RECT);
		SDL_RenderPresent(_renderer);

//		SDL_Delay(16);
	}

	bool Window::is_open() const {
		return _open;
	}

	SDL_Renderer *Window::get_renderer() {
		return _renderer;
	}

	Emulator::Emulator()
		:
		_window(256 * 4, 240 * 4),
		_memory(),
		_ppu(_memory, _window),
		_cpu(_memory)
	{
	}

	bool Emulator::load_rom(std::string filename) {
		auto cart = std::make_unique<Cartridge>(filename);
		_memory.attach_cartridge(cart);
		return true;
	}

	void Emulator::run() {
		if (!_memory.is_cartridge_attached())
			return;

		while (_window.is_open()) {
			_window.update();

			_cpu.emulate();
			_ppu.emulate();
			_ppu.emulate();
			_ppu.emulate();

			_window.render();
		}
	}
}

int main()
{
	Nes::Emulator emulator;
	//if (emulator.load_rom("test_roms/nestest.nes"))
	//	emulator.run();
	if (emulator.load_rom("test_roms/dk.nes"))
		emulator.run();

	return 0;
}
