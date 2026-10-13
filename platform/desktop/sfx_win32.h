/* sfx_win32.h - every sound effect: soft, pitched only (never hiss), all
 * on one pentatonic scale, kills climbing it while a streak lasts. */
#ifndef AD_SFX_WIN32_H
#define AD_SFX_WIN32_H

#include "../../src/core/common.h"

#define KILL_STREAK_NOTES 6           /* do re mi sol la do */
#define KILL_STREAK_GAP_MS 600        /* kills closer than this climb */

/* Renders one sound into a new float buffer (22050 Hz, about -1..1).
 * id: SoundId; note: kill streak step (SND_KILL only). Returns the length,
 * 0 on failure (the recipe plays instead). The caller frees. */
int sfx_render(int id, int note, float **out);

#endif
