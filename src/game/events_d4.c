/*
 * events_d4.c - the dungeon events of Diablo IV's design, in original form:
 *
 *   blood harvest     slay a quota of foes before the time runs out; packs
 *                     keep pouring in around the hero while it lasts
 *   cursed shrine     touching it calls three waves, each tougher
 *   bloodmarked hunt  a champion with an extra affix and its guard
 *   hell rift         a tear that spills monsters until it collapses
 *
 * Event monsters go through spawn_wave (events.c), so wave_left counts
 * them and events.c calls back here when a wave is gone.
 */
#include "events.h"
#include "events_int.h"
#include "world_int.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define HARVEST_TICKS  (45 * TICK_HZ)
#define HARVEST_EVERY  (2 * TICK_HZ)
#define HARVEST_CROWD  6                /* no new pack while this many are around the hero */
#define RIFT_TICKS     (18 * TICK_HZ)
#define RIFT_EVERY     (3 * TICK_HZ)
#define CURSED_WAVES   3
#define C_BLOOD        RGB565(255, 70, 60)
#define C_RIFT         RGB565(190, 110, 255)
#define C_GOLDEN       RGB565(255, 210, 80)

static void reward(World *w, Profile *p, fx x, fx y, int legends, int rares, double gold)
{
    int k;
    for (k = 0; k < legends; k++)
        world_drop_item(w, p, x, y, RAR_LEGEND, 5);
    for (k = 0; k < rares; k++)
        world_drop_item(w, p, x, y, RAR_RARE, 4);
    event_bonus_gold(w, p, gold, FX_TO_INT(x), FX_TO_INT(y) - 16);
    world_sound(w, SND_CHEST);
    world_goal(w, p, GE_EVENT, 0);
    w->ev.state = ES_DONE;
}

/* ---------------------------------------------------------------- hunt */

static void extra_affix(World *w, Monster *m)
{
    int tries;
    for (tries = 0; tries < 20; tries++) {
        int bit = 1 << rng_range(&w->rng, 0, CH_KINDS - 1);
        if (!(m->champ & bit)) {
            m->champ |= (uint8_t)bit;
            return;
        }
    }
}

static void spawn_hunt(World *w)
{
    int cx, cy, k, type = event_wave_type(w), before = w->nmon;
    if (!event_find_cell(w, 14, &cx, &cy))
        return;
    spawn_monster(w, type, cx, cy, true, false);
    if (w->nmon == before)
        return;
    extra_affix(w, &w->mon[before]);
    w->mon[before].special = MS_HUNT;
    w->mon[before].max_hp = w->mon[before].hp = w->mon[before].max_hp * 2.5;
    for (k = 0; k < 4; k++)                     /* the guard: spots that are not walkable are skipped */
        spawn_monster(w, type, cx + (k & 1 ? 2 : -2), cy + (k & 2 ? 1 : -1), false, false);
    w->ev.cx = cx;
    w->ev.cy = cy;
    w->ev.kind = EV_HUNT;
}

/* ---------------------------------------------------------------- init */

void events_d4_init(World *w, int kind)
{
    if (kind == EV_HUNT) {
        spawn_hunt(w);
        return;
    }
    if (kind != EV_HARVEST && !event_find_cell(w, 8, &w->ev.cx, &w->ev.cy))
        return;
    w->ev.kind = (uint8_t)kind;
    if (kind == EV_HARVEST)
        w->ev.goal = (int16_t)(30 + MIN(w->floor / 20, 20));
}

/* ---------------------------------------------------------------- tick */

static void harvest_start(World *w)
{
    char buf[96];
    w->ev.state = ES_RUNNING;
    w->ev.t = HARVEST_TICKS;
    w->ev.spawn_t = 0;
    world_banner(w, "BLOOD HARVEST!", C_BLOOD);
    snprintf(buf, sizeof buf, T("SLAY %d FOES IN %d SECONDS"), w->ev.goal, HARVEST_TICKS / TICK_HZ);
    world_message(w, buf, C_BLOOD);
    world_sound(w, SND_AMBUSH);
}

static void harvest_tick(World *w, Profile *p)
{
    if (w->ev.state == ES_WAITING && w->kills > 0)
        harvest_start(w);
    if (w->ev.state != ES_RUNNING)
        return;
    if (w->ev.count >= w->ev.goal) {
        world_banner(w, "THE HARVEST IS REAPED", C_GOLDEN);
        reward(w, p, w->h.x, w->h.y, 2, 1, 40);
        return;
    }
    if (--w->ev.t <= 0) {
        world_message(w, "THE HARVEST ENDS", C_BLOOD);
        if (w->ev.count >= w->ev.goal / 2)
            reward(w, p, w->h.x, w->h.y, 0, 2, 15);
        w->ev.state = ES_DONE;
        return;
    }
    if (--w->ev.spawn_t <= 0 && count_near(w, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y), 110) < HARVEST_CROWD) {
        w->ev.spawn_t = HARVEST_EVERY;
        spawn_wave(w, px_to_cell(w->h.x), px_to_cell(w->h.y), 5, ++w->ev.stage % 3 == 0);
    }
}

static void rift_tick(World *w)
{
    if (w->ev.state != ES_RUNNING || w->ev.stage)
        return;
    if (--w->ev.t <= 0) {
        w->ev.stage = 1;                            /* closed: the last of them still has to fall */
        world_message(w, "THE RIFT COLLAPSES", C_RIFT);
        effect(w, FX_NOVA, w->ev.cx * TILE_SIZE + TILE_SIZE / 2, w->ev.cy * TILE_SIZE + TILE_SIZE / 2, 0, 0, 40,
               16, C_RIFT);
        return;
    }
    if (--w->ev.spawn_t <= 0) {
        w->ev.spawn_t = RIFT_EVERY;
        spawn_wave(w, w->ev.cx, w->ev.cy, 4, (w->ev.t / RIFT_EVERY) & 1);
    }
}

static void hunt_tick(World *w)
{
    int i;
    if (w->ev.state != ES_WAITING)
        return;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && w->mon[i].special == MS_HUNT && w->mon[i].aggro) {
            w->ev.state = ES_RUNNING;
            world_banner(w, "A BLOODMARKED CHAMPION!", C_BLOOD);
            world_sound(w, SND_AMBUSH);
            return;
        }
}

void events_d4_tick(World *w, Profile *p)
{
    switch (w->ev.kind) {
    case EV_HARVEST: harvest_tick(w, p); break;
    case EV_RIFT:    rift_tick(w); break;
    case EV_HUNT:    hunt_tick(w); break;
    default: break;
    }
    if (w->ev.kind == EV_RIFT && w->ev.stage && w->ev.wave_left == 0 && w->ev.state == ES_RUNNING) {
        world_banner(w, "THE RIFT IS SEALED", C_GOLDEN);
        reward(w, p, cell_center(w->ev.cx), cell_center(w->ev.cy), 2, 1, 35);
    }
}

/* ---------------------------------------------------------- callbacks */

void events_d4_on_kill(World *w, Profile *p, const Monster *m)
{
    if (w->ev.kind == EV_HARVEST && w->ev.state == ES_RUNNING)
        w->ev.count++;
    if (m->special == MS_HUNT && w->ev.kind == EV_HUNT && w->ev.state != ES_DONE) {
        world_banner(w, "BLOODMARKED CHAMPION SLAIN", C_GOLDEN);
        reward(w, p, m->x, m->y, 2, 1, 30);
    }
}

void events_d4_touch(World *w, Profile *p)
{
    (void)p;
    w->ev.state = ES_RUNNING;
    if (w->ev.kind == EV_CURSED) {
        w->ev.stage = 1;
        world_banner(w, "CURSED SHRINE", C_BLOOD);
        world_message(w, "SURVIVE THREE WAVES", C_BLOOD);
        spawn_wave(w, w->ev.cx, w->ev.cy, 6, 1);
    } else if (w->ev.kind == EV_RIFT) {
        w->ev.t = RIFT_TICKS;
        w->ev.spawn_t = 0;
        world_banner(w, "A HELL RIFT TEARS OPEN!", C_RIFT);
    }
    world_sound(w, SND_AMBUSH);
}

bool events_d4_wave_cleared(World *w, Profile *p)
{
    switch (w->ev.kind) {
    case EV_HARVEST:
    case EV_RIFT:
        return true;                                /* timed: the tick decides */
    case EV_CURSED:
        if (w->ev.stage < CURSED_WAVES) {
            w->ev.stage++;
            spawn_wave(w, px_to_cell(w->h.x), px_to_cell(w->h.y), 5 + w->ev.stage * 2, w->ev.stage);
            world_sound(w, SND_AMBUSH);
            if (w->ev.wave_left > 0)
                return true;
        }
        world_banner(w, "THE CURSE IS LIFTED", C_GOLDEN);
        reward(w, p, cell_center(w->ev.cx), cell_center(w->ev.cy), 2, 2, 45);
        return true;
    default:
        return false;
    }
}

bool events_d4_running(const World *w)
{
    return (w->ev.kind == EV_HARVEST || w->ev.kind == EV_RIFT) && w->ev.state == ES_RUNNING;
}

/* HUD line for the event under way ("" when there is nothing to show). */
void events_d4_status(const World *w, char *out, size_t cap)
{
    out[0] = '\0';
    if (w->ev.state != ES_RUNNING)
        return;
    if (w->ev.kind == EV_HARVEST)
        snprintf(out, cap, T("HARVEST %d/%d  %dS"), MIN(w->ev.count, w->ev.goal), w->ev.goal,
                 w->ev.t / TICK_HZ + 1);
    else if (w->ev.kind == EV_CURSED)
        snprintf(out, cap, T("CURSED SHRINE  WAVE %d/%d"), w->ev.stage, CURSED_WAVES);
    else if (w->ev.kind == EV_RIFT && !w->ev.stage)
        snprintf(out, cap, T("HELL RIFT  %dS"), w->ev.t / TICK_HZ + 1);
    else if (w->ev.kind == EV_RIFT)
        snprintf(out, cap, T("HELL RIFT  %d LEFT"), w->ev.wave_left);
    else if (w->ev.kind == EV_HUNT)
        snprintf(out, cap, "%s", T("HUNT: THE BLOODMARKED"));
}
