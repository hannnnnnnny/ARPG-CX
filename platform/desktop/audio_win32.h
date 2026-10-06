/* audio_win32.h - synthesized sound effects for the Windows build. */
#ifndef AD_AUDIO_WIN32_H
#define AD_AUDIO_WIN32_H

#include "../../src/core/common.h"

/* False when there is no audio device: the game then simply plays silently. */
bool audio_init(void);
void audio_shutdown(void);
/* id: SoundId (sound.h); volume: 0..SOUND_VOLUMES-1. */
void audio_play(int id, int volume);

#endif
