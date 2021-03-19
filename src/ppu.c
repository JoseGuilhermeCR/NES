#include <stdio.h>

#include "ppu.h"
#include "memory.h"

static void PreRenderScanline(Ppu *ppu);
static void VisibleScanlines(Ppu *ppu);
static void VerticalBlankingLines(Ppu *ppu);
static void PostRenderScanline( Ppu *ppu);

void PpuInit(Ppu *ppu, Memory *mem) {
    ppu->mem = mem;
    ppu->oddFrame = 0;
    ppu->scanline = 261;
    ppu->cycle = 0;
}

void PpuEmulate(Ppu *ppu) {
    if (ppu->scanline <= 239) {
        VisibleScanlines(ppu);
    } else if (ppu->scanline == 240) {
        PostRenderScanline(ppu);
    } else if (ppu->scanline <= 260) {
        VerticalBlankingLines(ppu);
    } else { /* scanline == 261 */
        PreRenderScanline(ppu);
    }

    if (++ppu->cycle == 341) {
        /* puts("Next scanline");*/
        ppu->scanline = (ppu->scanline + 1) % 262;
        ppu->cycle = 0;
    }
}

static void PreRenderScanline(Ppu *ppu) {

}

static void VisibleScanlines(Ppu *ppu) {
    if (ppu->cycle >= 1 && ppu->cycle <= 256) {

    }
}

static void VerticalBlankingLines(Ppu *ppu) {
    /*puts("vertical_blanking_lines");*/
}

static void PostRenderScanline(Ppu *ppu) {
    /*puts("post_render_scanline");*/
}