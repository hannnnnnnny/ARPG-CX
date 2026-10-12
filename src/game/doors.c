/*
 * doors.c - the choice between floors (see doors.h).
 */
#include "doors.h"
#include "events.h"
#include "balance.h"
#include "../gfx/gfx.h"
#include <string.h>

/* How much each curse hurts and each reward is worth (1 mild .. 3 harsh / rich). */
static const uint8_t curse_level[DC_COUNT] = { 0, 2, 3, 1, 2 };
static const uint8_t reward_level[DR_COUNT] = { 0, 2, 1, 1, 2, 2, 3 };
/* The mythic door is the rare one. */
static const uint8_t reward_weight[DR_COUNT] = { 0, 20, 20, 20, 15, 15, 10 };

bool doors_open_on(int floor)
{
    return floor >= DOOR_FLOOR && !is_boss_floor(floor);
}

static int pick_reward(Rng *r, const Door *taken, int n)
{
    int total = 0, k, roll;
    bool used[DR_COUNT] = { false };
    for (k = 0; k < n; k++)
        used[taken[k].reward] = true;
    for (k = 1; k < DR_COUNT; k++)
        total += used[k] ? 0 : reward_weight[k];
    roll = rng_range(r, 0, total - 1);
    for (k = 1; k < DR_COUNT - 1; k++) {
        if (used[k])
            continue;
        if (roll < reward_weight[k])
            return k;
        roll -= reward_weight[k];
    }
    return DR_COUNT - 1;
}

/* A curse as heavy as the reward is rich; ties broken at random. */
static int pick_curse(Rng *r, int reward)
{
    int c, best = DC_MON_HP, n = 0;
    for (c = 1; c < DC_COUNT; c++)
        if (curse_level[c] == reward_level[reward] && rng_range(r, 0, n++) == 0)
            best = c;
    return best;
}

void doors_roll(Door out[DOORS], Rng *r)
{
    int i;
    for (i = 0; i < DOORS; i++) {
        out[i].reward = (uint8_t)pick_reward(r, out, i);
        out[i].curse = (uint8_t)pick_curse(r, out[i].reward);
    }
}

/* Higher is better for the preference; the safest door wins every tie. */
static int door_score(const Door *d, int pref)
{
    int safe = (4 - curse_level[d->curse % DC_COUNT]) * 10 + reward_level[d->reward % DR_COUNT];
    switch (pref) {
    case DP_LOOT:
        return (d->reward == DR_MYTHIC ? 400 : d->reward == DR_ELITE_LEGEND ? 300 : d->reward == DR_LOOT ? 200
                : d->reward == DR_GOBLIN ? 100 : 0) + safe;
    case DP_GOLD: return (d->reward == DR_GOLD ? 200 : d->reward == DR_GOBLIN ? 100 : 0) + safe;
    case DP_XP:   return (d->reward == DR_XP ? 200 : 0) + safe;
    default:      return safe;
    }
}

int doors_auto_pick(const Door d[DOORS], int pref)
{
    int i, best = 0;
    for (i = 1; i < DOORS; i++)
        if (door_score(&d[i], pref) > door_score(&d[best], pref))
            best = i;
    return best;
}

const char *door_name(const Door *d)
{
    static const char *const n[DR_COUNT] = { "", "DOOR OF FORTUNE", "DOOR OF GREED", "DOOR OF WISDOM",
                                             "DOOR OF THE THIEF", "DOOR OF CHAMPIONS", "DOOR OF THE ABYSS" };
    return n[d->reward % DR_COUNT];
}

const char *door_curse_text(int curse)
{
    static const char *const n[DC_COUNT] = { "", "MONSTERS +50% LIFE", "MONSTERS +40% DAMAGE",
                                             "NO HEALTH ORBS", "1 IN 4 MONSTERS IS AN ELITE" };
    return n[curse % DC_COUNT];
}

const char *door_reward_text(int reward)
{
    static const char *const n[DR_COUNT] = { "", "BETTER LOOT X2", "GOLD X3", "EXPERIENCE X2",
                                             "A TREASURE GOBLIN INSIDE", "ELITES DROP LEGENDARIES",
                                             "ELITES MAY DROP A MYTHIC (VERY RARE)" };
    return n[reward % DR_COUNT];
}

const char *door_pref_name(int pref)
{
    static const char *const n[DP_COUNT] = { "SAFEST", "BEST LOOT", "MOST GOLD", "MOST EXPERIENCE" };
    return n[pref % DP_COUNT];
}

uint16_t door_color(const Door *d)
{
    switch (d->reward) {
    case DR_MYTHIC:       return RGB565(200, 120, 255);
    case DR_ELITE_LEGEND: return RGB565(255, 140, 30);
    case DR_LOOT:         return RGB565(255, 230, 90);
    case DR_GOLD:         return RGB565(250, 212, 80);
    case DR_GOBLIN:       return RGB565(120, 220, 120);
    default:              return RGB565(140, 172, 255);
    }
}

/* ---------------------------------------------------------------- floor */

void door_on_spawn(World *w, Monster *m)
{
    if (m->boss)
        return;
    if (w->door.curse == DC_MON_HP)
        m->max_hp = m->hp = m->max_hp * DOOR_HP_MULT;
    else if (w->door.curse == DC_MON_DMG)
        m->dmg *= DOOR_DMG_MULT;
}

/* Elites as spawn_monster makes them: x3 life, x1.6 damage, a champion affix. */
static void make_elite(World *w, Monster *m)
{
    m->elite = 1;
    m->max_hp = m->hp = m->max_hp * 3.0;
    m->dmg *= 1.6;
    champion_roll(w, m);
}

void world_apply_door(World *w, Profile *p, Door d)
{
    int i;
    (void)p;
    w->door = d;
    for (i = 0; i < w->nmon; i++) {
        Monster *m = &w->mon[i];
        if (!m->alive || m->boss || m->goblin || m->special)
            continue;
        if (d.curse == DC_ELITES && !m->elite && rng_range(&w->rng, 0, 99) < DOOR_ELITE_PCT)
            make_elite(w, m);
        door_on_spawn(w, m);
    }
    if (d.reward == DR_GOBLIN)
        events_force_goblin(w);
}
