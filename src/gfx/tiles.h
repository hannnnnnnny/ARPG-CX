/*
 * tiles.h - procedural dungeon tiles in 5 depth themes, each pre-shaded
 * at 8 light levels. Torchlight is then free at draw time: pick the shade
 * by distance from the hero (or a wall torch) instead of darkening pixels
 * every frame. Glowing pixels (lava, rifts) keep their light in the dark.
 */
#ifndef AD_TILES_H
#define AD_TILES_H

#include "gfx.h"

enum {
    TL_FLOOR0, TL_FLOOR1, TL_FLOOR2,      /* plain flagstones, cracked */
    TL_FLOOR_DECO_A, TL_FLOOR_DECO_B,     /* the theme's floor clutter */
    TL_FLOOR_SHADOW,                      /* floor in a wall's contact shadow */
    TL_WALL_TOP, TL_WALL_FRONT,
    TL_WALL_FRONT2,                       /* the theme's feature wall (niche, seam, rune) */
    TL_STAIRS, TL_COUNT
};
#define SHADES 8
#define THEME_COUNT 5

void tiles_build(int theme);
const uint16_t *tile_px(int tile, int shade);
const char *theme_name(int theme);

#endif
