/*
 * music_win32.c - background music for the Windows build, synthesized
 * while it plays: no audio files, nothing borrowed.
 *
 * Slow, light and cosy: an electric piano strolls through broken
 * chords (sevenths and ninths), a marimba plucks a few notes over it, a
 * soft flute sings a long-note tune whose phrase repeats every four bars,
 * and a warm bass holds the root. Percussion is pitched and soft, never
 * hiss: a woodblock on two and four; fights add a gentle kick. Warm major and
 * lydian keys, a small room reverb. Tracks crossfade over two seconds.
 * Runs on the mixer thread; the game only sets the wanted track.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <math.h>
#include <string.h>
#include "music_win32.h"
#include "../../src/core/sound.h"

#define RATE 22050
#define FADE (RATE * 2)              /* crossfade length in samples */
#define STEPS 16                     /* sixteenths per bar */
#define VOICES 8                     /* piano and marimba notes ringing at once */
#define TAU 6.2831853f

typedef enum { TONE_PIANO, TONE_MARIMBA, TONE_GLASS } ToneKind;

/* FM tones: modulator ratio and index, how fast it fades, a second partial. */
typedef struct { float ratio, index, decay_s, partial, partial_vol; } ToneDef;

static const ToneDef tone_defs[] = {
    [TONE_PIANO]   = { 1.0f, 1.2f, 1.6f, 7.0f, .06f },    /* a soft electric piano with a tine */
    [TONE_MARIMBA] = { 1.0f, 0.3f, 0.45f, 4.0f, .25f },   /* wood: the fourth harmonic, quick */
    [TONE_GLASS]   = { 3.5f, 0.6f, 2.2f, 5.0f, .10f },    /* a music box */
};

typedef struct {
    float root;                      /* tonic in Hz */
    int8_t scale[7];                 /* semitones above the tonic */
    int8_t prog[4];                  /* chord degrees, one bar each */
    uint8_t bpm;
    uint8_t pluck;                   /* ToneKind of the plucked line */
    uint8_t fight;                   /* gentle kick and brush */
    float lead_oct;
} Track;

#define MAJOR  { 0, 2, 4, 5, 7, 9, 11 }
#define LYDIAN { 0, 2, 4, 6, 7, 9, 11 }
#define MIXO   { 0, 2, 4, 5, 7, 9, 10 }
#define DORIAN { 0, 2, 3, 5, 7, 9, 10 }
#define MINOR  { 0, 2, 3, 5, 7, 8, 10 }

static const Track tracks[MUS_COUNT] = {
    [MUS_TITLE]     = { 174.61f, MAJOR,  { 0, 5, 3, 4 }, 76,  TONE_MARIMBA, 0, 2.0f },
    [MUS_CRYPT]     = { 146.83f, DORIAN, { 0, 3, 0, 3 }, 80,  TONE_MARIMBA, 0, 2.0f },
    [MUS_CATACOMBS] = { 146.83f, MIXO,   { 0, 6, 3, 0 }, 72,  TONE_MARIMBA, 0, 2.0f },
    [MUS_HELLFORGE] = { 196.00f, DORIAN, { 0, 3, 6, 4 }, 88,  TONE_MARIMBA, 0, 1.0f },
    [MUS_FROZEN]    = { 164.81f, LYDIAN, { 0, 1, 0, 4 }, 66,  TONE_GLASS,   0, 2.0f },
    [MUS_VOID]      = { 116.54f, LYDIAN, { 0, 1, 5, 4 }, 70,  TONE_GLASS,   0, 4.0f },
    [MUS_BOSS]      = { 220.00f, MINOR,  { 0, 5, 6, 4 }, 112, TONE_MARIMBA, 1, 1.0f },
};

typedef struct { float car, mod, f, amp, decay; uint8_t tone; } Voice;

/* A small warm room: four damped combs into two allpasses. */
#define COMBS 4
#define ALLS 2
static const int comb_len[COMBS] = { 557, 593, 641, 677 };
static const int all_len[ALLS] = { 113, 277 };
typedef struct {
    float comb[COMBS][680], all[ALLS][280], damp[COMBS];
    int ci[COMBS], ai[ALLS];
} Reverb;

typedef struct {
    int track;
    uint32_t t;                      /* samples since the track started */
    Voice voice[VOICES];
    int next_voice;
    int lead_deg, lead_left;
    float lead_ph, lead_f, lead_target, lead_env, lead_gate, lead_age;
    float bass_ph, bass_f, bass_env, bass_amp;
    float tick_env, tick_ph, kick_env, kick_ph;
    Reverb rv;
} Player;

static Player g_cur, g_next;
static int g_fade;                   /* samples into a crossfade, 0 = none */
static volatile LONG g_want = MUS_NONE, g_vol = 2;

static uint32_t hash3(int a, int b, int c)
{
    uint32_t h = (uint32_t)a * 2654435761u ^ (uint32_t)b * 40503u ^ (uint32_t)c * 97u;
    h ^= h >> 15;
    return h * 2246822519u ^ (h >> 13);
}

static float degree_hz(const Track *k, int deg, float octave)
{
    int o = deg >= 0 ? deg / 7 : -((-deg + 6) / 7);
    int d = deg - o * 7;
    return k->root * octave * powf(2.0f, (float)(k->scale[d] + 12 * o) / 12.0f);
}

static float tri(float p) { p -= floorf(p); return p < .5f ? 4 * p - 1 : 3 - 4 * p; }

/* -------------------------------------------------------- piano, marimba */

static void play(Player *pl, const Track *k, int tone, int deg, float octave, float vel)
{
    Voice *v = &pl->voice[pl->next_voice++ % VOICES];
    v->f = degree_hz(k, deg, octave);
    v->amp = vel;
    v->decay = expf(-1.0f / (tone_defs[tone].decay_s * RATE));
    v->tone = (uint8_t)tone;
    v->car = v->mod = 0;
}

static float voices(Player *pl)
{
    float out = 0;
    int i;
    for (i = 0; i < VOICES; i++) {
        Voice *v = &pl->voice[i];
        const ToneDef *d = &tone_defs[v->tone];
        if (v->amp < 1e-4f)
            continue;
        v->car += v->f / RATE;
        v->mod += v->f * d->ratio / RATE;
        out += (sinf(TAU * v->car + d->index * v->amp * sinf(TAU * v->mod))
                + d->partial_vol * v->amp * sinf(TAU * v->car * d->partial)) * v->amp;
        v->amp *= v->decay;
    }
    return out;
}

/* The piano strolls: a low root on the one, the seventh chord's upper
 * notes on the off-beats, a ninth now and then. */
static void piano_step(Player *pl, const Track *k, int bar, int step)
{
    int chord = k->prog[bar % 4];
    uint32_t h = hash3(bar % 4, step, pl->track + 50);
    if (step == 0)
        play(pl, k, TONE_PIANO, chord, .5f, .55f);
    else if (step == 4 || step == 10)
        play(pl, k, TONE_PIANO, chord + 2 + (int)(h % 2) * 2, 1.0f, .32f);
    else if (step == 6 || step == 12)
        play(pl, k, TONE_PIANO, chord + (h % 3 == 0 ? 8 : 6), 1.0f, .26f);
}

/* A few plucks, never a stream: they answer the tune in its gaps. */
static void pluck_step(Player *pl, const Track *k, int bar, int step)
{
    static const int8_t notes[4] = { 4, 7, 9, 11 };
    uint32_t h = hash3(bar % 4 + (bar / 8 % 2) * 4, step, pl->track + 90);
    if (step % 2 || (h & 255) > 70)
        return;
    play(pl, k, k->pluck, k->prog[bar % 4] + notes[(h >> 8) % 4], 2.0f, .35f);
}

/* ---------------------------------------------------------------- lead */

/* The tune: long notes (a quarter to a whole bar), small steps, chord
 * tones at the bar lines, a breath between phrases. */
static void lead_step(Player *pl, const Track *k, int bar, int eighth)
{
    static const int8_t lens[8] = { 2, 4, 2, 6, 3, 2, 8, 4 };
    static const int8_t walk[8] = { 1, -1, 2, -1, 1, -2, 0, 1 };
    uint32_t h = hash3(bar % 4 + (bar / 8 % 2) * 8, eighth, pl->track);
    int chord = k->prog[bar % 4];
    if (--pl->lead_left > 0)
        return;
    if ((bar % 4 == 3 && eighth >= 4) || (h % 5 == 0 && eighth != 0)) {   /* breathe */
        pl->lead_gate = 0;
        pl->lead_left = 2;
        return;
    }
    pl->lead_deg = eighth == 0 ? chord + (int)((h >> 3) % 3) * 2 : pl->lead_deg + walk[(h >> 5) % 8];
    pl->lead_deg = CLAMP(pl->lead_deg, chord - 2, chord + 7);
    pl->lead_left = lens[(h >> 9) % 8];
    pl->lead_target = degree_hz(k, pl->lead_deg, k->lead_oct);
    if (pl->lead_gate < .5f)
        pl->lead_f = pl->lead_target;
    pl->lead_gate = 1;
    pl->lead_age = 0;
}

/* A soft flute: mostly sine with a little triangle, gentle vibrato. No breath noise. */
static float lead(Player *pl)
{
    float vib = 1.0f + .004f * MIN(pl->lead_age * 2.0f, 1.0f) * sinf(TAU * 4.8f * pl->lead_age);
    float x;
    pl->lead_f += .003f * (pl->lead_target - pl->lead_f);
    pl->lead_ph += pl->lead_f * vib / RATE;
    pl->lead_env += (pl->lead_gate - pl->lead_env) * (pl->lead_gate > pl->lead_env ? .0015f : .0008f);
    pl->lead_age += 1.0f / RATE;
    x = sinf(TAU * pl->lead_ph) * .8f + tri(pl->lead_ph) * .2f;
    return x * pl->lead_env;
}

/* ------------------------------------------------------- bass and time */

static void bass_step(Player *pl, const Track *k, int bar, int step)
{
    if (step != 0 && step != 8)
        return;
    pl->bass_f = degree_hz(k, k->prog[bar % 4] + (step == 8 ? 4 : 0), .5f);   /* audible on laptop speakers */
    pl->bass_env = 1.0f;
}

static float bass(Player *pl)
{
    pl->bass_amp += (pl->bass_env - pl->bass_amp) * .004f;   /* a soft swell, no thump */
    pl->bass_ph += pl->bass_f / RATE;
    pl->bass_env *= .99993f;
    return sinf(TAU * pl->bass_ph) * pl->bass_amp;
}

/* Pitched percussion only (no noise, no hiss): a soft woodblock on two
 * and four, and in fights a gentle kick. */
static void perc_step(Player *pl, const Track *k, int step)
{
    if (step == 4 || step == 12) {
        pl->tick_env = .45f;
        pl->tick_ph = 0;
    }
    if (k->fight && (step == 0 || step == 6 || step == 8)) {
        pl->kick_env = step == 0 ? .8f : .5f;
        pl->kick_ph = 0;
    }
}

static float perc(Player *pl)
{
    float out;
    pl->tick_ph += 880.0f / RATE;
    out = sinf(TAU * pl->tick_ph) * pl->tick_env;
    pl->kick_ph += (55.0f + 50.0f * pl->kick_env) / RATE;
    out += sinf(TAU * pl->kick_ph) * pl->kick_env;
    pl->tick_env *= .9965f;
    pl->kick_env *= .9992f;
    return out;
}

/* -------------------------------------------------------------- reverb */

static float reverb(Reverb *r, float in)
{
    float out = 0;
    int i;
    for (i = 0; i < COMBS; i++) {
        float y = r->comb[i][r->ci[i]];
        r->damp[i] = y * .5f + r->damp[i] * .5f;              /* warm: the highs fade first */
        r->comb[i][r->ci[i]] = in + r->damp[i] * .76f;
        r->ci[i] = (r->ci[i] + 1) % comb_len[i];
        out += y;
    }
    out *= .25f;
    for (i = 0; i < ALLS; i++) {
        float b = r->all[i][r->ai[i]], y = b - out;
        r->all[i][r->ai[i]] = out + b * .5f;
        r->ai[i] = (r->ai[i] + 1) % all_len[i];
        out = y;
    }
    return out;
}

/* ----------------------------------------------------------------- mix */

static void on_step(Player *pl, const Track *k, int bar, int step)
{
    if (step % 2 == 0)
        lead_step(pl, k, bar, step / 2);
    piano_step(pl, k, bar, step);
    pluck_step(pl, k, bar, step);
    bass_step(pl, k, bar, step);
    perc_step(pl, k, step);
}

static float sample(Player *pl)
{
    const Track *k = &tracks[pl->track];
    uint32_t step_len = (uint32_t)(RATE * 15 / k->bpm);   /* a sixteenth */
    uint32_t step_no = pl->t / step_len;
    float le, v, dry;
    if (pl->t % step_len == 0)
        on_step(pl, k, (int)(step_no / STEPS), (int)(step_no % STEPS));
    le = lead(pl);
    v = voices(pl);
    dry = le * .30f + v * .38f + bass(pl) * .45f + perc(pl) * .35f;
    dry += reverb(&pl->rv, le * .5f + v * .6f) * .55f;
    pl->t++;
    return dry / (1.0f + fabsf(dry));                      /* soft ceiling, never a hard clip */
}

static void start(Player *pl, int track)
{
    ZeroMemory(pl, sizeof *pl);
    pl->track = track;
}

/* Called by the mixer for each buffer: adds the music into 'acc'. */
void music_render(float *acc, int n)
{
    static const float master[SOUND_VOLUMES] = { 0.0f, 0.033f, 0.06f, 0.094f };   /* about the old loudness */
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
