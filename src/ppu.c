#include <stdio.h>

#include "ppu.h"
#include "memory.h"
#include "cpu.h"

#define VISIBLE_SCANLINE_END         239
#define POST_RENDER_SCANLINE         240
#define FIRST_VERTICAL_BLANKING_LINE 241
#define VERTICAL_BLANKING_LINES_END  260
#define LAST_CYCLE                   341
#define SCANLINE_MAX                 262

static void PreRenderScanline(Ppu *ppu);
static void VisibleScanlines(Ppu *ppu);
static void VerticalBlankingLines(Ppu *ppu);
static void PostRenderScanline(Ppu *ppu);

void PpuInit(Ppu *ppu, Memory *mem, uint64_t *totalCycles) {
    ppu->mem = mem;
    ppu->oddFrame = 0;
    ppu->scanline = SCANLINE_MAX - 1;
    ppu->cycle = 0;

    ppu->totalCycles = totalCycles;

    ppu->needsNmi = 0;
}

void PpuEmulate(Ppu *ppu) {
    if (ppu->scanline <= VISIBLE_SCANLINE_END) {
        VisibleScanlines(ppu);
    } else if (ppu->scanline == POST_RENDER_SCANLINE) {
        PostRenderScanline(ppu);
    } else if (ppu->scanline <= VERTICAL_BLANKING_LINES_END) {
        if (ppu->scanline == FIRST_VERTICAL_BLANKING_LINE && ppu->cycle == 0)
            SetPpuRegisterBit(ppu->mem, PPUSTATUS, PPUSTATUS_VERTICAL_BLANK_STARTED_BIT, 1);
        
        // This only needs to run once each VBlank Period.
        if (ppu->mem->ppustatusRead) {
            SetPpuRegisterBit(ppu->mem, PPUSTATUS, PPUSTATUS_VERTICAL_BLANK_STARTED_BIT, 0);
            ppu->mem->ppustatusRead = 0;
        }        
        
        VerticalBlankingLines(ppu);
    } else { /* scanline == 261 */
        if (ppu->cycle == 0) {
            SetPpuRegisterBit(ppu->mem, PPUSTATUS, PPUSTATUS_VERTICAL_BLANK_STARTED_BIT, 0);
        }

        PreRenderScanline(ppu);
    }

    if (++ppu->cycle == LAST_CYCLE) {
        /* puts("Next scanline");*/
        ppu->scanline = (ppu->scanline + 1) % SCANLINE_MAX;
        ppu->cycle = 0;
    }

    ppu->needsNmi = GetPpuRegisterBit(ppu->mem, PPUCTRL, PPUCTRL_GENERATE_NMI_AT_VBLANK_BIT) && 
                    GetPpuRegisterBit(ppu->mem, PPUSTATUS, PPUSTATUS_VERTICAL_BLANK_STARTED_BIT);
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