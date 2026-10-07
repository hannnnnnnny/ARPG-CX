/*
 * world_mon.c - monster behaviour, projectiles and ground effects.
 * (The damage pipeline lives in world_dmg.c.)
 */
#include "world_int.h"
#include "balance.h"
#include "skills.h"
#include "events.h"
#include "../gfx/gfx.h"
#include <string.h>

#define AGGRO_RANGE 110
#define AGGRO_PATH  12

/* ------------------------------------------------------------ monsters */

static void enemy_shot(World *w, const Monster *m)
{
    Proj *pj = spawn_proj(w);
    if (!pj)
        return;
    pj->kind = PJ_ENEMY;
    pj->x = pj->px = m->x;
    pj->y = pj->py = m->y - FX_FROM_INT(4);
    step_toward(pj->x, pj->y, w->h.x, w->h.y, FX(2.4), &pj->vx, &pj->vy);
    pj->life = 70;
    pj->enemy_dmg = m->dmg;
    pj->enemy_el = mon_defs[m->type].element;
    pj->vfx = VX_ENEMY;
}

static void boss_slam(World *w, Profile *p, Monster *m)
{
    if (--m->slam_cd > 0)
        return;
    m->slam_cd = 120;
    effect(w, FX_NOVA, FX_TO_INT(m->x), FX_TO_INT(m->y), 0, 0, 44, 14, RGB565(255, 90, 60));
    if (dist_px(m->x, m->y, w->h.x, w->h.y) <= 44)
        hurt_hero(w, p, m->dmg * 1.5, EL_PHYS, (int)(m - w->mon));
}

void monster_move(World *w, Monster *m, fx speed)
{
    fx dx, dy, wx = w->h.x, wy = w->h.y;
    int half = m->boss ? 10 : MON_HALF;
    if (!body_line_clear(w, m->x, m->y, w->h.x, w->h.y, half)
        && !smooth_waypoint(w, w->fh, m->x, m->y, half, &wx, &wy))
        return;
    step_toward(m->x, m->y, wx, wy, speed, &dx, &dy);
    if (dx)
        m->face = dx > 0 ? 1 : -1;
    move_body(w, &m->x, &m->y, dx, dy, half);
}

static void monster_act(World *w, Profile *p, Monster *m)
{
    const MonDef *d = &mon_defs[m->type];
    int dist = dist_px(m->x, m->y, w->h.x, w->h.y);
    int reach = m->boss ? 26 : 16;
    fx speed = m->boss ? FX(0.6) : d->speed;
    bool los = line_of_sight(w, m->x, m->y, w->h.x, w->h.y);
    if (m->goblin) {
        events_goblin_act(w, m);
        return;
    }
    if (m->special == MS_BUTCHER) {
        butcher_act(w, p, m);
        return;
    }
    if (m->champ & CH_FAST)
        speed = speed * 8 / 5;
    if (m->chill > 0)
        speed /= 2;
    if (m->atk_cd > 0)
        m->atk_cd -= m->chill > 0 && (w->tick & 1) ? 0 : 1;
    if (m->boss)
        boss_slam(w, p, m);
    if ((d->ranged && dist < 100 && los) || dist <= reach) {
        if (m->atk_cd <= 0) {
            m->atk_cd = (int16_t)(m->champ & CH_FRENZIED ? d->atk_ticks * 3 / 5 : d->atk_ticks);
            if (d->ranged && dist > reach)
                enemy_shot(w, m);
            else
                hurt_hero(w, p, m->dmg, d->element, (int)(m - w->mon));
        }
        return;
    }
    if (m->immob == 0)
        monster_move(w, m, speed);
}

/* Push overlapping awake monsters apart so packs spread around the hero. */
static void separate(World *w)
{
    int i, j;
    for (i = 0; i < w->nmon; i++) {
        Monster *a = &w->mon[i];
        if (!a->alive || !a->aggro)
            continue;
        for (j = i + 1; j < w->nmon; j++) {
            Monster *b = &w->mon[j];
            fx dx, dy;
            if (!b->alive || !b->aggro)
                continue;
            dx = a->x - b->x;
            dy = a->y - b->y;
            if (ABS(dx) < FX(9) && ABS(dy) < FX(9)) {
                fx px = dx >= 0 ? FX(0.4) : -FX(0.4), py = dy >= 0 ? FX(0.4) : -FX(0.4);
                move_body(w, &a->x, &a->y, px, py, MON_HALF);
                move_body(w, &b->x, &b->y, -px, -py, MON_HALF);
            }
        }
    }
}

static void tick_statuses(World *w, Profile *p, int i)
{
    Monster *m = &w->mon[i];
    if (m->stun > 0) m->stun--;
    if (m->chill > 0) m->chill--;
    if (m->immob > 0) m->immob--;
    if (m->vuln > 0) m->vuln--;
    tick_dots(w, p, i);
}

void monsters_update(World *w, Profile *p)
{
    int i;
    bool hero_alive = w->h.dead_t == 0;
    for (i = 0; i < w->nmon; i++) {
        Monster *m = &w->mon[i];
        if (!m->alive)
            continue;
        m->anim++;
        if (m->flash > 0) m->flash--;
        tick_statuses(w, p, i);
        if (!m->alive)
            continue;
        if (m->freeze > 0) {
            m->freeze--;
            continue;
        }
        if (m->stun > 0)
            continue;
        if (!m->aggro) {
            int path = w->fh[px_to_cell(m->y)][px_to_cell(m->x)];
            if (path <= AGGRO_PATH && dist_px(m->x, m->y, w->h.x, w->h.y) < AGGRO_RANGE)
                m->aggro = 1;
            continue;
        }
        if (hero_alive)
            monster_act(w, p, m);
    }
    if (w->tick % 2 == 0)
        separate(w);
}

/* --------------------------------------------------------- projectiles */

void world_spawn_ground(World *w, int x, int y, int r, int dur, const Hit *h, bool follow, int vfx)
{
    int i;
    for (i = 0; i < MAX_GROUND; i++)
        if (!w->gr[i].alive) {
            Ground *g = &w->gr[i];
            memset(g, 0, sizeof *g);
            g->alive = 1;
            g->x = FX_FROM_INT(x);
            g->y = FX_FROM_INT(y);
            g->r = (int16_t)r;
            g->dur = (int16_t)dur;
            g->every = 15;
            g->hit = *h;
            g->hit.status_dur = 0;
            g->follow = follow;
            g->color = element_color((Element)h->element);
            g->vfx = (uint8_t)vfx;
            return;
        }
    /* pool full: the effect is simply skipped */
}

static void projectile_hit(World *w, Profile *p, Proj *pj, int target)
{
    int i;
    sig_spear_burst(w, pj);
    if (pj->radius > 0)
        sig_fireball(w, p, pj);
    if (pj->radius > 0) {
        Effect *e = effect(w, FX_BOOM, FX_TO_INT(pj->x), FX_TO_INT(pj->y), 0, 0, pj->radius, 10,
                           element_color((Element)pj->hit.element));
        if (e)
            e->vfx = pj->vfx;
        for (i = 0; i < w->nmon; i++)
            if (w->mon[i].alive && dist_px(pj->x, pj->y, w->mon[i].x, w->mon[i].y) <= pj->radius)
                deal_damage(w, p, i, &pj->hit);
    } else if (target >= 0) {
        deal_damage(w, p, target, &pj->hit);
    }
    if (pj->ground) {
        Hit g = pj->hit;
        g.base *= 0.3;
        if (g.element == EL_FIRE)
            g.status = ST_BURN;
        world_spawn_ground(w, FX_TO_INT(pj->x), FX_TO_INT(pj->y), 24, 90, &g, false,
                           g.element == EL_FIRE ? VX_BURN : VX_POISON_POOL);
    }
}

/* Bone spear and friends shatter where they stop. */
static void projectile_explode(World *w, Profile *p, Proj *pj)
{
    Hit h = pj->hit;
    int i;
    Effect *e = effect(w, FX_BOOM, FX_TO_INT(pj->x), FX_TO_INT(pj->y), 0, 0, 22, 10, RGB565(235, 228, 205));
    if (e)
        e->vfx = pj->vfx;
    h.base *= 0.5;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && dist_px(pj->x, pj->y, w->mon[i].x, w->mon[i].y) <= 22)
            deal_damage(w, p, i, &h);
    sig_spear_end(w, p, pj);
}

static int proj_target(const World *w, const Proj *pj)
{
    int i;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && !((pj->hit_mask >> i) & 1u)
            && dist_px(pj->x, pj->y, w->mon[i].x, w->mon[i].y) <= (w->mon[i].boss ? 14 : 8))
            return i;
    return -1;
}

/* Tornadoes drift toward the nearest enemy they have not hit. */
static void wander(World *w, Proj *pj)
{
    int i, best = -1, bd = 90;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && !((pj->hit_mask >> i) & 1u)) {
            int d = dist_px(pj->x, pj->y, w->mon[i].x, w->mon[i].y);
            if (d < bd) { bd = d; best = i; }
        }
    if (best >= 0)
        step_toward(pj->x, pj->y, w->mon[best].x, w->mon[best].y, FX(2.2), &pj->vx, &pj->vy);
}

static void enemy_proj(World *w, Profile *p, Proj *pj, bool wall)
{
    if (dist_px(pj->x, pj->y, w->h.x, w->h.y) <= 7) {
        hurt_hero(w, p, pj->enemy_dmg, pj->enemy_el, -1);
        pj->alive = 0;
    } else if (wall || --pj->life <= 0) {
        pj->alive = 0;
    }
}

static void hero_proj(World *w, Profile *p, Proj *pj, bool wall)
{
    int t = proj_target(w, pj);
    if (pj->wander && w->tick % 6 == 0)
        wander(w, pj);
    if (t >= 0) {
        projectile_hit(w, p, pj, t);
        pj->hit_mask |= (uint64_t)1 << t;
        if (!pj->pierce) {
            pj->alive = 0;
            if (pj->explode)
                projectile_explode(w, p, pj);
        }
    } else if (wall || --pj->life <= 0) {
        if (pj->radius > 0 || pj->ground)
            projectile_hit(w, p, pj, -1);
        if (pj->explode)
            projectile_explode(w, p, pj);
        pj->alive = 0;
    }
}

void projectiles_update(World *w, Profile *p)
{
    int i;
    for (i = 0; i < MAX_PROJ; i++) {
        Proj *pj = &w->pj[i];
        bool wall;
        if (!pj->alive)
            continue;
        pj->x += pj->vx;
        pj->y += pj->vy;
        wall = !world_walkable(w, px_to_cell(pj->x), px_to_cell(pj->y));
        if (pj->kind == PJ_ENEMY)
            enemy_proj(w, p, pj, wall);
        else
            hero_proj(w, p, pj, wall);
    }
}

/* ------------------------------------------------------------- ground */

void grounds_update(World *w, Profile *p)
{
    int i, m;
    for (i = 0; i < MAX_GROUND; i++) {
        Ground *g = &w->gr[i];
        if (!g->alive)
            continue;
        if (g->follow) {
            g->x = w->h.x;
            g->y = w->h.y;
        }
        if (g->t % g->every == 0)
            for (m = 0; m < w->nmon; m++)
                if (w->mon[m].alive && dist_px(g->x, g->y, w->mon[m].x, w->mon[m].y) <= g->r)
                    deal_damage(w, p, m, &g->hit);
        if (++g->t >= g->dur)
            g->alive = 0;
    }
}
