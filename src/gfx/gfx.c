#include "gfx.h"
#include <string.h>

/* The renderer draws into exactly one framebuffer, owned by the platform. */
static uint16_t *g_fb;

void gfx_bind(uint16_t *fb) { g_fb = fb; }
uint16_t *gfx_target(void) { return g_fb; }

void gfx_clear(uint16_t c)
{
    int i;
    if (c == 0) {
        memset(g_fb, 0, SCREEN_W * SCREEN_H * sizeof(uint16_t));
        return;
    }
    for (i = 0; i < SCREEN_W * SCREEN_H; i++)
        g_fb[i] = c;
}

void gfx_pixel(int x, int y, uint16_t c)
{
    if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H)
        g_fb[y * SCREEN_W + x] = c;
}

/* Clip a rectangle to the screen. Returns false if nothing remains. */
static bool clip_rect(int *x, int *y, int *w, int *h)
{
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > SCREEN_W) *w = SCREEN_W - *x;
    if (*y + *h > SCREEN_H) *h = SCREEN_H - *y;
    return *w > 0 && *h > 0;
}

void gfx_fill_rect(int x, int y, int w, int h, uint16_t c)
{
    int i, j;
    if (!clip_rect(&x, &y, &w, &h))
        return;
    for (j = 0; j < h; j++) {
        uint16_t *row = g_fb + (y + j) * SCREEN_W + x;
        for (i = 0; i < w; i++)
            row[i] = c;
    }
}

void gfx_hline(int x, int y, int w, uint16_t c) { gfx_fill_rect(x, y, w, 1, c); }
void gfx_vline(int x, int y, int h, uint16_t c) { gfx_fill_rect(x, y, 1, h, c); }

void gfx_rect(int x, int y, int w, int h, uint16_t c)
{
    gfx_hline(x, y, w, c);
    gfx_hline(x, y + h - 1, w, c);
    gfx_vline(x, y, h, c);
    gfx_vline(x + w - 1, y, h, c);
}

void gfx_dim_rect(int x, int y, int w, int h)
{
    int i, j;
    if (!clip_rect(&x, &y, &w, &h))
        return;
    for (j = 0; j < h; j++) {
        uint16_t *row = g_fb + (y + j) * SCREEN_W + x;
        /* Shift each 565 channel right by one: mask drops the bits that
         * would bleed into the neighbouring channel. */
        for (i = 0; i < w; i++)
            row[i] = (uint16_t)((row[i] >> 1) & 0x7BEF);
    }
}

void gfx_blit(const Sprite *s, int x, int y, unsigned flags)
{
    int sx0 = 0, sy0 = 0, w = s->w, h = s->h;
    int dx = x, dy = y, i, j;

    /* Clip in destination space, remember how much of the source was cut. */
    if (dx < 0) { sx0 = -dx; w += dx; dx = 0; }
    if (dy < 0) { sy0 = -dy; h += dy; dy = 0; }
    if (dx + w > SCREEN_W) w = SCREEN_W - dx;
    if (dy + h > SCREEN_H) h = SCREEN_H - dy;
    if (w <= 0 || h <= 0)
        return;

    for (j = 0; j < h; j++) {
        int srow = sy0 + j;
        const uint16_t *src;
        uint16_t *dst = g_fb + (dy + j) * SCREEN_W + dx;
        if (flags & BLIT_FLIP_Y)
            srow = s->h - 1 - srow;
        src = s->px + srow * s->w;
        if (flags & BLIT_FLIP_X) {
            /* Destination column i maps to source column (w_full-1-(sx0+i)). */
            const uint16_t *p = src + s->w - 1 - sx0;
            for (i = 0; i < w; i++, p--) {
                uint16_t c = *p;
                if (c != COLOR_KEY)
                    dst[i] = (flags & BLIT_WHITE) ? 0xFFFF : c;
            }
        } else {
            const uint16_t *p = src + sx0;
            for (i = 0; i < w; i++) {
                uint16_t c = p[i];
                if (c != COLOR_KEY)
                    dst[i] = (flags & BLIT_WHITE) ? 0xFFFF : c;
            }
        }
    }
}

void gfx_blit_tile(const uint16_t *px, int x, int y, bool opaque)
{
    int j, i;
    /* Fully on-screen opaque tiles are the overwhelmingly common case:
     * 16 row copies of 32 bytes each. */
    if (opaque && x >= 0 && y >= 0 && x + TILE_SIZE <= SCREEN_W && y + TILE_SIZE <= SCREEN_H) {
        uint16_t *dst = g_fb + y * SCREEN_W + x;
        for (j = 0; j < TILE_SIZE; j++, dst += SCREEN_W, px += TILE_SIZE)
            memcpy(dst, px, TILE_SIZE * sizeof(uint16_t));
        return;
    }
    if (!opaque && x >= 0 && y >= 0 && x + TILE_SIZE <= SCREEN_W && y + TILE_SIZE <= SCREEN_H) {
        uint16_t *dst = g_fb + y * SCREEN_W + x;
        for (j = 0; j < TILE_SIZE; j++, dst += SCREEN_W, px += TILE_SIZE)
            for (i = 0; i < TILE_SIZE; i++)
                if (px[i] != COLOR_KEY)
                    dst[i] = px[i];
        return;
    }
    {
        /* Edge of screen: fall back to the general clipped blit. */
        Sprite s;
        s.w = TILE_SIZE;
        s.h = TILE_SIZE;
        s.px = px;
        gfx_blit(&s, x, y, 0);
    }
}

void gfx_line(int x0, int y0, int x1, int y1, uint16_t c)
{
    int dx = ABS(x1 - x0), dy = -ABS(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy, guard = 0;
    while (guard++ < 2048) { /* Bresenham, bounded so bad input can't hang */
        int e2;
        gfx_pixel(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_circle(int cx, int cy, int r, uint16_t c)
{
    int x = r, y = 0, err = 1 - r;
    if (r <= 0)
        return;
    while (x >= y) { /* midpoint circle */
        gfx_pixel(cx + x, cy + y, c); gfx_pixel(cx - x, cy + y, c);
        gfx_pixel(cx + x, cy - y, c); gfx_pixel(cx - x, cy - y, c);
        gfx_pixel(cx + y, cy + x, c); gfx_pixel(cx - y, cy + x, c);
        gfx_pixel(cx + y, cy - x, c); gfx_pixel(cx - y, cy - x, c);
        y++;
        if (err < 0) {
            err += 2 * y + 1;
        } else {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}

void gfx_blit_scaled(const Sprite *s, int x, int y, int scale)
{
    int i, j;
    for (j = 0; j < s->h; j++)
        for (i = 0; i < s->w; i++) {
            uint16_t c = s->px[j * s->w + i];
            if (c != COLOR_KEY)
                gfx_fill_rect(x + i * scale, y + j * scale, scale, scale, c);
        }
}

void gfx_blend_rect(int x, int y, int w, int h, uint16_t c)
{
    int i, j;
    uint16_t half = (uint16_t)((c >> 1) & 0x7BEF);
    if (!clip_rect(&x, &y, &w, &h))
        return;
    for (j = 0; j < h; j++) {
        uint16_t *row = g_fb + (y + j) * SCREEN_W + x;
        /* 50% mix: halve both colours per channel, then add */
        for (i = 0; i < w; i++)
            row[i] = (uint16_t)(((row[i] >> 1) & 0x7BEF) + half);
    }
}
