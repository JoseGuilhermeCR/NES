#include <stdio.h>

#include "ppu.h"
#include "memory.h"

static void pre_render_scanline(struct Ppu *ppu);
static void visible_scanlines(struct Ppu *ppu);
static void vertical_blanking_lines(struct Ppu *ppu);
static void post_render_scanline(struct Ppu *ppu);

void
ppu_init(struct Ppu *ppu, struct Memory *mem)
{
	ppu->mem = mem;
	ppu->odd_frame = 0;
	ppu->scanline = 261;
	ppu->cycle = 0;
}

void
ppu_emulate(struct Ppu *ppu)
{
	if (ppu->scanline <= 239) {
		visible_scanlines(ppu);
	} else if (ppu->scanline == 240) {
		post_render_scanline(ppu);
	} else if (ppu->scanline <= 260) {
		vertical_blanking_lines(ppu);
	} else { /* scanline == 261 */
		pre_render_scanline(ppu);
	}

	if (++ppu->cycle == 341) {
		/* puts("Next scanline");*/
		ppu->scanline = (ppu->scanline + 1) % 262;
		ppu->cycle = 0;
	}
}

static void
pre_render_scanline(struct Ppu *ppu)
{

}

static void
visible_scanlines(struct Ppu *ppu)
{
	if (ppu->cycle >= 1 && ppu->cycle <= 256) {

	}
}

static void
vertical_blanking_lines(struct Ppu *ppu)
{
	/*puts("vertical_blanking_lines");*/
}

static void
post_render_scanline(struct Ppu *ppu)
{
	/*puts("post_render_scanline");*/
}