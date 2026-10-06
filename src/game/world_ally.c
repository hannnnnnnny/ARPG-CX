/*
 * world_ally.c - minions (skeleton warriors and mages, spirit wolves),
 * corpses left by the dead (for Corpse Explosion) and health potions
 * dropped on the floor.
 */
#include "world_int.h"
#include "../gfx/gfx.h"
#include <string.h>

#define ALLY_LEASH  140      /* px: further than this, minions rejoin the hero */
#define ALLY_SIGHT  90
#define CORPSE_LIFE (20 * TICK_HZ)
#define ORB_LIFE    (30 * TICK_HZ)

/* --------------------------------------------------------------- allies */

int allies_alive(const World *w)
{
    int i, n = 0;
    for (i = 0; i < MAX_ALLY; i++)
        n += w->al[i].alive;
    return n;
}

bool ally_summon(World *w, int kind, int element)
{
    int i;
    for (i = 0; i < MAX_ALLY; i++)
        if (!w->al[i].alive) {
            Ally *a = &w->al[i];
            memset(a, 0, sizeof *a);
            a->alive = 1;
            a->kind = (uint8_t)kind;
            a->element = (uint8_t)element;
            a->skill = 0xFF;                       /* the caster fills this in */
            a->x = a->px = w->h.x + FX_FROM_INT((i % 3 - 1) * 10);
            a->y = a->py = w->h.y + FX_FROM_INT((i / 3 - 1) * 8);
            a->face = 1;
            a->rise = kind == AK_WOLF ? 0 : 20;
            a->atk_cd = (int16_t)(10 + i * 3);
            effect(w, FX_RAISE, FX_TO_INT(a->x), FX_TO_INT(a->y), 0, 0, 8, 20,
                   kind == AK_WOLF ? RGB565(200, 210, 230) : RGB565(120, 255, 200));
            return true;
        }
    return false;
}

static int ally_target(const World *w, const Ally *a)
{
    int i, best = -1, bd = ALLY_SIGHT;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && w->mon[i].aggro) {
            int d = dist_px(a->x, a->y, w->mon[i].x, w->mon[i].y);
            if (d < bd) { bd = d; best = i; }
        }
    return best;
}

static Hit ally_hit(const World *w, const Ally *a)
{
    Hit h = skill_hit(w, a->skill, a->kind == AK_MAGE ? 0.8 : 1.0);
    h.element = a->element;
    h.status = ST_NONE;
    h.minion = true;
    h.flags = 0;
    return h;
}

static void ally_shoot(World *w, const Ally *a, const Monster *m)
{
    Proj *pj = spawn_proj(w);
    if (!pj)
        return;
    pj->kind = PJ_MINION;
    pj->x = pj->px = a->x;
    pj->y = pj->py = a->y - FX_FROM_INT(4);
    step_toward(pj->x, pj->y, m->x, m->y, FX(3.5), &pj->vx, &pj->vy);
    pj->life = 40;
    pj->hit = ally_hit(w, a);
    pj->vfx = VX_MINION_BOLT;
}

static void ally_move(World *w, Ally *a, fx tx, fx ty)
{
    /* Minions always outpace the hero (who gets faster with gear), and
     * sprint when they fall behind so they never trail the fight. */
    double hero = 1.6 * (1.0 + w->st.move_pct / 100.0) * (w->h.buff_t[BUFF_BERSERK] > 0 ? 1.15 : 1.0);
    double mult = (a->kind == AK_WOLF ? 1.35 : 1.2) * (dist_px(a->x, a->y, w->h.x, w->h.y) > 48 ? 1.6 : 1.0);
    fx dx, dy, speed = (fx)(FX(1) * hero * mult);
    step_toward(a->x, a->y, tx, ty, speed, &dx, &dy);
    if (dx)
        a->face = dx > 0 ? 1 : -1;
    move_body(w, &a->x, &a->y, dx, dy, 4);
}

static void ally_act(World *w, Profile *p, Ally *a)
{
    int t = ally_target(w, a), reach = a->kind == AK_MAGE ? 80 : 14;
    if (t < 0) {
        if (dist_px(a->x, a->y, w->h.x, w->h.y) > 26)
            ally_move(w, a, w->h.x, w->h.y);
        return;
    }
    if (dist_px(a->x, a->y, w->mon[t].x, w->mon[t].y) > reach
        || !line_of_sight(w, a->x, a->y, w->mon[t].x, w->mon[t].y)) {
        ally_move(w, a, w->mon[t].x, w->mon[t].y);
        return;
    }
    a->face = w->mon[t].x > a->x ? 1 : -1;
    if (a->atk_cd > 0)
        return;
    a->atk_cd = a->kind == AK_WOLF ? 22 : 30;
    a->anim = 0;
    if (a->kind == AK_MAGE) {
        ally_shoot(w, a, &w->mon[t]);
    } else {
        Hit h = ally_hit(w, a);
        deal_damage(w, p, t, &h);
    }
}

void allies_update(World *w, Profile *p)
{
    int i;
    for (i = 0; i < MAX_ALLY; i++) {
        Ally *a = &w->al[i];
        if (!a->alive)
            continue;
        a->px = a->x;
        a->py = a->y;
        /* Minions only exist while their skill is on the bar. */
        if (a->skill >= CLASS_SKILLS || !w->st.b.skill[a->skill].usable) {
            a->alive = 0;
            continue;
        }
        a->anim++;
        if (a->atk_cd > 0) a->atk_cd--;
        if (a->rise > 0) { a->rise--; continue; }
        if (dist_px(a->x, a->y, w->h.x, w->h.y) > ALLY_LEASH) {
            a->x = a->px = w->h.x;
            a->y = a->py = w->h.y;
        }
        if (w->h.dead_t == 0)
            ally_act(w, p, a);
    }
}

/* -------------------------------------------------------------- corpses */

void corpse_add(World *w, fx x, fx y)
{
    int i, oldest = 0;
    for (i = 0; i < MAX_CORPSE; i++) {
        if (!w->co[i].alive) {
            oldest = i;
            break;
        }
        if (w->co[i].t > w->co[oldest].t)
            oldest = i;
    }
    w->co[oldest].alive = 1;
    w->co[oldest].x = x;
    w->co[oldest].y = y;
    w->co[oldest].t = 0;
}

int corpse_best(const World *w, int range, int r)
{
    int i, best = -1, bn = 0;
    for (i = 0; i < MAX_CORPSE; i++) {
        int n;
        if (!w->co[i].alive || dist_px(w->h.x, w->h.y, w->co[i].x, w->co[i].y) > range)
            continue;
        n = count_near(w, FX_TO_INT(w->co[i].x), FX_TO_INT(w->co[i].y), r);
        if (n > bn) {
            bn = n;
            best = i;
        }
    }
    return best;
}

/* ------------------------------------------------------- health orbs */

void orb_drop(World *w, fx x, fx y)
{
    int i;
    for (i = 0; i < MAX_ORB; i++)
        if (!w->orb[i].alive) {
            w->orb[i].alive = 1;
            w->orb[i].x = x;
            w->orb[i].y = y;
            w->orb[i].t = 0;
            return;
        }
}

static void orb_collect(World *w, Orb *o)
{
    o->alive = 0;
    if (w->h.potions < w->st.potion_max) {
        w->h.potions++;
        floater(w, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y) - 22, "+POTION", RGB565(255, 90, 100));
    } else {
        w->h.hp = MIN(w->st.max_hp, w->h.hp + w->st.max_hp * 0.10);
    }
}

void orbs_update(World *w)
{
    int i;
    for (i = 0; i < MAX_CORPSE; i++)
        if (w->co[i].alive && ++w->co[i].t > CORPSE_LIFE)
            w->co[i].alive = 0;
    for (i = 0; i < MAX_ORB; i++) {
        Orb *o = &w->orb[i];
        int d;
        if (!o->alive)
            continue;
        if (++o->t > ORB_LIFE) {
            o->alive = 0;
            continue;
        }
        d = dist_px(o->x, o->y, w->h.x, w->h.y);
        if (d <= 8)
            orb_collect(w, o);
        else if (d <= 48 && o->t > 10) {   /* potions roll toward the hero */
            fx dx, dy;
            step_toward(o->x, o->y, w->h.x, w->h.y, FX(2.5), &dx, &dy);
            o->x += dx;
            o->y += dy;
        }
    }
}
