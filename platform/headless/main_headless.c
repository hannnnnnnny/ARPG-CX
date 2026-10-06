/*
 * main_headless.c - deterministic, windowless runner for Ashen Depths.
 *
 *   ad_headless [--script "O:2 -:300 T:2"] [--shot TICK:file.png]...
 *               [--new] [--class 0-5] [--preset 0-2] [--save file] [--trace N] [--fast HOURS]
 *               [--lang 0-4]
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
#include "../../src/i18n/i18n.h"
#include "png.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STEPS 256
#define MAX_SHOTS 64

typedef struct { uint32_t buttons; int ticks; int mx, my; } Step;
typedef struct { int tick; const char *path; } Shot;

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
void plat_sound(int id, int volume) { (void)id; (void)volume; }

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
    int nsteps = 0, nshots = 0, i, s, tick = 0, trace = 0, cls = 0;
    double fast = 0;
    int preset = 0, lang = -1;
    const char *save = NULL;
    bool fresh = false;
    uint32_t now = 2000000000u;
    Input in;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--script") && i + 1 < argc) nsteps = parse_script(argv[++i], steps);
        else if (!strcmp(argv[i], "--save") && i + 1 < argc) save = argv[++i];
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fast") && i + 1 < argc) fast = atof(argv[++i]);
        else if (!strcmp(argv[i], "--new")) fresh = true;
        else if (!strcmp(argv[i], "--class") && i + 1 < argc) cls = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--lang") && i + 1 < argc) lang = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--preset") && i + 1 < argc) preset = atoi(argv[++i]);
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
            for (k = 0; k < nshots; k++)
                if (shots[k].tick == tick) {
                    game_render(&g_game);
                    png_write_rgb565(shots[k].path, g_fb, FB_W, FB_H);
                }
            input_feed(&in, g_buttons);
            in.mouse = plat_read_mouse(&in.mx, &in.my);
            game_tick(&g_game, &in, now + (uint32_t)(tick / TICK_HZ));
            if (trace && tick % trace == 0)
                printf("t=%d state=%d page=%d floor=%d lvl=%d hp=%.0f kills=%d/%d\n", tick, g_game.state, g_game.page,
                       g_game.p.floor, g_game.p.level, g_game.s.w.h.hp, g_game.s.w.kills, g_game.s.w.quota);
        }
    }
    return 0;
}
