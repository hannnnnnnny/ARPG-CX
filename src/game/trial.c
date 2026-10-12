/*
 * trial.c - the daily trial (see trial.h).
 */
#include "trial.h"
#include "build.h"
#include "../gfx/gfx.h"
#include "../gfx/font.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

#define C_TITLE RGB565(255, 200, 90)
#define C_TXT   RGB565(230, 220, 200)
#define C_LOW   RGB565(130, 118, 104)
#define C_CURSE RGB565(240, 90, 80)
#define C_BOON  RGB565(110, 230, 110)
#define C_CODE  RGB565(200, 120, 255)

static uint32_t mix(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    return x ^ (x >> 16);
}

uint32_t trial_day(uint32_t now)  { return now / 86400u; }
uint32_t trial_seed(uint32_t day) { return mix(day * 2654435761u + 0x9E37u) | 1u; }

Door trial_rule(uint32_t day)
{
    Rng r;
    Door d[DOORS];
    rng_seed(&r, trial_seed(day));
    doors_roll(d, &r);
    return d[0];
}

int trial_start_floor(const Profile *p)
{
    return MAX(DOOR_FLOOR, p->best_floor - TRIAL_BACK);
}

static unsigned code_check(uint32_t day, int start, int floors, int cls)
{
    return mix(day ^ (uint32_t)start * 131u ^ (uint32_t)floors * 7919u ^ (uint32_t)cls * 104729u) & 0xFFFu;
}

void trial_code(char *out, size_t cap, uint32_t day, int start, int floors, int cls)
{
    snprintf(out, cap, "AD-%X-%d-%d-%d-%03X", (unsigned)day, start, floors, cls, code_check(day, start, floors, cls));
}

bool trial_code_valid(uint32_t day, int start, int floors, int cls, unsigned check)
{
    return check == code_check(day, start, floors, cls);
}

Profile *trial_saved_profile(Game *g)
{
    return g->trial.active ? &g->trial.backup : &g->p;
}

/* ------------------------------------------------------------- running */

void trial_begin(Game *g, uint32_t now)
{
    TrialState *t = &g->trial;
    t->backup = g->p;
    t->day = trial_day(now);
    t->start = trial_start_floor(&g->p);
    t->ticks = TRIAL_TICKS;
    t->floors = 0;
    t->active = true;
    t->card = t->result = false;
    g->p.seed = trial_seed(t->day);
    g->p.floor = t->start;
    g->p.mode = MODE_PUSH;
    g->s.rule = trial_rule(t->day);
    session_start(&g->s, &g->p);
    g->state = GS_BATTLE;
    world_banner(&g->s.w, "THE DAILY TRIAL BEGINS", C_TITLE);
}

void trial_tick(Game *g)
{
    TrialState *t = &g->trial;
    if (!t->active)
        return;
    t->floors = MAX(t->floors, g->p.floor - t->start);
    if (--t->ticks <= 0)
        trial_end(g);
}

void trial_end(Game *g)
{
    TrialState *t = &g->trial;
    if (!t->active)
        return;
    t->floors = MAX(t->floors, g->p.floor - t->start);
    g->p = t->backup;                              /* nothing from the run is kept but the score */
    if (g->p.trial_day != t->day || t->floors > g->p.trial_best) {
        g->p.trial_day = t->day;
        g->p.trial_best = (uint16_t)MIN(t->floors, 65535);
    }
    t->active = false;
    t->result = true;
    memset(&g->s.rule, 0, sizeof g->s.rule);
    session_start(&g->s, &g->p);
}

/* ------------------------------------------------------------- drawing */

static void panel(int x, int y, int w, int h, uint16_t edge)
{
    gfx_fill_rect(x, y, w, h, RGB565(20, 15, 17));
    gfx_rect(x, y, w, h, edge);
}

void trial_render_hud(const Game *g)
{
    char buf[64];
    const TrialState *t = &g->trial;
    int s = (t->ticks + TICK_HZ - 1) / TICK_HZ;
    snprintf(buf, sizeof buf, T("TRIAL %d:%02d   +%d FLOORS"), s / 60, s % 60, t->floors);
    panel(96, 22, 128, 12, s < 60 ? C_CURSE : C_TITLE);
    font_draw(160 - font_text_width(buf, 1) / 2, 24, buf, s < 60 ? C_CURSE : C_TITLE, 1);
}

static int rule_lines(const Door *d, int x, int y, int w)
{
    font_draw(x, y, "TODAY'S RULES", C_LOW, 1);
    y = font_draw_wrapped(x, y + 11, door_curse_text(d->curse), w, C_CURSE);
    return font_draw_wrapped(x, y, door_reward_text(d->reward), w, C_BOON);
}

void trial_render_card(const Game *g, uint32_t now)
{
    char buf[96];
    uint32_t day = trial_day(now);
    Door d = trial_rule(day);
    int y;
    panel(30, 40, 260, 150, C_TITLE);
    font_draw_centered(48, "DAILY TRIAL", C_TITLE, RGB565(0, 0, 0), 1);
    font_draw_centered(60, "THE SAME FLOORS AND RULES FOR EVERYONE TODAY", C_LOW, RGB565(0, 0, 0), 1);
    snprintf(buf, sizeof buf, T("START AT FLOOR %d. TEN MINUTES: GO AS DEEP AS YOU CAN."), trial_start_floor(&g->p));
    y = font_draw_wrapped(40, 76, buf, 240, C_TXT) + 4;
    y = rule_lines(&d, 40, y, 240) + 4;
    if (g->p.trial_day == day)
        snprintf(buf, sizeof buf, T("YOUR BEST TODAY: +%d FLOORS"), g->p.trial_best);
    else
        snprintf(buf, sizeof buf, "%s", T("NO RUN YET TODAY"));
    font_draw(40, y, buf, C_TITLE, 1);
    font_draw(40, y + 11, "NOTHING FOUND IN THE TRIAL IS KEPT, ONLY THE SCORE.", C_LOW, 1);
    font_draw_centered(176, "ENTER: START   ESC: CLOSE", C_TXT, RGB565(0, 0, 0), 1);
}

void trial_render_result(const Game *g)
{
    char buf[96], code[48];
    const TrialState *t = &g->trial;
    panel(40, 50, 240, 120, C_CODE);
    font_draw_centered(58, "DAILY TRIAL", C_LOW, RGB565(0, 0, 0), 1);
    snprintf(buf, sizeof buf, T("+%d FLOORS"), t->floors);
    font_draw_centered(72, buf, C_TITLE, RGB565(0, 0, 0), 2);
    snprintf(buf, sizeof buf, T("FROM FLOOR %d - %s"), t->start, T(class_defs[g->p.cls % CLASS_COUNT].name));
    font_draw_centered(96, buf, C_TXT, RGB565(0, 0, 0), 1);
    snprintf(buf, sizeof buf, T("BEST TODAY: +%d"), g->p.trial_best);
    font_draw_centered(108, buf, C_LOW, RGB565(0, 0, 0), 1);
    trial_code(code, sizeof code, t->day, t->start, t->floors, g->p.cls);
    font_draw_centered(126, "SHARE CODE", C_LOW, RGB565(0, 0, 0), 1);
    font_draw_centered(138, code, C_CODE, RGB565(0, 0, 0), 1);
    font_draw_centered(156, "ENTER: CONTINUE", C_TXT, RGB565(0, 0, 0), 1);
}
