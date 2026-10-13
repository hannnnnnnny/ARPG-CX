/*
 * sfx_win32.c - every sound effect (see sfx_win32.h). Pitched sounds only,
 * so nothing hisses - except skill casts, which sound like their element
 * (thunder, a fire's roar) with noise in short shaped bursts; all notes come
 * from one pentatonic scale (D E F# A B), so sounds that overlap each
 * other or the music never clash. Rising means good (loot, levels),
 * falling means leaving or losing (stairs, hurt). Loot grows with rarity:
 * a tick, then two, three, four notes, and a whole phrase for a mythic.
 * Frequent sounds (hits, clicks, gold, common drops) stay very quiet.
 * Kills climb the scale while a streak lasts.
 */
#include "sfx_win32.h"
#include "../../src/core/sound.h"
#include <math.h>
#include <stdlib.h>

#define RATE 22050
#define TAU 6.2831853f
#define ATTACK 44                    /* 2 ms: no click at the start */

/* Step s of the pentatonic scale from D4 (5 steps an octave). */
static float pent(int s)
{
    static const float ratio[5] = { 1.0f, 1.125f, 1.25f, 1.5f, 1.6667f };
    return 293.66f * powf(2.0f, (float)(s / 5)) * ratio[s % 5];
}

static float attack(int i) { return i < ATTACK ? (float)i / ATTACK : 1.0f; }

/* A sine gliding from f0 to f1 (time constant glide_s), fading with decay_s. */
static void tone(float *o, int len, float at_s, float f0, float f1, float glide_s, float decay_s, float vol)
{
    int i, at = (int)(at_s * RATE);
    float ph = 0;
    for (i = 0; at + i < len; i++) {
        float t = (float)i / RATE, f = f1 + (f0 - f1) * expf(-t / glide_s);
        ph += f / RATE;
        o[at + i] += sinf(TAU * ph) * expf(-t / decay_s) * attack(i) * vol;
    }
}

/* A music-box note: two-operator FM on the octave, soft and round. */
static void chime(float *o, int len, float at_s, float f, float decay_s, float vol)
{
    int i, at = (int)(at_s * RATE);
    for (i = 0; at + i < len; i++) {
        float t = (float)i / RATE, e = expf(-t / decay_s);
        o[at + i] += sinf(TAU * f * t + .6f * e * sinf(TAU * f * 2.0f * t)) * e * attack(i) * vol;
    }
}

/* Chimes on scale steps, 'gap' seconds apart. */
static void run(float *o, int len, float at_s, const int8_t *steps, int n, float gap, float decay_s, float vol)
{
    int k;
    for (k = 0; k < n; k++)
        chime(o, len, at_s + gap * k, pent(steps[k]), decay_s, vol);
}

/* A short pitched blip that slides up into its note (bubbles, clicks). */
static void blip(float *o, int len, float at_s, float f, float decay_s, float vol)
{
    tone(o, len, at_s, f * .8f, f, .008f, decay_s, vol);
}

/* --------------------------------------------------------------- combat */

static void kill(float *o, int len, int note)
{
    float f = pent(5 + CLAMP(note, 0, KILL_STREAK_NOTES - 1));
    tone(o, len, 0, f * 1.8f, f, .012f, .07f, .45f);              /* the round pop */
    tone(o, len, 0, 160, 90, .02f, .05f, .35f);                   /* a little weight under it */
    chime(o, len, .015f, f * 2, .09f, .10f);                      /* a glint on top */
}

static void combat(float *o, int len, int id, int note)
{
    static const int8_t boss_run[6] = { 5, 6, 7, 8, 9, 10 };
    switch (id) {
    case SND_HIT:   tone(o, len, 0, 620, 420, .006f, .022f, .30f); tone(o, len, 0, 1240, 1240, 1, .010f, .08f); break;
    case SND_CRIT:  tone(o, len, 0, 980, 1180, .01f, .05f, .32f); chime(o, len, 0, 1760, .06f, .14f); break;
    case SND_KILL:  kill(o, len, note); break;
    case SND_ELITE_DIE:                                           /* two rising chimes: a reward */
        kill(o, len, 0);
        chime(o, len, .06f, pent(10), .35f, .30f);
        chime(o, len, .14f, pent(12), .45f, .30f);
        break;
    case SND_BOSS_DIE:                                            /* a warm swell under a music-box run */
        tone(o, len, 0, pent(0) / 2, pent(0) / 2, 1, .9f, .35f);
        tone(o, len, 0, pent(3) / 2, pent(3) / 2, 1, .9f, .20f);
        run(o, len, .05f, boss_run, 6, .08f, .7f, .28f);
        chime(o, len, .62f, pent(12), 1.2f, .30f);
        break;
    case SND_HURT:                                                /* a soft bonk, falling */
        tone(o, len, 0, 392, 262, .03f, .08f, .40f);
        tone(o, len, 0, 130, 90, .02f, .06f, .30f);
        break;
    default: {                                                    /* SND_DEATH: the scale walks down */
        static const int8_t down[5] = { 9, 7, 5, 3, 1 };
        tone(o, len, 0, pent(0) / 4, pent(0) / 4, 1, .9f, .30f);
        run(o, len, 0, down, 5, .14f, .6f, .26f);
        break;
    }
    }
}

/* ----------------------------------------------------------------- loot */

static void loot(float *o, int len, int id)
{
    static const int8_t rare[2] = { 8, 10 }, legend[3] = { 7, 8, 10 }, unique[5] = { 5, 7, 8, 10, 12 };
    static const int8_t mythic[6] = { 5, 8, 10, 12, 13, 15 };
    switch (id) {
    case SND_DROP:        blip(o, len, 0, pent(10), .025f, .18f); break;
    case SND_DROP_RARE:   run(o, len, 0, rare, 2, .07f, .25f, .25f); break;
    case SND_DROP_LEGEND:
        tone(o, len, 0, pent(0), pent(0), 1, .4f, .15f);
        run(o, len, 0, legend, 3, .07f, .35f, .28f);
        break;
    case SND_DROP_UNIQUE:
        tone(o, len, 0, pent(0), pent(0), 1, .6f, .15f);
        tone(o, len, 0, pent(3), pent(3), 1, .6f, .10f);
        run(o, len, 0, unique, 5, .08f, .5f, .26f);
        break;
    case SND_DROP_MYTHIC:                                         /* a whole phrase, with a sparkle above */
        tone(o, len, 0, pent(0) / 2, pent(0) / 2, 1, 1.5f, .30f);
        tone(o, len, 0, pent(3) / 2, pent(3) / 2, 1, 1.5f, .20f);
        run(o, len, .05f, mythic, 6, .12f, .9f, .30f);
        run(o, len, .11f, mythic, 6, .12f, .5f, .08f);
        chime(o, len, .85f, pent(20), 1.4f, .12f);
        break;
    default:                                                      /* SND_GOLD: ting-ting, tiny */
        chime(o, len, 0, pent(13), .05f, .12f);
        chime(o, len, .04f, pent(15), .06f, .12f);
        break;
    }
}

/* ---------------------------------------------------------------- casts */

/* Skill casts are the one place noise is allowed - a thunderclap or a
 * roar of fire has no pitch - but only in short, shaped bursts through a
 * filter, never as a hiss that lingers. */
static uint32_t g_seed = 0x2545F491u;

static float noise(void)
{
    g_seed = g_seed * 1664525u + 1013904223u;
    return (float)(int32_t)g_seed / 2147483648.0f;
}

/* Noise through a resonant bandpass whose centre glides f0 -> f1;
 * envelope: rise_s up, then decay_s down. */
static void band(float *o, int len, float at_s, float f0, float f1, float q, float rise_s, float decay_s, float vol)
{
    int i, at = (int)(at_s * RATE), n = (int)((rise_s + decay_s * 4) * RATE);
    float lo = 0, bp = 0;
    for (i = 0; i < n && at + i < len; i++) {
        float t = (float)i / RATE, fc = f1 + (f0 - f1) * expf(-t / (decay_s + rise_s));
        float g = 2.0f * sinf(3.14159f * MIN(fc, RATE / 6.0f) / RATE);
        float env = t < rise_s ? t / rise_s : expf(-(t - rise_s) / decay_s);
        lo += g * bp;
        bp += g * (noise() - lo - q * bp);
        o[at + i] += bp * env * vol;
    }
}

/* Crackles: a few very short bright pops at random moments. */
static void crackle(float *o, int len, float at_s, float span_s, int count, float vol)
{
    int k, i;
    for (k = 0; k < count; k++) {
        int at = (int)((at_s + span_s * (noise() * .5f + .5f)) * RATE);
        for (i = 0; i < RATE / 200 && at + i < len; i++)
            o[at + i] += noise() * expf(-(float)i / (RATE * .0012f)) * vol;
    }
}

/* Lightning: a crack, an electric buzz that jitters, the thunder rolling off. */
static void lightning(float *o, int len)
{
    int i;
    float ph = 0, f = 90;
    band(o, len, 0, 3500, 1800, .3f, .001f, .012f, 1.4f);         /* the crack */
    for (i = 0; i < (int)(.22f * RATE) && i < len; i++) {           /* the buzz */
        float t = (float)i / RATE;
        if (i % 120 == 0)
            f = 70 + 80 * (noise() * .5f + .5f);
        ph += f / RATE;
        o[i] += (2.0f * (ph - floorf(ph)) - 1.0f) * expf(-t / .08f) * .30f;
    }
    crackle(o, len, .0f, .15f, 9, .5f);
    band(o, len, .03f, 300, 80, .5f, .04f, .25f, 1.1f);             /* thunder rolling off */
}

static void fire(float *o, int len)
{
    band(o, len, 0, 600, 250, .6f, .08f, .14f, 1.2f);               /* the fireball's whoosh */
    tone(o, len, 0, 120, 70, .08f, .18f, .30f);                     /* a roar under it */
    crackle(o, len, .05f, .30f, 10, .35f);                          /* sparks */
}

/* Ice: glass shards breaking off, high and bright, and a cold breath. */
static void ice(float *o, int len)
{
    static const float shard[7] = { 2637, 3136, 2349, 3520, 2794, 3951, 2960 };
    int k;
    for (k = 0; k < 7; k++)
        chime(o, len, .012f * k + .004f * (noise() + 1.0f), shard[k], .05f, .12f);
    band(o, len, 0, 5000, 3000, .4f, .005f, .06f, .5f);
}

static void physical(float *o, int len)
{
    int i;
    band(o, len, 0, 2500, 600, .35f, .03f, .06f, 1.1f);             /* the blade's whoosh */
    for (i = 0; i < (int)(.3f * RATE) && i < len; i++) {            /* a steel "shing" */
        float t = (float)i / RATE, e = expf(-t / .07f);
        o[i] += sinf(TAU * 1900 * t + 2.0f * e * sinf(TAU * 1900 * 2.76f * t)) * e * attack(i) * .12f;
    }
}

static void poison(float *o, int len)
{
    int k;
    for (k = 0; k < 7; k++) {                                       /* bubbles rising and popping */
        float f = 300 + 500 * (noise() * .5f + .5f);
        tone(o, len, .035f * k, f * .6f, f * 1.3f, .02f, .025f, .22f);
    }
    tone(o, len, 0, 110, 90, .1f, .2f, .18f);                       /* the gurgle beneath */
}

static void shadow(float *o, int len)
{
    band(o, len, 0, 900, 200, .7f, .15f, .15f, .9f);                /* a dark swell */
    tone(o, len, 0, 196, 110, .12f, .25f, .25f);
    tone(o, len, 0, 199, 112, .12f, .25f, .20f);                    /* two voices beating: unease */
}

static void cast(float *o, int len, int id)
{
    switch (id) {
    case SND_CAST_PHYS:      physical(o, len); break;
    case SND_CAST_FIRE:      fire(o, len); break;
    case SND_CAST_COLD:      ice(o, len); break;
    case SND_CAST_LIGHTNING: lightning(o, len); break;
    case SND_CAST_POISON:    poison(o, len); break;
    default:                 shadow(o, len); break;
    }
}

/* --------------------------------------------------------------- events */

static void event(float *o, int len, int id)
{
    static const int8_t down[4] = { 10, 8, 7, 5 }, jig[6] = { 10, 12, 10, 13, 12, 15 };
    static const int8_t up3[3] = { 7, 8, 10 }, fan[4] = { 5, 7, 8, 10 }, bloom[4] = { 5, 7, 8, 10 };
    static const int8_t chord[3] = { 10, 12, 13 }, alarm[3] = { 5, 7, 10 }, level[5] = { 5, 7, 8, 10, 12 };
    int k;
    switch (id) {
    case SND_LEVEL:   run(o, len, 0, level, 5, .06f, .45f, .26f); break;
    case SND_POTION:  for (k = 0; k < 3; k++) blip(o, len, .05f * k, pent(5 + k), .03f, .28f); break;
    case SND_STAIRS:  run(o, len, 0, down, 4, .09f, .3f, .25f); break;
    case SND_GOBLIN:  for (k = 0; k < 6; k++) blip(o, len, .06f * k, pent(jig[k]), .04f, .20f); break;
    case SND_SHRINE:
        tone(o, len, 0, pent(0), pent(0), 1, .9f, .14f);
        run(o, len, 0, bloom, 4, .02f, .9f, .16f);
        break;
    case SND_CHEST:   tone(o, len, 0, 180, 110, .02f, .06f, .40f); run(o, len, .12f, chord + 1, 2, .05f, .08f, .12f); break;
    case SND_AMBUSH:
        run(o, len, 0, alarm, 3, .05f, .12f, .22f);
        tone(o, len, .18f, 150, 100, .03f, .12f, .45f);
        tone(o, len, .33f, 150, 100, .03f, .12f, .40f);
        break;
    case SND_ACHIEVE:
        tone(o, len, 0, pent(0), pent(0), 1, 1.0f, .15f);
        run(o, len, 0, fan, 4, .08f, .4f, .24f);
        run(o, len, .40f, chord, 3, .0f, 1.0f, .18f);
        break;
    default:          run(o, len, 0, up3, 3, .07f, .3f, .25f); break;   /* SND_BOUNTY */
    }
}

/* ------------------------------------------------------------------- ui */

static void ui(float *o, int len, int id)
{
    switch (id) {
    case SND_UI_MOVE:  tone(o, len, 0, 1200, 1200, 1, .008f, .08f); break;
    case SND_UI_OK:    blip(o, len, 0, pent(8), .03f, .14f); blip(o, len, .04f, pent(10), .03f, .14f); break;
    case SND_UI_BACK:  blip(o, len, 0, pent(10), .03f, .14f); blip(o, len, .04f, pent(8), .03f, .14f); break;
    case SND_UI_ERROR: tone(o, len, 0, 220, 196, .02f, .04f, .20f); tone(o, len, .09f, 220, 196, .02f, .04f, .20f); break;
    default:           tone(o, len, 0, 900, 700, .01f, .015f, .08f); break;   /* SND_PAGE */
    }
}

/* ------------------------------------------------------------- dispatch */

static float seconds(int id)
{
    static const float s[SND_COUNT] = {
        [SND_HIT] = .12f, [SND_CRIT] = .4f, [SND_KILL] = .5f, [SND_ELITE_DIE] = 2.6f, [SND_BOSS_DIE] = 6.5f,
        [SND_HURT] = .5f, [SND_DEATH] = 4.0f, [SND_LEVEL] = 2.6f, [SND_POTION] = .3f, [SND_DROP] = .15f,
        [SND_DROP_RARE] = 1.3f, [SND_DROP_LEGEND] = 2.1f, [SND_DROP_UNIQUE] = 3.0f, [SND_DROP_MYTHIC] = 7.5f,
        [SND_GOLD] = .4f, [SND_CAST_PHYS] = .4f, [SND_CAST_FIRE] = .9f, [SND_CAST_COLD] = .5f,
        [SND_CAST_LIGHTNING] = 1.2f, [SND_CAST_POISON] = .5f, [SND_CAST_SHADOW] = 1.0f, [SND_STAIRS] = 1.8f,
        [SND_GOBLIN] = .6f, [SND_SHRINE] = 4.6f, [SND_CHEST] = .6f, [SND_AMBUSH] = 1.0f, [SND_ACHIEVE] = 5.5f,
        [SND_BOUNTY] = 1.7f, [SND_UI_MOVE] = .05f, [SND_UI_OK] = .2f, [SND_UI_BACK] = .2f, [SND_UI_ERROR] = .3f,
        [SND_PAGE] = .08f,
    };
    return id >= 0 && id < SND_COUNT ? s[id] : 0;
}

/* Each sound's loudest moment, so frequent ones stay quiet, rare events
 * stand out, and nothing clips even at HIGH volume. */
static float peak_for(int id)
{
    switch (id) {
    case SND_UI_MOVE: case SND_PAGE:                     return .10f;
    case SND_UI_OK: case SND_UI_BACK: case SND_UI_ERROR:  return .16f;
    case SND_DROP: case SND_GOLD:                         return .16f;
    case SND_HIT:                                         return .22f;
    case SND_CAST_PHYS: case SND_CAST_FIRE: case SND_CAST_COLD:
    case SND_CAST_POISON: case SND_CAST_SHADOW: case SND_POTION: return .26f;
    case SND_CAST_LIGHTNING:                              return .30f;   /* thunder is allowed to be loud */
    case SND_CRIT: case SND_DROP_RARE:                    return .30f;
    case SND_HURT: case SND_DROP_LEGEND: case SND_BOUNTY: case SND_CHEST: return .36f;
    case SND_KILL: case SND_DROP_UNIQUE: case SND_STAIRS: case SND_GOBLIN: case SND_SHRINE: return .42f;
    default:                                              return .50f;   /* elites, bosses, mythics, levels */
    }
}

static void normalize(float *o, int len, float peak)
{
    float m = 0;
    int i;
    for (i = 0; i < len; i++)
        m = MAX(m, fabsf(o[i]));
    for (i = 0; m > 0 && i < len; i++)
        o[i] *= peak / m;
}

int sfx_render(int id, int note, float **out)
{
    int len = (int)(seconds(id) * RATE);
    float *o;
    *out = NULL;
    if (len <= 0 || !(o = (float *)calloc((size_t)len, sizeof *o)))
        return 0;
    if (id <= SND_DEATH)
        combat(o, len, id, note);
    else if ((id >= SND_DROP && id <= SND_DROP_MYTHIC) || id == SND_GOLD)
        loot(o, len, id);
    else if (id >= SND_CAST_PHYS && id <= SND_CAST_SHADOW)
        cast(o, len, id);
    else if (id >= SND_UI_MOVE)
        ui(o, len, id);
    else
        event(o, len, id);
    normalize(o, len, peak_for(id));
    *out = o;
    return len;
}
