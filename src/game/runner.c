/*
 * runner.c - fixed 30 Hz logic, rendering decoupled from logic.
 *
 * Logic runs at 30 Hz to save battery, but the battle view is drawn up to
 * ~60 FPS with positions interpolated between the last two ticks, so
 * movement looks smooth. Low-power mode draws every second tick (~15 FPS).
 * Wall-clock time (time()) is passed to the game for autosaves and offline
 * progress; on the calculator Ndless backs time() with the RTC.
 */
#include "runner.h"
#include "game.h"
#include "../core/platform.h"
#include "../gfx/sprites.h"
#include "../gfx/gfx.h"
#include "render.h"
#include <time.h>

#define US_PER_SEC      1000000u
#define MAX_TICKS_FRAME 4
#define MAX_FRAME_DT_US 250000u
#define MIN_FRAME_US    16000u  /* cap at ~60 FPS: smooth, but no wasted battery */

static uint16_t g_fb[FB_W * FB_H];
static Game     g_game;

typedef struct { uint32_t start, frames, ticks, logic_us, render_us; } PerfWindow;

static void perf_frame(PerfWindow *w, uint32_t now, uint32_t logic_us, uint32_t render_us, int ticks)
{
    PerfStats s;
    w->frames++;
    w->ticks += (uint32_t)ticks;
    w->logic_us += logic_us;
    w->render_us += render_us;
    if (now - w->start < US_PER_SEC)
        return;
    s.fps = (int)((uint64_t)w->frames * US_PER_SEC / (now - w->start));
    s.logic_us = w->ticks ? (int)(w->logic_us / w->ticks) : 0;
    s.render_us = w->frames ? (int)(w->render_us / w->frames) : 0;
    game_set_perf(&g_game, s);
    w->start = now;
    w->frames = w->ticks = w->logic_us = w->render_us = 0;
}

static int run_ticks(Input *in, uint32_t *acc)
{
    int steps = 0;
    uint32_t wall = (uint32_t)time(NULL);
    while (*acc >= US_PER_SEC && steps < MAX_TICKS_FRAME) {
        input_feed(in, plat_read_buttons());
        in->mouse = plat_read_mouse(&in->mx, &in->my);
        game_tick(&g_game, in, wall);
        *acc -= US_PER_SEC;
        steps++;
    }
    if (*acc >= US_PER_SEC)
        *acc %= US_PER_SEC;
    return steps;
}

int run_game(void)
{
    Input in;
    PerfWindow perf = {0};
    uint32_t last, last_frame, acc = US_PER_SEC;
    int pending = 0;

    if (!sprites_init())
        return 2;
    gfx_bind(g_fb);
    game_init(&g_game, plat_save_path(), (uint32_t)time(NULL));
    input_init(&in);
    last = last_frame = perf.start = plat_time_us();

    while (!game_should_quit(&g_game) && !plat_quit_requested()) {
        uint32_t now = plat_time_us(), dt = now - last, t_logic, t_render;
        bool draw;
        last = now;
        acc += MIN(dt, MAX_FRAME_DT_US) * TICK_HZ;
        pending += run_ticks(&in, &acc);
        if (game_low_power(&g_game))
            draw = pending >= 2;                     /* ~15 FPS, no in-between frames */
        else if (game_animating(&g_game))
            draw = now - last_frame >= MIN_FRAME_US; /* interpolated frames up to ~60 FPS */
        else
            draw = pending > 0;                      /* menus: redraw only when something ticked */
        if (!draw) {
            plat_wait();
            continue;
        }
        /* acc holds the unfinished part of the next tick (us * TICK_HZ). */
        render_set_alpha(game_low_power(&g_game) ? 256 : (int)(acc / (US_PER_SEC / 256)));
        t_logic = plat_time_us();
        game_render(&g_game);
        plat_present(g_fb);
        t_render = plat_time_us();
        perf_frame(&perf, t_render, t_logic - now, t_render - t_logic, pending);
        pending = 0;
        last_frame = now;
    }
    /* Leaving: persist progress so offline time starts counting now. */
    if (g_game.state != GS_TITLE)
        game_save(&g_game, (uint32_t)time(NULL));
    return 0;
}
