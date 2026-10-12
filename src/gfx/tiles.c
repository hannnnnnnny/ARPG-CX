#include "tiles.h"
#include "../core/rng.h"
#include <string.h>

#define T 16

static uint16_t g_tiles[TL_COUNT][SHADES][T * T];
static uint8_t g_glow[TL_COUNT][T * T];   /* 1 = emissive: stays lit in the dark */
static int g_theme = -1;

/* Ramps run deep -> light. 'accent' is the theme's clutter (bone, ash, ice),
 * 'glow' its light source (candle, lava, rift). */
typedef struct {
    uint16_t floor[4], face[4], top[2], mortar, accent[2], glow;
    const char *name;
} Palette;

#define C(r, g, b) RGB565(r, g, b)
static const Palette pal[THEME_COUNT] = {
    { { C(30, 26, 34), C(50, 46, 54), C(64, 60, 68), C(78, 74, 82) },
      { C(58, 52, 58), C(84, 78, 82), C(104, 98, 100), C(138, 130, 128) },
      { C(16, 12, 18), C(26, 22, 28) }, C(18, 14, 20), { C(214, 200, 166), C(150, 134, 104) },
      C(255, 170, 60), "THE CRYPT" },
    { { C(40, 30, 24), C(62, 48, 36), C(80, 62, 46), C(98, 78, 58) },
      { C(66, 50, 38), C(98, 76, 56), C(124, 98, 72), C(160, 130, 96) },
      { C(20, 14, 10), C(32, 24, 18) }, C(24, 16, 12), { C(206, 186, 140), C(140, 118, 84) },
      C(255, 170, 60), "CATACOMBS" },
    { { C(34, 18, 16), C(54, 28, 24), C(72, 38, 32), C(90, 50, 40) },
      { C(54, 30, 26), C(84, 44, 36), C(112, 58, 44), C(150, 80, 56) },
      { C(18, 8, 8), C(30, 14, 12) }, C(16, 6, 6), { C(96, 88, 84), C(62, 56, 54) },
      C(255, 128, 32), "HELLFORGE" },
    { { C(30, 40, 56), C(46, 58, 78), C(62, 78, 100), C(80, 98, 122) },
      { C(50, 64, 86), C(78, 98, 126), C(104, 130, 162), C(150, 180, 210) },
      { C(14, 20, 30), C(24, 32, 46) }, C(18, 24, 36), { C(190, 230, 244), C(110, 170, 200) },
      C(160, 226, 255), "FROZEN HALLS" },
    { { C(30, 22, 44), C(46, 34, 66), C(62, 46, 86), C(78, 60, 106) },
      { C(50, 36, 72), C(76, 56, 104), C(98, 72, 134), C(136, 104, 176) },
      { C(14, 10, 22), C(24, 18, 34) }, C(16, 10, 24), { C(104, 94, 124), C(70, 60, 90) },
      C(206, 116, 255), "THE VOID" },
};
#undef C

const char *theme_name(int theme) { return pal[((theme % THEME_COUNT) + THEME_COUNT) % THEME_COUNT].name; }

static uint16_t scale_color(uint16_t c, int num)
{
    int r = (c >> 11) * num / 256, g = ((c >> 5) & 63) * num / 256, b = (c & 31) * num / 256;
    return (uint16_t)((r << 11) | (g << 5) | b);
}

static void fill(uint16_t *px, int x, int y, int w, int h, uint16_t c)
{
    int i, j;
    for (j = MAX(y, 0); j < y + h && j < T; j++)
        for (i = MAX(x, 0); i < x + w && i < T; i++)
            px[j * T + i] = c;
}

/* Points as (x, y) byte pairs, ending at 0xFF. */
static void plot(uint16_t *px, const uint8_t *pts, uint16_t c, uint8_t *glow)
{
    for (; pts[0] != 0xFF; pts += 2) {
        px[pts[1] * T + pts[0]] = c;
        if (glow)
            glow[pts[1] * T + pts[0]] = 1;
    }
}

/* ------------------------------------------------------------ floors */

/* Large flagstones with soft, low-contrast joints, so the floor never reads
 * as brickwork (that belongs to the walls). Each slab has a lit top-left
 * bevel. Variant 1 has no side joint, so slabs run across two tiles. */
static void draw_floor(uint16_t *px, const Palette *p, int variant)
{
    static const uint8_t crack[] = { 4, 10, 5, 11, 5, 12, 6, 13, 7, 13, 8, 14, 0xFF };
    Rng r;
    int i;
    rng_seed(&r, 77u + (uint32_t)variant * 131u);
    fill(px, 0, 0, T, T, p->floor[2]);
    for (i = 0; i < 10; i++)
        px[rng_range(&r, 0, T * T - 1)] = p->floor[1];
    for (i = 0; i < 4; i++)
        px[rng_range(&r, 0, T * T - 1)] = p->floor[3];
    fill(px, 0, 0, T, 1, p->floor[1]);
    fill(px, 1, 1, T - 1, 1, p->floor[3]);
    if (variant != 1) {
        fill(px, 0, 0, 1, T, p->floor[1]);
        fill(px, 1, 1, 1, T - 1, p->floor[3]);
    }
    if (variant == 2)
        plot(px, crack, p->mortar, NULL);
}

static void deco_bones(uint16_t *px, const Palette *p)
{
    static const uint8_t lit[] = { 9, 10, 3, 4, 4, 4, 0xFF };
    static const uint8_t bone[] = { 10, 11, 11, 11, 12, 11, 3, 5, 12, 12, 0xFF };
    static const uint8_t sh[] = { 9, 11, 13, 12, 4, 5, 0xFF };
    plot(px, bone, p->accent[0], NULL);
    plot(px, lit, p->accent[0], NULL);
    plot(px, sh, p->accent[1], NULL);
}

static void deco_skulls(uint16_t *px, const Palette *p)
{
    static const uint8_t skull[] = { 5, 8, 6, 8, 7, 8, 4, 9, 5, 9, 6, 9, 7, 9, 8, 9, 4, 10, 6, 10, 8, 10,
                                     5, 11, 6, 11, 7, 11, 10, 11, 11, 11, 10, 12, 11, 12, 12, 12, 0xFF };
    static const uint8_t eyes[] = { 5, 10, 7, 10, 0xFF };
    plot(px, skull, p->accent[0], NULL);
    plot(px, eyes, p->mortar, NULL);
    fill(px, 4, 12, 4, 1, p->accent[1]);
}

static void deco_blood(uint16_t *px, const Palette *p)
{
    static const uint8_t dark[] = { 7, 9, 8, 9, 7, 10, 9, 10, 10, 11, 6, 11, 11, 12, 3, 5, 0xFF };
    (void)p;
    plot(px, dark, RGB565(86, 16, 22), NULL);
    px[10 * T + 8] = RGB565(124, 26, 30);
}

/* Small stones, each with a lit top-left and a shadow. */
static void deco_rubble(uint16_t *px, const Palette *p)
{
    static const uint8_t stone[] = { 4, 6, 5, 6, 4, 7, 10, 10, 11, 10, 12, 4, 0xFF };
    static const uint8_t sh[] = { 5, 7, 6, 7, 11, 11, 12, 11, 13, 5, 0xFF };
    plot(px, stone, p->accent[0], NULL);
    plot(px, sh, p->floor[0], NULL);
    px[6 * T + 4] = p->face[3];
}

/* A glowing fissure (lava, rift): emissive core, dark burnt rim. */
static void deco_fissure(uint16_t *px, const Palette *p, uint8_t *glow)
{
    static const uint8_t core[] = { 2, 5, 3, 6, 4, 6, 5, 7, 6, 8, 7, 8, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 0xFF };
    static const uint8_t rim[] = { 2, 6, 3, 7, 4, 7, 5, 8, 6, 9, 7, 9, 8, 10, 9, 11, 10, 11, 11, 12, 0xFF };
    plot(px, rim, p->floor[0], NULL);
    plot(px, core, p->glow, glow);
}

static void deco_ice(uint16_t *px, const Palette *p)
{
    static const uint8_t lit[] = { 5, 7, 5, 8, 11, 10, 11, 11, 8, 4, 0xFF };
    static const uint8_t mid[] = { 6, 8, 5, 9, 6, 9, 12, 11, 11, 12, 12, 12, 9, 5, 8, 5, 0xFF };
    static const uint8_t sh[] = { 6, 10, 7, 10, 13, 12, 9, 6, 0xFF };
    plot(px, mid, p->accent[1], NULL);
    plot(px, lit, p->accent[0], NULL);
    plot(px, sh, p->floor[0], NULL);
}

static void draw_deco(uint16_t *px, const Palette *p, int theme, bool second, uint8_t *glow)
{
    draw_floor(px, p, second ? 1 : 0);
    switch (theme * 2 + second) {
    case 0: deco_bones(px, p); break;            /* crypt */
    case 1: deco_blood(px, p); break;
    case 2: deco_skulls(px, p); break;           /* catacombs */
    case 3: deco_bones(px, p); break;
    case 4: deco_fissure(px, p, glow); break;    /* hellforge: lava */
    case 5: deco_rubble(px, p); break;
    case 6: deco_ice(px, p); break;              /* frozen halls */
    case 7: deco_ice(px, p); deco_rubble(px, p); break;
    case 8: deco_fissure(px, p, glow); break;    /* the void: rift */
    default: deco_rubble(px, p); break;
    }
}

/* Contact shadow under a wall face: the top rows fall off toward the wall. */
static void draw_floor_shadow(uint16_t *px, const Palette *p)
{
    static const int k[3] = { 112, 160, 208 };
    int x, y;
    draw_floor(px, p, 0);
    for (y = 0; y < 3; y++)
        for (x = 0; x < T; x++)
            px[y * T + x] = scale_color(px[y * T + x], k[y]);
}

/* ------------------------------------------------------------- walls */

/* Three courses of stone blocks of uneven width, lit along their tops. */
static void draw_blocks(uint16_t *px, const Palette *p, Rng *r)
{
    static const int y0s[3] = { 2, 7, 11 }, hs[3] = { 4, 3, 4 };
    int row, x, y;
    for (row = 0; row < 3; row++) {
        for (x = -rng_range(r, 0, 5); x < T; ) {
            int w = rng_range(r, 5, 8);
            fill(px, x, y0s[row], w - 1, 1, p->face[3]);
            fill(px, x, y0s[row] + 1, w - 1, hs[row] - 1, p->face[2]);
            fill(px, x, y0s[row] + hs[row] - 1, w - 1, 1, p->face[1]);
            if (x >= 0)
                px[y0s[row] * T + x] = p->face[3];
            x += w;
        }
        for (y = 0; y < 3; y++)                      /* chips and pits */
            px[(y0s[row] + 1 + y % hs[row]) * T + rng_range(r, 0, T - 1)] = p->face[1];
    }
}

static void draw_wall_front(uint16_t *px, const Palette *p, int seed)
{
    Rng r;
    int x;
    rng_seed(&r, 911u + (uint32_t)seed * 37u);
    fill(px, 0, 0, T, T, p->mortar);
    fill(px, 0, 0, T, 1, p->face[3]);                /* lit top edge sells the height */
    fill(px, 0, 1, T, 1, p->face[2]);
    draw_blocks(px, p, &r);
    for (x = 0; x < T; x++) {                       /* grime gathers at the foot */
        px[15 * T + x] = p->face[0];
        if (rng_range(&r, 0, 1))
            px[14 * T + x] = p->face[0];
    }
}

/* A dark alcove holding the theme's relic. */
static void feature_niche(uint16_t *px, const Palette *p, int skulls)
{
    static const uint8_t skull[] = { 7, 8, 8, 8, 6, 9, 7, 9, 8, 9, 9, 9, 6, 10, 9, 10, 7, 11, 8, 11, 0xFF };
    static const uint8_t eyes[] = { 7, 10, 8, 10, 0xFF };
    fill(px, 4, 5, 8, 8, p->face[3]);
    fill(px, 5, 6, 6, 7, p->top[0]);
    plot(px, skull, p->accent[0], NULL);
    plot(px, eyes, p->top[0], NULL);
    if (skulls > 1) {
        fill(px, 5, 12, 2, 1, p->accent[1]);
        fill(px, 9, 12, 2, 1, p->accent[1]);
    }
}

static void feature_icicles(uint16_t *px, const Palette *p)
{
    static const uint8_t lit[] = { 2, 2, 2, 3, 2, 4, 7, 2, 7, 3, 7, 4, 7, 5, 7, 6, 12, 2, 12, 3, 0xFF };
    static const uint8_t sh[] = { 3, 2, 3, 3, 8, 2, 8, 3, 8, 4, 13, 2, 0xFF };
    plot(px, lit, p->accent[0], NULL);
    plot(px, sh, p->accent[1], NULL);
}

/* A glowing seam or rune cut into the stone. */
static void feature_glow(uint16_t *px, const Palette *p, bool rune, uint8_t *glow)
{
    static const uint8_t seam[] = { 9, 2, 9, 3, 8, 4, 8, 5, 7, 6, 7, 7, 8, 8, 8, 9, 7, 10, 6, 11, 6, 12, 7, 13, 0xFF };
    static const uint8_t runes[] = { 7, 5, 8, 5, 6, 6, 9, 6, 6, 7, 9, 7, 7, 8, 8, 8, 7, 9, 8, 10, 7, 11, 8, 11, 0xFF };
    plot(px, rune ? runes : seam, p->glow, glow);
}

static void draw_feature_wall(uint16_t *px, const Palette *p, int theme, uint8_t *glow)
{
    draw_wall_front(px, p, 5);
    switch (theme) {
    case 0: feature_niche(px, p, 1); break;
    case 1: feature_niche(px, p, 3); break;
    case 2: feature_glow(px, p, false, glow); break;
    case 3: feature_icicles(px, p); break;
    default: feature_glow(px, p, true, glow); break;
    }
}

static void draw_wall_top(uint16_t *px, const Palette *p)
{
    Rng r;
    int i;
    rng_seed(&r, 4242u);
    fill(px, 0, 0, T, T, p->top[0]);
    for (i = 0; i < 26; i++)
        px[rng_range(&r, 0, T * T - 1)] = p->top[1];
}

static void draw_stairs(uint16_t *px, const Palette *p)
{
    int i;
    fill(px, 0, 0, T, T, RGB565(6, 4, 8));
    for (i = 0; i < 4; i++) {
        fill(px, 1 + i, 2 + i * 3, T - 2 - i * 2, 1, i == 0 ? p->face[3] : p->face[2]);
        fill(px, 1 + i, 3 + i * 3, T - 2 - i * 2, 1, p->face[1]);
        fill(px, 1 + i, 4 + i * 3, T - 2 - i * 2, 1, p->mortar);
    }
}

/* ------------------------------------------------------------ build */

/* Light falls off in 8 steps; emissive pixels never drop below 'GLOW_MIN'. */
static void shade_all(void)
{
    static const int level[SHADES] = { 256, 226, 198, 172, 146, 122, 98, 72 };
    enum { GLOW_MIN = 236 };
    int t, s, i;
    for (t = 0; t < TL_COUNT; t++)
        for (s = 1; s < SHADES; s++)
            for (i = 0; i < T * T; i++)
                g_tiles[t][s][i] = scale_color(g_tiles[t][0][i], g_glow[t][i] ? MAX(level[s], GLOW_MIN) : level[s]);
}

void tiles_build(int theme)
{
    const Palette *p;
    theme = ((theme % THEME_COUNT) + THEME_COUNT) % THEME_COUNT;
    if (theme == g_theme)
        return;
    g_theme = theme;
    p = &pal[theme];
    memset(g_glow, 0, sizeof g_glow);
    draw_floor(g_tiles[TL_FLOOR0][0], p, 0);
    draw_floor(g_tiles[TL_FLOOR1][0], p, 1);
    draw_floor(g_tiles[TL_FLOOR2][0], p, 2);
    draw_deco(g_tiles[TL_FLOOR_DECO_A][0], p, theme, false, g_glow[TL_FLOOR_DECO_A]);
    draw_deco(g_tiles[TL_FLOOR_DECO_B][0], p, theme, true, g_glow[TL_FLOOR_DECO_B]);
    draw_floor_shadow(g_tiles[TL_FLOOR_SHADOW][0], p);
    draw_wall_top(g_tiles[TL_WALL_TOP][0], p);
    draw_wall_front(g_tiles[TL_WALL_FRONT][0], p, 0);
    draw_feature_wall(g_tiles[TL_WALL_FRONT2][0], p, theme, g_glow[TL_WALL_FRONT2]);
    draw_stairs(g_tiles[TL_STAIRS][0], p);
    shade_all();
}

const uint16_t *tile_px(int tile, int shade) { return g_tiles[CLAMP(tile, 0, TL_COUNT - 1)][CLAMP(shade, 0, SHADES - 1)]; }
