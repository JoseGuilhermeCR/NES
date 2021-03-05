#include "ppu.h"
#include "memory.h"

void
init_ppu(struct ppu *ppu, struct memory *mem)
{
	ppu->mem = mem;
	ppu->odd_frame = 0;
}

void
emulate_ppu(struct ppu *ppu)
{

}
