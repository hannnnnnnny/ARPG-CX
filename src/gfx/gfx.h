/*
 * gfx.h - software renderer into a 320x240 RGB565 framebuffer.
 *
 * RGB565 is the native format of the TI-Nspire CX LCD (lcd_blit with
 * SCR_320x240_565), so the calculator build presents the buffer with no
 * conversion. The desktop build converts to 32-bit when presenting.
 *
 * Game code always works in 320x240 logical pixels. The desktop build
 * (GFX_HD) renders into a 640x480 buffer: every logical pixel becomes a
 * 2x2 block, except text, which draws its 12px CJK glyphs at the full
 * resolution (gfx_pixel_hd) so Chinese stays crisp.
 */
#ifndef AD_GFX_H
#define AD_GFX_H

#include "../core/common.h"

#define RGB565(r, g, b) \
    ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | (((b) & 0xF8) >> 3)))

/* Magenta marks transparent pixels in sprites. Never used as a real colour. */
#define COLOR_KEY 0xF81F

#ifdef GFX_HD
#define GFX_S 2
#else
#define GFX_S 1
#endif
#define FB_W (SCREEN_W * GFX_S)   /* framebuffer size in physical pixels */
#define FB_H (SCREEN_H * GFX_S)

typedef struct {
    int16_t w, h;
    const uint16_t *px; /* w*h pixels, row major */
} Sprite;

enum {
    BLIT_FLIP_X = 1u << 0,
    BLIT_FLIP_Y = 1u << 1,
    BLIT_WHITE  = 1u << 2  /* draw every opaque pixel white (hit flash) */
};

void      gfx_bind(uint16_t *fb);
uint16_t *gfx_target(void);

void gfx_clear(uint16_t c);
void gfx_pixel(int x, int y, uint16_t c);
/* One physical pixel: (x, y) logical origin plus (dx, dy) in 1/GFX_S steps. */
void gfx_pixel_hd(int x, int y, int dx, int dy, uint16_t c);
void gfx_fill_rect(int x, int y, int w, int h, uint16_t c);
void gfx_rect(int x, int y, int w, int h, uint16_t c);
void gfx_hline(int x, int y, int w, uint16_t c);
void gfx_vline(int x, int y, int h, uint16_t c);
/* Halve brightness of a region (pause/dialog backdrops). */
void gfx_dim_rect(int x, int y, int w, int h);
/* 50% blend of a colour over a region (light beams, glows). */
void gfx_blend_rect(int x, int y, int w, int h, uint16_t c);

void gfx_line(int x0, int y0, int x1, int y1, uint16_t c);
void gfx_circle(int cx, int cy, int r, uint16_t c);
void gfx_blit(const Sprite *s, int x, int y, unsigned flags);
/* Nearest-neighbour integer upscale (menus only; not used in the hot path). */
void gfx_blit_scaled(const Sprite *s, int x, int y, int scale);
/* Fast path for 16x16 world tiles. 'opaque' tiles skip the colour-key test. */
void gfx_blit_tile(const uint16_t *px, int x, int y, bool opaque);

#endif
