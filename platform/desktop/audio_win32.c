/*
 * audio_win32.c - sound effects for the Windows build.
 *
 * Every effect is synthesized at start-up from a short recipe (square,
 * triangle, saw, sine or noise layers with pitch sweeps or arpeggios), so
 * there are no audio files to ship and nothing borrowed from other games.
 * A small mixer thread streams 16-bit mono at 22050 Hz through waveOut.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "audio_win32.h"
#include "../../src/core/sound.h"

#define RATE      22050
#define BUF_LEN   441               /* 20 ms per buffer */
#define BUFS      4
#define VOICES    12
#define RETRIGGER (RATE / 20)       /* the same effect at most every 50 ms */

typedef enum { W_SQUARE, W_TRI, W_SAW, W_SINE, W_NOISE } Wave;

/* One synth layer: f0 -> f1 sweep, or 'steps' arpeggio notes f0 * f1^k. */
typedef struct { uint8_t wave; float f0, f1; uint16_t ms, delay; float vol; uint8_t steps; } Layer;
typedef struct { Layer a, b; } Recipe;

static const Recipe recipes[SND_COUNT] = {
    [SND_HIT]            = { { W_NOISE, 0, 0, 40, 0, .30f, 0 }, { W_SQUARE, 180, 90, 50, 0, .18f, 0 } },
    [SND_CRIT]           = { { W_NOISE, 0, 0, 60, 0, .35f, 0 }, { W_SQUARE, 900, 300, 80, 0, .22f, 0 } },
    [SND_KILL]           = { { W_NOISE, 0, 0, 90, 0, .30f, 0 }, { W_TRI, 220, 60, 120, 0, .35f, 0 } },
    [SND_ELITE_DIE]      = { { W_NOISE, 0, 0, 180, 0, .40f, 0 }, { W_SQUARE, 330, 80, 220, 0, .28f, 0 } },
    [SND_BOSS_DIE]       = { { W_NOISE, 0, 0, 500, 0, .45f, 0 }, { W_SAW, 160, 40, 600, 0, .35f, 0 } },
    [SND_HURT]           = { { W_SQUARE, 220, 140, 70, 0, .22f, 0 }, { W_NOISE, 0, 0, 40, 0, .15f, 0 } },
    [SND_DEATH]          = { { W_SAW, 330, 55, 900, 0, .35f, 0 }, { W_NOISE, 0, 0, 300, 0, .18f, 0 } },
    [SND_LEVEL]          = { { W_SQUARE, 523, 1.26f, 400, 0, .22f, 4 }, { W_TRI, 1046, 1.26f, 400, 0, .15f, 4 } },
    [SND_POTION]         = { { W_SINE, 400, 900, 180, 0, .35f, 0 }, { W_NOISE, 0, 0, 60, 0, .08f, 0 } },
    [SND_DROP]           = { { W_TRI, 700, 900, 60, 0, .18f, 0 }, { W_NOISE, 0, 0, 0, 0, 0, 0 } },
    [SND_DROP_RARE]      = { { W_TRI, 880, 1.19f, 180, 0, .25f, 3 }, { W_NOISE, 0, 0, 0, 0, 0, 0 } },
    [SND_DROP_LEGEND]    = { { W_SQUARE, 659, 1.33f, 450, 0, .22f, 4 }, { W_SINE, 1318, 1318, 600, 100, .18f, 0 } },
    [SND_DROP_UNIQUE]    = { { W_SAW, 523, 1.5f, 500, 0, .22f, 3 }, { W_SINE, 1046, 2093, 700, 150, .18f, 0 } },
    [SND_GOLD]           = { { W_SQUARE, 1568, 2093, 40, 0, .10f, 0 }, { W_SQUARE, 2093, 2637, 40, 40, .08f, 0 } },
    [SND_CAST_PHYS]      = { { W_NOISE, 0, 0, 80, 0, .25f, 0 }, { W_SQUARE, 150, 80, 90, 0, .15f, 0 } },
    [SND_CAST_FIRE]      = { { W_NOISE, 0, 0, 220, 0, .30f, 0 }, { W_SAW, 120, 60, 220, 0, .18f, 0 } },
    [SND_CAST_COLD]      = { { W_SINE, 1800, 1200, 200, 0, .18f, 0 }, { W_NOISE, 0, 0, 120, 0, .12f, 0 } },
    [SND_CAST_LIGHTNING] = { { W_NOISE, 0, 0, 150, 0, .32f, 0 }, { W_SQUARE, 2000, 300, 120, 0, .15f, 0 } },
    [SND_CAST_POISON]    = { { W_TRI, 300, 0.85f, 200, 0, .25f, 4 }, { W_NOISE, 0, 0, 100, 0, .08f, 0 } },
    [SND_CAST_SHADOW]    = { { W_SAW, 200, 90, 250, 0, .22f, 0 }, { W_SINE, 90, 70, 300, 0, .20f, 0 } },
    [SND_STAIRS]         = { { W_TRI, 392, 0.84f, 400, 0, .28f, 4 }, { W_NOISE, 0, 0, 0, 0, 0, 0 } },
    [SND_GOBLIN]         = { { W_SQUARE, 1046, 1.12f, 400, 0, .16f, 6 }, { W_NOISE, 0, 0, 0, 0, 0, 0 } },
    [SND_SHRINE]         = { { W_SINE, 523, 1.26f, 600, 0, .25f, 4 }, { W_SINE, 1046, 1046, 800, 200, .12f, 0 } },
    [SND_CHEST]          = { { W_SQUARE, 110, 70, 300, 0, .25f, 0 }, { W_NOISE, 0, 0, 300, 0, .15f, 0 } },
    [SND_AMBUSH]         = { { W_SAW, 110, 100, 500, 0, .28f, 0 }, { W_SQUARE, 98, 92, 500, 0, .18f, 0 } },
    [SND_ACHIEVE]        = { { W_SQUARE, 784, 1.26f, 500, 0, .22f, 4 }, { W_TRI, 1568, 1568, 600, 300, .12f, 0 } },
    [SND_BOUNTY]         = { { W_TRI, 659, 1.26f, 300, 0, .28f, 3 }, { W_NOISE, 0, 0, 0, 0, 0, 0 } },
    [SND_UI_MOVE]        = { { W_SQUARE, 1200, 1200, 15, 0, .08f, 0 }, { W_NOISE, 0, 0, 0, 0, 0, 0 } },
    [SND_UI_OK]          = { { W_SQUARE, 880, 1320, 50, 0, .14f, 0 }, { W_NOISE, 0, 0, 0, 0, 0, 0 } },
    [SND_UI_BACK]        = { { W_SQUARE, 660, 440, 50, 0, .14f, 0 }, { W_NOISE, 0, 0, 0, 0, 0, 0 } },
    [SND_UI_ERROR]       = { { W_SQUARE, 140, 130, 120, 0, .20f, 0 }, { W_NOISE, 0, 0, 0, 0, 0, 0 } },
    [SND_PAGE]           = { { W_NOISE, 0, 0, 30, 0, .10f, 0 }, { W_TRI, 600, 700, 30, 0, .08f, 0 } },
};

typedef struct { int16_t *pcm; int len; } Clip;
typedef struct { const Clip *clip; int pos; float gain; } Voice;

static Clip      g_clips[SND_COUNT];
static Voice     g_voices[VOICES];
static uint32_t  g_last_start[SND_COUNT];
static uint32_t  g_clock;                 /* samples mixed so far */
static HWAVEOUT  g_out;
static WAVEHDR   g_hdr[BUFS];
static int16_t   g_buf[BUFS][BUF_LEN];
static HANDLE    g_event, g_thread;
static volatile LONG g_stop;
static CRITICAL_SECTION g_lock;
static bool      g_ready;
static uint32_t  g_noise = 0x1234567u;

/* ----------------------------------------------------------- synthesis */

static float osc(int wave, float phase)
{
    float p = phase - floorf(phase);
    switch (wave) {
    case W_SQUARE: return p < 0.5f ? 0.8f : -0.8f;
    case W_TRI:    return p < 0.5f ? 4.0f * p - 1.0f : 3.0f - 4.0f * p;
    case W_SAW:    return 2.0f * p - 1.0f;
    case W_SINE:   return sinf(6.2831853f * p);
    default:
        g_noise = g_noise * 1664525u + 1013904223u;
        return (float)(int32_t)g_noise / 2147483648.0f;
    }
}

static float layer_freq(const Layer *l, float t)
{
    if (l->steps > 0) {
        int k = (int)(t * l->steps);
        return l->f0 * powf(l->f1, (float)(k < l->steps ? k : l->steps - 1));
    }
    return l->f0 + (l->f1 - l->f0) * t;
}

/* Adds one layer into 'out' (float accumulation, clipped later). */
static void render_layer(const Layer *l, float *out, int len)
{
    int n = l->ms * RATE / 1000, start = l->delay * RATE / 1000, i;
    float phase = 0;
    for (i = 0; i < n && start + i < len; i++) {
        float t = (float)i / (float)n;
        float env = (i < RATE / 300 ? (float)i / (RATE / 300) : 1.0f) * (1.0f - t) * (1.0f - t);
        phase += layer_freq(l, t) / RATE;
        out[start + i] += osc(l->wave, phase) * env * l->vol;
    }
}

static bool build_clip(const Recipe *r, Clip *c)
{
    int len = (MAX(r->a.ms + r->a.delay, r->b.ms + r->b.delay)) * RATE / 1000, i;
    float *acc;
    if (len <= 0)
        return false;
    acc = (float *)calloc((size_t)len, sizeof *acc);
    c->pcm = (int16_t *)malloc((size_t)len * sizeof *c->pcm);
    if (!acc || !c->pcm) {
        free(acc);
        free(c->pcm);
        c->pcm = NULL;
        return false;
    }
    render_layer(&r->a, acc, len);
    render_layer(&r->b, acc, len);
    for (i = 0; i < len; i++)
        c->pcm[i] = (int16_t)(MAX(-1.0f, MIN(1.0f, acc[i])) * 32000.0f);
    c->len = len;
    free(acc);
    return true;
}

/* --------------------------------------------------------------- mixing */

static void mix(int16_t *out)
{
    static float acc[BUF_LEN];
    int i, v;
    memset(acc, 0, sizeof acc);
    EnterCriticalSection(&g_lock);
    for (v = 0; v < VOICES; v++) {
        Voice *vo = &g_voices[v];
        for (i = 0; vo->clip && i < BUF_LEN && vo->pos < vo->clip->len; i++)
            acc[i] += vo->clip->pcm[vo->pos++] * vo->gain;
        if (vo->clip && vo->pos >= vo->clip->len)
            vo->clip = NULL;
    }
    g_clock += BUF_LEN;
    LeaveCriticalSection(&g_lock);
    for (i = 0; i < BUF_LEN; i++)
        out[i] = (int16_t)MAX(-32767.0f, MIN(32767.0f, acc[i]));
}

static DWORD WINAPI audio_thread(LPVOID arg)
{
    int b;
    (void)arg;
    while (!g_stop) {
        WaitForSingleObject(g_event, 100);
        for (b = 0; b < BUFS && !g_stop; b++)
            if (g_hdr[b].dwFlags & WHDR_DONE) {
                mix(g_buf[b]);
                waveOutWrite(g_out, &g_hdr[b], sizeof g_hdr[b]);
            }
    }
    return 0;
}

static bool open_device(void)
{
    WAVEFORMATEX f;
    int b;
    memset(&f, 0, sizeof f);
    f.wFormatTag = WAVE_FORMAT_PCM;
    f.nChannels = 1;
    f.nSamplesPerSec = RATE;
    f.wBitsPerSample = 16;
    f.nBlockAlign = 2;
    f.nAvgBytesPerSec = RATE * 2;
    if (waveOutOpen(&g_out, WAVE_MAPPER, &f, (DWORD_PTR)g_event, 0, CALLBACK_EVENT) != MMSYSERR_NOERROR)
        return false;
    for (b = 0; b < BUFS; b++) {
        memset(&g_hdr[b], 0, sizeof g_hdr[b]);
        memset(g_buf[b], 0, sizeof g_buf[b]);
        g_hdr[b].lpData = (LPSTR)g_buf[b];
        g_hdr[b].dwBufferLength = sizeof g_buf[b];
        waveOutPrepareHeader(g_out, &g_hdr[b], sizeof g_hdr[b]);
        waveOutWrite(g_out, &g_hdr[b], sizeof g_hdr[b]);   /* prime with silence */
    }
    return true;
}

bool audio_init(void)
{
    int i;
    for (i = 0; i < SND_COUNT; i++)
        build_clip(&recipes[i], &g_clips[i]);
    InitializeCriticalSection(&g_lock);
    g_event = CreateEventA(NULL, FALSE, FALSE, NULL);
    if (!g_event || !open_device())
        return false;                              /* no audio device: play silently */
    g_thread = CreateThread(NULL, 0, audio_thread, NULL, 0, NULL);
    g_ready = g_thread != NULL;
    return g_ready;
}

void audio_shutdown(void)
{
    int b;
    if (!g_ready)
        return;
    InterlockedExchange(&g_stop, 1);
    SetEvent(g_event);
    WaitForSingleObject(g_thread, 1000);
    waveOutReset(g_out);
    for (b = 0; b < BUFS; b++)
        waveOutUnprepareHeader(g_out, &g_hdr[b], sizeof g_hdr[b]);
    waveOutClose(g_out);
    CloseHandle(g_thread);
    CloseHandle(g_event);
    g_ready = false;
}

void audio_play(int id, int volume)
{
    static const float gains[SOUND_VOLUMES] = { 0.0f, 0.3f, 0.6f, 1.0f };
    int v, slot = -1;
    if (!g_ready || id < 0 || id >= SND_COUNT || volume <= 0 || !g_clips[id].pcm)
        return;
    EnterCriticalSection(&g_lock);
    if (g_clock - g_last_start[id] >= RETRIGGER || g_last_start[id] == 0) {
        for (v = 0; v < VOICES && slot < 0; v++)
            if (!g_voices[v].clip)
                slot = v;
        if (slot >= 0) {
            g_voices[slot].clip = &g_clips[id];
            g_voices[slot].pos = 0;
            g_voices[slot].gain = gains[MIN(volume, SOUND_VOLUMES - 1)];
            g_last_start[id] = g_clock ? g_clock : 1;
        }
    }
    LeaveCriticalSection(&g_lock);
}
