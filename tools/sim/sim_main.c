/*
 * sim_main.c - balance simulator: idles every class and build preset for
 * a number of game hours without rendering and prints the progress.
 *
 *   ad_sim [hours] [class] [preset]
 */
#include "../../src/game/session.h"
#include "../../src/game/progress.h"
#include "../../src/game/build.h"
#include "../../src/game/paragon.h"
#include "../../src/core/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The simulator never draws or reads keys: platform stubs. */
bool plat_init(void) { return true; }
void plat_shutdown(void) {}
uint32_t plat_read_buttons(void) { return 0; }
void plat_present(const uint16_t *fb) { (void)fb; }
uint32_t plat_time_us(void) { return 0; }
void plat_wait(void) {}
bool plat_quit_requested(void) { return false; }
const char *plat_save_path(void) { return NULL; }
const char *plat_name(void) { return "SIM"; }
const char *plat_clock_desc(void) { return "SIM"; }
const char *const *plat_control_lines(void) { return NULL; }
bool plat_read_mouse(int *x, int *y) { (void)x; (void)y; return false; }
void plat_sound(int id, int volume) { (void)id; (void)volume; }

static Profile P;
static Session S;

static void run(int cls, int preset, double hours)
{
    long t, ticks = (long)(hours * 3600 * TICK_HZ);
    Stats st;
    memset(&S, 0, sizeof S);
    prog_new(&P, 1234u + (uint32_t)cls * 77u + (uint32_t)preset, cls);
    build_apply_preset(&P, preset);
    session_start(&S, &P);
    for (t = 0; t < ticks; t++) {
        session_tick(&S, &P);
        while (S.story_pending >= 0)
            session_story_shown(&S, &P);
    }
    stats_compute(&st, &P);
    printf("%-11s %-15s floor %3d best %3d lv %2d+%3d deaths %4d stuck %3d gold %.3g dps %.3g hp %.3g "
           "crit %.0f%% vuln %.0f%% op %.0f%%\n",
           class_defs[cls].name, class_defs[cls].preset[preset].name, P.floor, P.best_floor, P.level,
           P.paragon_level, S.deaths, S.stuck_resets, P.gold, stats_dps(&st, &P), st.max_hp,
           100.0 * S.w.hs.crits / (S.w.hs.hits + 1e-9), 100.0 * S.w.hs.vulns / (S.w.hs.hits + 1e-9),
           100.0 * S.w.hs.ops / (S.w.hs.hits + 1e-9));
}

int main(int argc, char **argv)
{
    double hours = argc > 1 ? atof(argv[1]) : 1.0;
    int c0 = 0, c1 = CLASS_COUNT - 1, p0 = 0, p1 = PRESETS - 1;
    if (argc > 2) c0 = c1 = atoi(argv[2]);
    if (argc > 3) p0 = p1 = atoi(argv[3]);
    for (; c0 <= c1; c0++) {
        int p;
        for (p = p0; p <= p1; p++)
            run(c0, p, hours);
    }
    return 0;
}
