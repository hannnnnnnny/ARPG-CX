/*
 * sfx_win32.c - the combat sounds (see sfx_win32.h). Hits are a soft
 * wooden tock and crits a brighter one; a kill is a round pop whose pitch
 * climbs do-re-mi-sol-la-do while kills keep coming, so clearing a pack
 * plays a little tune that never clashes with the music; an elite adds two
 * rising chimes, a boss a warm swell under a music-box run.
 */
#include "sfx_win32.h"
#include "../../src/core/sound.h"
#include <math.h>
#include <stdlib.h>

#define RATE 22050
#define TAU 6.2831853f
#define ATTACK 44                    /* 2 ms: no click at the start */

static const float pentatonic[KILL_STREAK_NOTES] = { 1.0f, 1.125f, 1.25f, 1.5f, 1.6667f, 2.0f };

/* A sine gliding from f0 to f1 (time constant glide_s), fading with decay_s. */
static void tone(float *o, int len, int at, float f0, float f1, float glide_s, float decay_s, float vol)
{
    int i;
    float ph = 0;
    for (i = 0; at + i < len; i++) {
        float t = (float)i / RATE, f = f1 + (f0 - f1) * expf(-t / glide_s);
        ph += f / RATE;
        o[at + i] += sinf(TAU * ph) * expf(-t / decay_s) * (i < ATTACK ? (float)i / ATTACK : 1.0f) * vol;
    }
}

/* A music-box note: two-operator FM on the octave, soft and round. */
static void chime(float *o, int len, int at, float f, float decay_s, float vol)
{
    int i;
    for (i = 0; at + i < len; i++) {
        float t = (float)i / RATE, e = expf(-t / decay_s);
        o[at + i] += sinf(TAU * f * t + .6f * e * sinf(TAU * f * 2.0f * t)) * e
                   * (i < ATTACK ? (float)i / ATTACK : 1.0f) * vol;
    }
}

static void kill(float *o, int len, int note)
{
    float f = 587.33f * pentatonic[CLAMP(note, 0, KILL_STREAK_NOTES - 1)];
    tone(o, len, 0, f * 1.8f, f, .012f, .07f, .45f);              /* the round pop */
    tone(o, len, 0, 160, 90, .02f, .05f, .35f);                   /* a little weight under it */
    chime(o, len, (int)(RATE * .015f), f * 2, .09f, .10f);        /* a glint on top */
}

static void boss(float *o, int len)
{
    int k;
    tone(o, len, 0, 146.83f, 146.83f, 1, .9f, .35f);              /* a warm swell */
    tone(o, len, 0, 220.0f, 220.0f, 1, .9f, .20f);
    for (k = 0; k < KILL_STREAK_NOTES; k++)                       /* the music box runs up */
        chime(o, len, (int)(RATE * (.05f + .08f * k)), 587.33f * pentatonic[k], .7f, .28f);
    chime(o, len, (int)(RATE * .62f), 1468.3f, 1.2f, .30f);
}

static float seconds(int id)
{
    switch (id) {
    case SND_HIT:       return .12f;
    case SND_CRIT:      return .4f;
    case SND_KILL:      return .5f;
    case SND_ELITE_DIE: return 2.6f;
    case SND_BOSS_DIE:  return 6.5f;
    default:            return 0;
    }
}

int sfx_render(int id, int note, float **out)
{
    int len = (int)(seconds(id) * RATE);
    float *o;
    *out = NULL;
    if (len <= 0 || !(o = (float *)calloc((size_t)len, sizeof *o)))
        return 0;
    switch (id) {
    case SND_HIT:                                                 /* soft: it plays all the time */
        tone(o, len, 0, 620, 420, .006f, .022f, .30f);
        tone(o, len, 0, 1240, 1240, 1, .010f, .08f);
        break;
    case SND_CRIT:
        tone(o, len, 0, 980, 1180, .01f, .05f, .32f);
        chime(o, len, 0, 1760, .06f, .14f);
        break;
    case SND_KILL:
        kill(o, len, note);
        break;
    case SND_ELITE_DIE:                                           /* two rising chimes: a reward */
        kill(o, len, 0);
        chime(o, len, (int)(RATE * .06f), 1174.66f, .35f, .30f);
        chime(o, len, (int)(RATE * .14f), 1479.98f, .45f, .30f);
        break;
    default:
        boss(o, len);
        break;
    }
    *out = o;
    return len;
}
