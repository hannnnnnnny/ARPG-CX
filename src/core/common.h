/*
 * common.h - shared constants and small helpers for Ashen Depths.
 *
 * Everything in src/ is platform independent C99. Platform code lives in
 * platform/<target>/ and talks to the game only through platform.h.
 */
#ifndef AD_COMMON_H
#define AD_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* The TI-Nspire CX LCD is 320x240 RGB565. The whole game is laid out for it. */
#define SCREEN_W 320
#define SCREEN_H 240

/* Dungeon tiles are 16x16 pixels. */
#define TILE_SIZE  16
#define TILE_SHIFT 4

/* Fixed logic rate. 30 Hz is plenty for an auto-battler and saves battery. */
#define TICK_HZ 30

/* Note: these macros evaluate arguments more than once - never pass
 * expressions with side effects (e.g. rng calls). */
#define ARRAY_LEN(a) ((int)(sizeof(a) / sizeof((a)[0])))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))
#define ABS(a) ((a) < 0 ? -(a) : (a))
#define SIGN(a) ((a) > 0 ? 1 : ((a) < 0 ? -1 : 0))

#endif
