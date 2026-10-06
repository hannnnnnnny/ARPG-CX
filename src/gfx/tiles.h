/*
 * tiles.h - procedural dungeon tiles in 5 depth themes, each pre-shaded
 * at 4 light levels. Torchlight is then free at draw time: pick the shade
 * by distance from the hero instead of darkening pixels every frame.
 */
#ifndef AD_TILES_H
#define AD_TILES_H

#include "gfx.h"

enum { TL_FLOOR0, TL_FLOOR1, TL_FLOOR2, TL_WALL_TOP, TL_WALL_FRONT, TL_STAIRS, TL_COUNT };
#define SHADES 4
#define THEME_COUNT 5

void tiles_build(int theme);
const uint16_t *tile_px(int tile, int shade);
const char *theme_name(int theme);

#endif
