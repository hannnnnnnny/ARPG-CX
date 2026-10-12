/*
 * music_win32.c - background music for the Windows build, synthesized
 * while it plays: no audio files, nothing borrowed.
 *
 * Every track is a few rules - a key and mode, a tempo, timbres and a
 * drum pattern - played by three parts: a low pad on a four-chord loop,
 * a sparse lead whose motif repeats every four bars (so it reads as a
 * tune, not noise) and drums. Each depth theme has its own track, bosses
 * and the butcher bring a driving one, and tracks crossfade over two
 * seconds. Runs on the mixer thread; the game only sets the wanted track.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include "music_win32.h"
#include "../../src/core/sound.h"

#define RATE 22050
#define FADE (RATE * 2)              /* crossfade length in samples */
#define STEPS 16                     /* sixteenths per bar */

typedef enum { MW_SINE, MW_TRI, MW_SAW, MW_SQUARE } MWave;

typedef struct {
    float root;                      /* tonic in Hz */
    int8_t scale[7];                 /* semitones above the tonic */
    uint8_t bpm, lead, pad;          /* tempo, MWave of lead and pad */
    uint8_t drums;                   /* 0 none, 1 heartbeat, 2 forge hammer, 3 battle */
    float lead_oct, lead_vol, density;
} Track;

static const Track tracks[MUS_COUNT] = {
    [MUS_TITLE]     = { 146.83f, { 0, 2, 3, 5, 7, 8, 10 }, 60, MW_TRI, MW_SINE, 0, 2, .55f, .22f },
    [MUS_CRYPT]     = { 146.83f, { 0, 2, 3, 5, 7, 8, 10 }, 72, MW_TRI, MW_SINE, 1, 2, .50f, .28f },
    [MUS_CATACOMBS] = { 164.81f, { 0, 1, 3, 5, 7, 8, 10 }, 66, MW_TRI, MW_TRI, 1, 2, .45f, .25f },
    [MUS_HELLFORGE] = { 138.59f, { 0, 1, 4, 5, 7, 8, 10 }, 84, MW_SAW, MW_SAW, 2, 2, .30f, .32f },
    [MUS_FROZEN]    = { 185.00f, { 0, 2, 3, 5, 7, 9, 10 }, 60, MW_SINE, MW_SINE, 0, 4, .40f, .22f },
    [MUS_VOID]      = { 123.47f, { 0, 1, 3, 5, 6, 8, 10 }, 54, MW_SINE, MW_TRI, 1, 2, .50f, .20f },
    [MUS_BOSS]      = { 110.00f, { 0, 2, 3, 5, 7, 8, 11 }, 132, MW_SQUARE, MW_SAW, 3, 2, .26f, .45f },
};

/* i - VI - iv - v, two bars each */
static const int8_t chords[4] = { 0, 5, 3, 4 };

typedef struct {
    int track;
    uint32_t t;                       /* samples since the track started */
    float pad_ph[3], lead_ph, lead_f, lead_env, kick_env, kick_ph, snare_env, hat_env;
    float lp_lead, lp_pad;
} Player;

static Player g_cur, g_next;
static int g_fade;                    /* samples into a crossfade, 0 = none */
static volatile LONG g_want = MUS_NONE, g_vol = 2;
static uint32_t g_rng = 0x9E3779B9u;

static float wave(int w, float p)
{
    p -= floorf(p);
    switch (w) {
    case MW_TRI:    return p < .5f ? 4 * p - 1 : 3 - 4 * p;
    case MW_SAW:    return 2 * p - 1;
    case MW_SQUARE: return p < .5f ? .7f : -.7f;
    default:        return sinf(6.2831853f * p);
    }
}

static float noise(void)
{
    g_rng = g_rng * 1664525u + 1013904223u;
    return (float)(int32_t)g_rng / 2147483648.0f;
}

static uint32_t hash3(int a, int b, int c)
{
    uint32_t h = (uint32_t)a * 2654435761u ^ (uint32_t)b * 40503u ^ (uint32_t)c * 97u;
    h ^= h >> 15;
    return h * 2246822519u ^ (h >> 13);
}

static float degree_hz(const Track *k, int deg, float oct)
{
    int o = deg >= 0 ? deg / 7 : (deg - 6) / 7, d = deg - o * 7;
    return k->root * oct * powf(2.0f, (float)(k->scale[d] + 12 * o) / 12.0f);
}

/* A new sixteenth: maybe a lead note (the motif repeats every 4 bars,
 * varied every other phrase), and the drum hits of the pattern. */
static void on_step(Player *pl, const Track *k, int bar, int step)
{
    uint32_t h = hash3(pl->track, bar % 4 + (bar / 8 % 2) * 4, step);
    float kick = 0;
    if ((h & 1023) < (uint32_t)(k->density * 1024) && (step & 1) == 0) {
        int deg = chords[bar / 2 % 4] + (int)(h >> 10) % 5 - 1;
        pl->lead_f = degree_hz(k, deg, k->lead_oct);
        pl->lead_env = 1.0f;
    }
    switch (k->drums) {
    case 1: kick = step == 0 ? 1.0f : step == 3 ? .6f : 0; break;           /* a heartbeat */
    case 2: if (step % 8 == 0) kick = 1.0f, pl->snare_env = .5f; break;     /* the forge's hammer */
    case 3:                                                                  /* battle */
        kick = step % 4 == 0 ? 1.0f : 0;
        if (step % 8 == 4) pl->snare_env = 1.0f;
        if (step % 2 == 0) pl->hat_env = .5f;
        break;
    default: break;
    }
    if (kick > 0) {
        pl->kick_env = kick;
        pl->kick_ph = 0;
    }
}

static float drums(Player *pl)
{
    float out = 0, kf = 50.0f + 90.0f * pl->kick_env * pl->kick_env;
    pl->kick_ph += kf / RATE;
    out += sinf(6.2831853f * pl->kick_ph) * pl->kick_env * .9f;
    out += noise() * pl->snare_env * .35f + noise() * pl->hat_env * .12f;
    pl->kick_env *= .99935f;
    pl->snare_env *= .9993f;
    pl->hat_env *= .997f;
    return out;
}

static float pad(Player *pl, const Track *k, int bar)
{
    static const int8_t voicing[3] = { 0, 2, 4 };             /* root, third, fifth */
    float out = 0, lfo = .75f + .25f * sinf(6.2831853f * (float)pl->t / (RATE * 4.0f));
    int v, chord = chords[bar / 2 % 4];
    for (v = 0; v < 3; v++) {
        pl->pad_ph[v] += degree_hz(k, chord + voicing[v], .5f) * (1.0f + v * .002f) / RATE;
        out += wave(k->pad, pl->pad_ph[v]);
    }
    pl->lp_pad += .03f * (out * lfo / 3 - pl->lp_pad);         /* soften the edges */
    return pl->lp_pad;
}

static float sample(Player *pl)
{
    const Track *k = &tracks[pl->track];
    uint32_t step_len = (uint32_t)(RATE * 15 / k->bpm);      /* a sixteenth */
    uint32_t step_no = pl->t / step_len;
    int bar = (int)(step_no / STEPS);
    float lead;
    if (pl->t % step_len == 0)
        on_step(pl, k, bar, (int)(step_no % STEPS));
    pl->lead_ph += pl->lead_f / RATE;
    lead = wave(k->lead, pl->lead_ph) * pl->lead_env * k->lead_vol;
    pl->lp_lead += .2f * (lead - pl->lp_lead);
    pl->lead_env *= .99975f;
    pl->t++;
    return pad(pl, k, bar) * .55f + pl->lp_lead + drums(pl) * .6f;
}

static void start(Player *pl, int track)
{
    ZeroMemory(pl, sizeof *pl);
    pl->track = track;
}

/* Called by the mixer for each buffer: adds the music into 'acc'. */
void music_render(float *acc, int n)
{
    static const float master[SOUND_VOLUMES] = { 0.0f, 0.06f, 0.11f, 0.17f };
    int want = (int)g_want, i;
    float gain = master[CLAMP((int)g_vol, 0, SOUND_VOLUMES - 1)] * 32767.0f;
    if (!g_fade && want != g_cur.track) {          /* a change mid-fade waits for the fade */
        start(&g_next, want);
        g_fade = 1;
    }
    for (i = 0; i < n; i++) {
        float a = g_cur.track != MUS_NONE ? sample(&g_cur) : 0.0f, mix = a;
        if (g_fade) {
            float b = g_next.track != MUS_NONE ? sample(&g_next) : 0.0f, f = (float)g_fade / FADE;
            mix = a * (1 - f) + b * f;
            if (++g_fade >= FADE) {
                g_cur = g_next;
                g_fade = 0;
            }
        }
        acc[i] += mix * gain;
    }
}

void music_set(int track, int volume)
{
    InterlockedExchange(&g_want, CLAMP(track, MUS_NONE, MUS_COUNT - 1));
    InterlockedExchange(&g_vol, volume);
}
