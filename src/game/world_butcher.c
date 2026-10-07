/*
 * world_butcher.c - the Fleshrender, a rare butcher who walks onto an
 * ordinary floor and hunts the hero down, in the spirit of Diablo IV's
 * dungeon Butcher: about one floor in forty from floor 20, never on a
 * guardian floor. He is faster than most monsters, throws a chain that
 * drags the hero to his cleaver, enrages when hurt and pays out like a
 * guardian when he falls.
 */
#include "world_int.h"
#include "events.h"
#include "events_int.h"
#include "mythic.h"
#include "../gfx/gfx.h"
#include "../core/sound.h"

#define BUTCHER_PERMILLE  25            /* chance per ordinary floor, per thousand */
#define BUTCHER_MIN_FLOOR 20
#define HOOK_WINDUP       14            /* ticks of warning before the chain lands */
#define HOOK_COOLDOWN     (6 * TICK_HZ)
#define HOOK_MIN          36            /* px: closer than this he just swings */
#define HOOK_MAX          120
#define C_MEAT            RGB565(200, 40, 40)
#define C_CHAIN           RGB565(170, 160, 150)

void butcher_init(World *w)
{
    w->butcher_t = 0;
    w->hook_t = w->hook_cd = 0;
    if (w->boss_floor || w->floor < BUTCHER_MIN_FLOOR || rng_range(&w->rng, 0, 999) >= BUTCHER_PERMILLE)
        return;
    w->butcher_t = rng_range(&w->rng, 8 * TICK_HZ, 20 * TICK_HZ);
}

static void butcher_arrive(World *w)
{
    int cx, cy, before = w->nmon;
    Monster *m;
    if (!event_find_cell(w, 12, &cx, &cy))
        return;
    spawn_monster(w, MT_BUTCHER, cx, cy, false, false);
    if (w->nmon == before)
        return;
    m = &w->mon[before];
    m->special = MS_BUTCHER;
    m->aggro = 1;
    m->max_hp = m->hp = m->max_hp * 20.0;
    m->dmg *= 1.8;
    m->px = m->x;
    m->py = m->y;
    w->hook_cd = HOOK_COOLDOWN / 2;
    world_banner(w, "THE FLESHRENDER HAS COME!", C_MEAT);
    world_message(w, "SOMETHING HEAVY DRAGS A CLEAVER...", C_MEAT);
    world_sound(w, SND_AMBUSH);
    sig_shake(w, 12, 3);
}

void butcher_tick(World *w, Profile *p)
{
    (void)p;
    if (w->butcher_t > 0 && --w->butcher_t == 0 && w->h.dead_t == 0)
        butcher_arrive(w);
}

/* The chain: after a wind-up, the hero is dragged to his feet. */
static void hook(World *w, Profile *p, Monster *m)
{
    int i;
    fx tx = m->x + (w->h.x > m->x ? FX(10) : -FX(10)), ty = m->y;
    effect(w, FX_BOLT, FX_TO_INT(m->x), FX_TO_INT(m->y), FX_TO_INT(w->h.x), FX_TO_INT(w->h.y), 0, 8, C_CHAIN);
    if (dist_px(m->x, m->y, w->h.x, w->h.y) > HOOK_MAX + 20 || hero_has(w, BUFF_UNSTOP))
        return;
    for (i = 0; i < 16; i++) {                       /* drag along the line, stopping at walls */
        fx dx, dy;
        step_toward(w->h.x, w->h.y, tx, ty, FX(6), &dx, &dy);
        move_body(w, &w->h.x, &w->h.y, dx, dy, HERO_HALF);
    }
    hurt_hero(w, p, m->dmg * 0.5, EL_PHYS, (int)(m - w->mon));
    sig_shake(w, 6, 2);
    world_sound(w, SND_HURT);
}

static bool hook_ready(const World *w, const Monster *m, int dist)
{
    return w->hook_cd <= 0 && dist >= HOOK_MIN && dist <= HOOK_MAX && line_of_sight(w, m->x, m->y, w->h.x, w->h.y);
}

void butcher_act(World *w, Profile *p, Monster *m)
{
    int dist = dist_px(m->x, m->y, w->h.x, w->h.y);
    bool enraged = m->hp < m->max_hp * 0.4;
    fx speed = enraged ? FX(1.85) : FX(1.45);
    if (m->chill > 0)
        speed /= 2;
    if (w->hook_cd > 0)
        w->hook_cd--;
    if (m->atk_cd > 0)
        m->atk_cd--;
    if (w->hook_t > 0) {                             /* winding up: stands still, chain glinting */
        effect(w, FX_WARN, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y), 0, 0, 10, 2, C_CHAIN);
        if (--w->hook_t == 0) {
            hook(w, p, m);
            w->hook_cd = HOOK_COOLDOWN;
        }
        return;
    }
    if (hook_ready(w, m, dist)) {
        w->hook_t = HOOK_WINDUP;
        return;
    }
    if (dist <= 22) {
        if (m->atk_cd <= 0) {
            m->atk_cd = enraged ? 26 : 40;
            effect(w, FX_SLASH, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y), 0, 0, 14, 8, C_MEAT);
            hurt_hero(w, p, m->dmg, EL_PHYS, (int)(m - w->mon));
            sig_shake(w, 4, 2);
        }
        return;
    }
    if (m->immob == 0 && m->stun == 0)
        monster_move(w, m, speed);
}

void butcher_on_kill(World *w, Profile *p, const Monster *m)
{
    int k;
    if (m->special != MS_BUTCHER)
        return;
    for (k = 0; k < 3; k++)
        world_drop_item(w, p, m->x, m->y, RAR_LEGEND, 8);
    if (rng_range(&w->rng, 0, 99) < 35)
        world_drop_item(w, p, m->x, m->y, RAR_UNIQUE, 8);
    mythic_try_drop(w, p, m->x, m->y, MYTHIC_ODDS_BUTCHER);
    event_bonus_gold(w, p, 150, FX_TO_INT(m->x), FX_TO_INT(m->y) - 24);
    p->souls += 10;
    world_banner(w, "THE FLESHRENDER IS SLAIN!", RGB565(255, 210, 80));
    world_sound(w, SND_BOSS_DIE);
    sig_shake(w, 14, 4);
    world_goal(w, p, GE_ELITE, 0);
    world_goal(w, p, GE_EVENT, 0);
}
