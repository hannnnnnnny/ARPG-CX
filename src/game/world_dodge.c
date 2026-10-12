/*
 * world_dodge.c - the dodge roll and the blows worth dodging.
 *
 * Guardians wind up their slam for a moment (a ring fills on the ground)
 * and the Fleshrender winds up his hook. A player who rolls out of a
 * wind-up makes a PERFECT DODGE: the boss is open, taking double damage
 * for a few seconds, and drops an extra legendary when it falls. An idle
 * hero steps aside from half the slams on its own, without the reward,
 * so manual play is better but never required.
 */
#include "world_int.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"

#define DODGE_STEP 5               /* px per tick while rolling */
#define AUTO_SIDESTEP_PCT 50

/* Away from the nearest threat, or where the keys point. */
static void dodge_vector(World *w, fx *vx, fx *vy)
{
    const HeroCommand *c = &w->cmd;
    fx tx = w->h.x + FX_FROM_INT(w->h.face * 16), ty = w->h.y;
    int i, best = -1, bd = 1 << 30;
    if (c->mx || c->my) {
        tx = w->h.x + FX_FROM_INT(c->mx * 16);
        ty = w->h.y + FX_FROM_INT(c->my * 16);
    } else {
        for (i = 0; i < w->nmon; i++)
            if (w->mon[i].alive) {
                int d = dist_px(w->h.x, w->h.y, w->mon[i].x, w->mon[i].y) - (w->mon[i].windup > 0 ? 200 : 0);
                if (d < bd) { bd = d; best = i; }
            }
        if (best >= 0) {
            tx = w->h.x * 2 - w->mon[best].x;
            ty = w->h.y * 2 - w->mon[best].y;
        }
    }
    step_toward(w->h.x, w->h.y, tx, ty, FX_FROM_INT(DODGE_STEP), vx, vy);
}

/* A roll that starts inside a wind-up dodges that blow. */
static void mark_dodged(World *w)
{
    int i;
    for (i = 0; i < w->nmon; i++) {
        Monster *m = &w->mon[i];
        if (m->alive && m->windup > 0 && dist_px(m->x, m->y, w->h.x, w->h.y) <= SLAM_RADIUS + 8)
            m->dodged = 1;
    }
    if (w->hook_t > 0)
        w->hook_dodged = true;
}

static void start_dodge(World *w)
{
    int x0 = FX_TO_INT(w->h.x), y0 = FX_TO_INT(w->h.y);
    dodge_vector(w, &w->h.dodge_vx, &w->h.dodge_vy);
    w->h.dodge_t = DODGE_TICKS;
    w->h.dodge_cd = DODGE_CD;
    mark_dodged(w);
    effect(w, FX_DASH, x0, y0, x0 + FX_TO_INT(w->h.dodge_vx * DODGE_TICKS), y0 + FX_TO_INT(w->h.dodge_vy * DODGE_TICKS),
           0, DODGE_TICKS, RGB565(200, 220, 255));
    world_sound(w, SND_CAST_PHYS);
}

bool hero_dodge_update(World *w)
{
    if (w->h.dodge_cd > 0)
        w->h.dodge_cd--;
    if (w->cmd.dodge && w->h.dodge_cd == 0 && w->h.dodge_t == 0)
        start_dodge(w);
    if (w->h.dodge_t <= 0)
        return false;
    w->h.dodge_t--;
    move_body(w, &w->h.x, &w->h.y, w->h.dodge_vx, w->h.dodge_vy, HERO_HALF);
    w->h.moving = true;
    return true;
}

void perfect_dodge(World *w, Monster *m)
{
    m->weak_t = WEAK_TICKS;
    w->perfect_dodges++;
    floater_kind(w, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y) - 26, T("PERFECT DODGE!"), RGB565(255, 210, 80), FL_BIG);
    floater_kind(w, FX_TO_INT(m->x), FX_TO_INT(m->y) - 30, T("WEAK POINT X2"), RGB565(255, 160, 40), FL_BIG);
    world_sound(w, SND_CRIT);
    sig_shake(w, 6, 2);
    w->sig.hitstop = MAX(w->sig.hitstop, 3);
}

/* The slam lands: dodged, sidestepped by an idle hero, or felt. */
static void slam_land(World *w, Profile *p, Monster *m)
{
    bool inside = dist_px(m->x, m->y, w->h.x, w->h.y) <= SLAM_RADIUS;
    effect(w, FX_NOVA, FX_TO_INT(m->x), FX_TO_INT(m->y), 0, 0, SLAM_RADIUS, 14, RGB565(255, 90, 60));
    if (m->dodged)
        perfect_dodge(w, m);
    else if (inside && w->ctl.manual_t == 0 && rng_range(&w->rng, 0, 99) < AUTO_SIDESTEP_PCT)
        floater(w, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y) - 20, T("SIDESTEP"), RGB565(200, 200, 200));
    else if (inside)
        hurt_hero(w, p, m->dmg * 1.5, EL_PHYS, (int)(m - w->mon));
    m->dodged = 0;
}

bool boss_windup(World *w, Profile *p, Monster *m)
{
    if (m->windup > 0) {
        if (--m->windup == 0)
            slam_land(w, p, m);
        return true;                           /* stands its ground while winding up */
    }
    if (--m->slam_cd > 0)
        return false;
    m->slam_cd = 120;
    m->windup = SLAM_WINDUP;
    m->dodged = 0;
    return true;
}
