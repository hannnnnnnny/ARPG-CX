/* music_win32.h - synthesized background music (Windows build). */
#ifndef AD_MUSIC_WIN32_H
#define AD_MUSIC_WIN32_H

#include "../../src/core/common.h"

/* Wanted track (MusicId, sound.h) and volume 0..SOUND_VOLUMES-1; any thread. */
void music_set(int track, int volume);
/* Mixer thread: add n samples of music into acc. */
void music_render(float *acc, int n);

#endif
