#include "ppu.h"
#include "memory.h"

namespace Nes {
	Ppu::Ppu(Memory &memory, Window &window)
		:
		_window(window),
		_memory(memory),
		_odd_frame(false)
	{
	}

	void Ppu::emulate() {

	}
}
