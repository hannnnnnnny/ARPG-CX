#include "tiles.h"
#include "../core/rng.h"

#define T 16

static uint16_t g_tiles[TL_COUNT][SHADES][T * T];
static int g_theme = -1;

typedef struct { uint16_t floor, floor_dk, brick, brick_hi, mortar, cap; const char *name; } Palette;

static const Palette pal[THEME_COUNT] = {
    { RGB565(74, 72, 80),  RGB565(52, 50, 58), RGB565(96, 88, 86),  RGB565(130, 120, 112), RGB565(40, 36, 40), RGB565(28, 26, 32), "THE CRYPT" },
    { RGB565(88, 70, 52),  RGB565(62, 48, 36), RGB565(118, 88, 60), RGB565(150, 116, 80),  RGB565(48, 36, 26), RGB565(34, 26, 20), "CATACOMBS" },
    { RGB565(86, 44, 36),  RGB565(60, 28, 24), RGB565(128, 58, 40), RGB565(176, 86, 52),   RGB565(48, 18, 14), RGB565(36, 14, 12), "HELLFORGE" },
    { RGB565(66, 82, 104), RGB565(46, 58, 78), RGB565(90, 116, 146),RGB565(140, 170, 200), RGB565(34, 44, 60), RGB565(24, 30, 44), "FROZEN HALLS" },
    { RGB565(70, 52, 96),  RGB565(48, 34, 70), RGB565(98, 68, 132), RGB565(140, 104, 180), RGB565(36, 24, 52), RGB565(24, 16, 36), "THE VOID" },
};

const char *theme_name(int theme) { return pal[((theme % THEME_COUNT) + THEME_COUNT) % THEME_COUNT].name; }

static uint16_t scale_color(uint16_t c, int num)
{
    int r = (c >> 11) * num / 256, g = ((c >> 5) & 63) * num / 256, b = (c & 31) * num / 256;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static void fill(uint16_t *px, int x, int y, int w, int h, uint16_t c)
{
    int i, j;
    for (j = y; j < y + h && j < T; j++)
        for (i = x; i < x + w && i < T; i++)
            px[j * T + i] = c;
}

static void draw_floor(uint16_t *px, const Palette *p, int variant)
{
    Rng r;
    int i;
    rng_seed(&r, 77u + (uint32_t)variant * 131u);
    fill(px, 0, 0, T, T, p->floor);
    /* Flagstone joints: two slabs per tile, offset per variant. */
    fill(px, 0, 15, T, 1, p->floor_dk);
    fill(px, variant == 1 ? 9 : 7, 0, 1, 15, p->floor_dk);
    fill(px, 0, 7, variant == 2 ? 7 : T, 1, p->floor_dk);
    for (i = 0; i < 6 + variant * 3; i++)   /* grit */
        px[rng_range(&r, 0, T * T - 1)] = i & 1 ? p->floor_dk : p->brick;
    if (variant == 2) {                      /* a crack */
        fill(px, 3, 3, 1, 2, p->mortar);
        fill(px, 4, 4, 2, 1, p->mortar);
        fill(px, 5, 5, 1, 2, p->mortar);
    }
}

static void draw_wall_front(uint16_t *px, const Palette *p)
{
    int row;
    fill(px, 0, 0, T, T, p->mortar);
    for (row = 0; row < 4; row++) {
        int y = row * 4, off = (row & 1) ? 4 : 0, x;
        for (x = -off; x < T; x += 8) {
            fill(px, x < 0 ? 0 : x, y, x < 0 ? 7 + x : 7, 3, p->brick);
            fill(px, x < 0 ? 0 : x, y, x < 0 ? 7 + x : 7, 1, p->brick_hi);
        }
    }
    fill(px, 0, 0, T, 2, p->brick_hi); /* lit top edge sells the height */
}

static void draw_wall_top(uint16_t *px, const Palette *p)
{
    fill(px, 0, 0, T, T, p->cap);
    fill(px, 0, T - 2, T, 2, p->mortar);
}

static void draw_stairs(uint16_t *px, const Palette *p)
{
    int i;
    fill(px, 0, 0, T, T, RGB565(6, 4, 8));
    for (i = 0; i < 4; i++) {
        fill(px, 1 + i, 2 + i * 3, T - 2 - i * 2, 2, i == 0 ? p->brick_hi : p->brick);
        fill(px, 1 + i, 4 + i * 3, T - 2 - i * 2, 1, p->mortar);
    }
}

void tiles_build(int theme)
{
    const Palette *p;
    static const int level[SHADES] = { 256, 190, 120, 64 };
    int t, s, i;
    theme = ((theme % THEME_COUNT) + THEME_COUNT) % THEME_COUNT;
    if (theme == g_theme)
        return;
    g_theme = theme;
    p = &pal[theme];
    draw_floor(g_tiles[TL_FLOOR0][0], p, 0);
    draw_floor(g_tiles[TL_FLOOR1][0], p, 1);
    draw_floor(g_tiles[TL_FLOOR2][0], p, 2);
    draw_wall_top(g_tiles[TL_WALL_TOP][0], p);
    draw_wall_front(g_tiles[TL_WALL_FRONT][0], p);
    draw_stairs(g_tiles[TL_STAIRS][0], p);
    for (t = 0; t < TL_COUNT; t++)
        for (s = 1; s < SHADES; s++)
            for (i = 0; i < T * T; i++)
                g_tiles[t][s][i] = scale_color(g_tiles[t][0][i], level[s]);
}

const uint16_t *tile_px(int tile, int shade) { return g_tiles[tile][CLAMP(shade, 0, SHADES - 1)]; }
