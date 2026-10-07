/*
 * main_headless.c - deterministic, windowless runner for Ashen Depths.
 *
 *   ad_headless [--script "O:2 -:300 T:2"] [--shot TICK:file.png]...
 *               [--new] [--class 0-5] [--preset 0-2] [--save file] [--trace N] [--fast HOURS]
 *               [--lang 0-4] [--sig] [--record START:COUNT:EVERY:PREFIX] [--sounds FILE]
 *               [--mythic ID,ID...] [--butcher] [--event KIND]
 *
 * --record writes COUNT frames, one every EVERY ticks from tick START, as
 * PREFIX0000.png, PREFIX0001.png ... (README clips are made from these).
 * --sounds logs every sound effect the game asks for as "tick id volume"
 * lines, so a video can be scored with the game's own effects.
 * --mythic wears the listed mythic powers (mythic.h ids) and --butcher
 * makes the Fleshrender walk in; --event replaces the floor's event with
 * one of events_d4.c (FloorEventKind). All three act after --fast.
 *
 * Script tokens BUTTONS:TICKS, letters L R U D O(ok) B(back) T(tab) A(alt)
 * K(lock) F(debug), l / r (mouse buttons), 1-6 (skill keys), q (potion),
 * z (auto battle), and a dash for no buttons. BUTTONS@X/Y:TICKS also moves
 * the mouse pointer to logical pixel (X, Y). --fast simulates HOURS of
 * idle play before the script runs (no rendering), for late-game shots.
 */
#include "../../src/core/platform.h"
#include "../../src/game/game.h"
#include "../../src/game/build.h"
#include "../../src/gfx/sprites.h"
#include "../../src/gfx/gfx.h"
#include "../../src/game/aspects.h"
#include "../../src/game/world_int.h"
#include "../../src/game/items.h"
#include "../../src/game/mythic.h"
#include "../../src/game/events_int.h"
#include "../../src/i18n/i18n.h"
#include "png.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STEPS 256
#define MAX_SHOTS 64

typedef struct { uint32_t buttons; int ticks; int mx, my; } Step;
typedef struct { int tick; const char *path; } Shot;
typedef struct { int start, count, every; const char *prefix; } Record;

static uint16_t g_fb[FB_W * FB_H];
static Game g_game;
static uint32_t g_buttons;
static bool g_mouse;
static int g_mx, g_my;

bool plat_init(void) { return true; }
void plat_shutdown(void) {}
uint32_t plat_read_buttons(void) { return g_buttons; }
void plat_present(const uint16_t *fb) { (void)fb; }
uint32_t plat_time_us(void) { return 0; }
void plat_wait(void) {}
bool plat_quit_requested(void) { return false; }
const char *plat_save_path(void) { return NULL; }
const char *plat_name(void) { return "HEADLESS"; }
const char *plat_clock_desc(void) { return "CLOCK: SCRIPTED"; }
const char *const *plat_control_lines(void) { return NULL; }

static FILE *g_sound_log;   /* --sounds: one "tick id volume" line per effect, for video soundtracks */
static int g_tick;

void plat_sound(int id, int volume)
{
    if (g_sound_log && fprintf(g_sound_log, "%d %d %d\n", g_tick, id, volume) < 0) {
        fprintf(stderr, "sound log write failed, logging stopped\n");
        fclose(g_sound_log);
        g_sound_log = NULL;
    }
}

bool plat_read_mouse(int *x, int *y)
{
    *x = g_mx;
    *y = g_my;
    return g_mouse;
}

static uint32_t parse_buttons(const char *s, size_t n)
{
    static const char keys[] = "LRUDOBTAKFlr123456qz";
    uint32_t b = 0;
    size_t i;
    for (i = 0; i < n && s[i] != '@'; i++) {
        const char *k = s[i] ? strchr(keys, s[i]) : NULL;
        if (k)
            b |= 1u << (k - keys);
    }
    return b;
}

static int parse_script(const char *p, Step *steps)
{
    int n = 0;
    while (*p && n < MAX_STEPS) {
        const char *colon;
        while (*p == ' ' || *p == ',') p++;
        if (!*p || !(colon = strchr(p, ':')))
            break;
        steps[n].buttons = parse_buttons(p, (size_t)(colon - p));
        steps[n].ticks = atoi(colon + 1);
        steps[n].mx = steps[n].my = -1;
        {
            const char *at = memchr(p, '@', (size_t)(colon - p));
            if (at) {
                steps[n].mx = atoi(at + 1);
                steps[n].my = strchr(at, '/') ? atoi(strchr(at, '/') + 1) : 0;
            }
        }
        n++;
        p = colon + 1;
        while (*p && *p != ' ' && *p != ',') p++;
    }
    return n;
}

static void parse_record(const char *arg, Record *r)
{
    const char *p = arg;
    int *v[3] = { &r->start, &r->count, &r->every }, i;
    for (i = 0; i < 3 && p; i++) {
        *v[i] = atoi(p);
        p = strchr(p, ':');
        if (p)
            p++;
    }
    r->prefix = p;
    if (r->every < 1)
        r->every = 1;
}

/* One frame of the recording, if this tick is one of them. */
static void record_frame(const Record *r, int tick)
{
    char path[400];
    int n;
    if (!r->prefix || tick < r->start || (tick - r->start) % r->every)
        return;
    n = (tick - r->start) / r->every;
    if (n >= r->count)
        return;
    game_render(&g_game);
    snprintf(path, sizeof path, "%s%04d.png", r->prefix, n);
    png_write_rgb565(path, g_fb, FB_W, FB_H);
}

/* --sig: wear the class's build-defining unique (demo clips). */
static void equip_signature(Profile *p)
{
    int uid = sig_unique_for(p->cls);
    Rng r;
    if (!uid)
        return;
    rng_seed(&r, 99);
    item_make_unique(&p->equip[unique_def(uid)->slot], &r, MAX(p->best_floor, 1), uid, true, p->cls);
}

/* --mythic 1,5,9: wear those mythic powers (MythicPower ids, demo clips). */
static void equip_mythics(Profile *p, const char *list)
{
    Rng r;
    int rings = 0;
    rng_seed(&r, 77);
    while (list && *list) {
        int uid = mythic_unique(atoi(list));
        const char *comma = strchr(list, ',');
        if (uid) {
            int slot = unique_def(uid)->slot;
            if (slot == SLOT_RING1 && rings++)
                slot = SLOT_RING2;
            item_make_unique(&p->equip[slot], &r, MAX(p->best_floor, 1), uid, true, p->cls);
        }
        list = comma ? comma + 1 : NULL;
    }
}

static void fast_forward(double hours, uint32_t now)
{
    long t;
    Session *s = &g_game.s;
    g_game.state = GS_BATTLE;
    g_game.has_save = true;
    session_start(s, &g_game.p);
    for (t = 0; t < (long)(hours * 3600 * TICK_HZ); t++)
        session_tick(s, &g_game.p);
    while (s->story_pending >= 0)   /* skip story pages queued while fast-forwarding */
        session_story_shown(s, &g_game.p);
    g_game.p.save_time = now;
}

int main(int argc, char **argv)
{
    static Step steps[MAX_STEPS];
    Shot shots[MAX_SHOTS];
    Record rec = { 0, 0, 1, NULL };
    int nsteps = 0, nshots = 0, i, s, tick = 0, trace = 0, cls = 0;
    double fast = 0;
    int preset = 0, lang = -1;
    const char *save = NULL, *mythics = NULL;
    bool fresh = false, sig = false, butcher = false;
    int event = 0;
    uint32_t now = 2000000000u;
    Input in;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--script") && i + 1 < argc) nsteps = parse_script(argv[++i], steps);
        else if (!strcmp(argv[i], "--save") && i + 1 < argc) save = argv[++i];
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fast") && i + 1 < argc) fast = atof(argv[++i]);
        else if (!strcmp(argv[i], "--new")) fresh = true;
        else if (!strcmp(argv[i], "--sig")) sig = true;
        else if (!strcmp(argv[i], "--butcher")) butcher = true;
        else if (!strcmp(argv[i], "--event") && i + 1 < argc) event = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--mythic") && i + 1 < argc) mythics = argv[++i];
        else if (!strcmp(argv[i], "--class") && i + 1 < argc) cls = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--lang") && i + 1 < argc) lang = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--record") && i + 1 < argc) parse_record(argv[++i], &rec);
        else if (!strcmp(argv[i], "--preset") && i + 1 < argc) preset = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--sounds") && i + 1 < argc && !(g_sound_log = fopen(argv[++i], "w"))) {
            fprintf(stderr, "cannot write %s\n", argv[i]);
            return 2;
        }
        else if (!strcmp(argv[i], "--shot") && i + 1 < argc && nshots < MAX_SHOTS) {
            char *arg = argv[++i], *colon = strchr(arg, ':');
            if (colon) { shots[nshots].tick = atoi(arg); shots[nshots].path = colon + 1; nshots++; }
        }
    }
    if (!sprites_init()) {
        fprintf(stderr, "sprite art is malformed\n");
        return 2;
    }
    gfx_bind(g_fb);
    game_init(&g_game, save, now);
    if (fresh) {
        prog_new(&g_game.p, 99, cls);
        build_apply_preset(&g_game.p, preset);
    }
    if (lang >= 0) {
        lang_set(lang);
        g_game.p.lang = (uint8_t)lang_get();
    }
    if (fast > 0)
        fast_forward(fast, now);
    if (sig || mythics) {
        if (sig)
            equip_signature(&g_game.p);
        equip_mythics(&g_game.p, mythics);
        session_profile_changed(&g_game.s, &g_game.p);
    }
    if (butcher)
        g_game.s.w.butcher_t = TICK_HZ;            /* he walks in a second into the script */
    if (event >= EV_HARVEST && event < EV_COUNT) {  /* this floor's event becomes the one asked for */
        memset(&g_game.s.w.ev, 0, sizeof g_game.s.w.ev);
        events_d4_init(&g_game.s.w, event);
    }
    input_init(&in);
    for (s = 0; s <= nsteps; s++) {
        int n = s < nsteps ? steps[s].ticks : 1;
        g_buttons = s < nsteps ? steps[s].buttons : 0;
        if (s < nsteps && steps[s].mx >= 0) {
            g_mouse = true;
            g_mx = steps[s].mx;
            g_my = steps[s].my;
        }
        for (i = 0; i < n; i++, tick++) {
            int k;
            record_frame(&rec, tick);
            for (k = 0; k < nshots; k++)
                if (shots[k].tick == tick) {
                    game_render(&g_game);
                    png_write_rgb565(shots[k].path, g_fb, FB_W, FB_H);
                }
            input_feed(&in, g_buttons);
            in.mouse = plat_read_mouse(&in.mx, &in.my);
            g_tick = tick;
            game_tick(&g_game, &in, now + (uint32_t)(tick / TICK_HZ));
            if (trace && tick % trace == 0)
                printf("t=%d state=%d page=%d floor=%d lvl=%d hp=%.0f kills=%d/%d\n", tick, g_game.state, g_game.page,
                       g_game.p.floor, g_game.p.level, g_game.s.w.h.hp, g_game.s.w.kills, g_game.s.w.quota);
        }
    }
    if (g_sound_log)
        fclose(g_sound_log);
    return 0;
}
