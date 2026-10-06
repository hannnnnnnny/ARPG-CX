/*
 * world_cast.c - one code path per skill behaviour: arc, projectile, nova,
 * chain, channel, strike, ground, buff, summon, dash and corpse. Each
 * returns false when casting now would be pointless, so the idle hero only
 * fires skills where they matter.
 */
#include "world_int.h"
#include "build.h"
#include "../core/trig.h"
#include "../gfx/gfx.h"
#include <string.h>

#define BARRIER_TICKS (5 * TICK_HZ)

static int hx(const World *w) { return FX_TO_INT(w->h.x); }
static int hy(const World *w) { return FX_TO_INT(w->h.y); }

int count_near(const World *w, int x, int y, int r)
{
    int i, n = 0;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && dist_px(FX_FROM_INT(x), FX_FROM_INT(y), w->mon[i].x, w->mon[i].y) <= r)
            n++;
    return n;
}

/* Monster with the most neighbours within 'r', inside 'range' of the hero. */
int densest_pack(const World *w, int range, int r, int *count)
{
    int i, best = -1, bn = 0;
    for (i = 0; i < w->nmon; i++) {
        const Monster *m = &w->mon[i];
        int n;
        if (!m->alive || dist_px(w->h.x, w->h.y, m->x, m->y) > range)
            continue;
        n = count_near(w, FX_TO_INT(m->x), FX_TO_INT(m->y), r) + (m->boss ? 4 : 0);
        if (n > bn) {
            bn = n;
            best = i;
        }
    }
    *count = bn;
    return best;
}

static Effect *tag(Effect *e, const SkillRT *s)
{
    if (e)
        e->vfx = s->vfx;
    return e;
}

/* Status durations only make sense for one-off hits. */
static bool duration_is_status(int behavior)
{
    return behavior != SB_GROUND && behavior != SB_CHANNEL && behavior != SB_BUFF && behavior != SB_SUMMON;
}

Hit skill_hit(const World *w, int i, double mult)
{
    const SkillRT *s = &w->st.b.skill[i % CLASS_SKILLS];
    Hit h = { 0 };
    h.base = w->st.weapon * s->coef * mult;
    h.element = s->element;
    h.status = s->status;
    h.status_dur = (int16_t)(duration_is_status(s->behavior) ? s->duration : 0);
    h.skill = (uint8_t)i;
    h.flags = s->flags;
    h.crit_add = s->crit_add;
    h.op_add = s->op_add;
    h.lucky = s->lucky;
    /* Imbuements turn other skills' hits into their element and status. */
    if (w->h.buff_t[BUFF_IMBUE] > 0 && s->tag != TAG_IMBUE && s->cat <= CAT_CORE) {
        h.element = w->h.imbue_el;
        if (h.status == ST_NONE)
            h.status = w->h.imbue_st;
        h.flags |= w->h.imbue_flags & RF_VULN;
        h.base *= (1.0 + w->h.buff_val[BUFF_IMBUE] / 100.0) * w->st.b.x_tag[TAG_IMBUE];
    }
    return h;
}

static void area_hit(World *w, Profile *p, int x, int y, int r, int skill, double mult)
{
    Hit h = skill_hit(w, skill, mult);
    int i;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && dist_px(FX_FROM_INT(x), FX_FROM_INT(y), w->mon[i].x, w->mon[i].y) <= r)
            deal_damage(w, p, i, &h);
}

/* ------------------------------------------------------ hero states */

void hero_add_barrier(World *w, double frac_of_life)
{
    double amount = w->st.max_hp * frac_of_life * (1.0 + w->st.barrier_gen);
    w->h.barrier = MIN(w->st.max_hp, MAX(w->h.barrier, 0) + amount);
    w->h.barrier_t = BARRIER_TICKS;
}

void hero_buff(World *w, int kind, double val, int ticks)
{
    if (kind <= BUFF_NONE || kind >= BUFF_COUNT)
        return;
    w->h.buff_t[kind] = MAX(w->h.buff_t[kind], ticks);
    w->h.buff_val[kind] = val;
}

void hero_gain_res(World *w, double amount)
{
    w->h.res = CLAMP(w->h.res + amount, 0.0, world_max_res(w));
}

/* Side effects every cast can carry (flags from upgrades and aspects). */
static void cast_extras(World *w, const SkillRT *s)
{
    if (s->flags & RF_BERSERK)
        hero_buff(w, BUFF_BERSERK, MAX(w->h.buff_val[BUFF_BERSERK], 25), 4 * TICK_HZ);
    if (s->flags & RF_BARRIER)
        hero_add_barrier(w, s->behavior == SB_NOVA ? 0.25 : 0.05);
    if (s->flags & RF_UNSTOP)
        hero_buff(w, BUFF_UNSTOP, MAX(w->h.buff_val[BUFF_UNSTOP], 0), 2 * TICK_HZ);
    if (s->flags & RF_HASTE)
        w->h.haste_t = 3 * TICK_HZ;
    if (s->cd_ticks > 0 && w->st.b.barrier_cd > 0)
        hero_add_barrier(w, w->st.b.barrier_cd / 100.0);
    w->h.cast_t = 8;
}

/* ------------------------------------------------------- projectiles */

Proj *spawn_proj(World *w)
{
    int i;
    for (i = 0; i < MAX_PROJ; i++)
        if (!w->pj[i].alive) {
            memset(&w->pj[i], 0, sizeof w->pj[i]);
            w->pj[i].alive = 1;
            return &w->pj[i];
        }
    return NULL;
}

static void aim(Proj *pj, fx tx, fx ty, fx speed, int spread)
{
    fx vx, vy;
    step_toward(pj->x, pj->y, tx, ty, speed, &vx, &vy);
    /* rotate (vx, vy) by 'spread' 64ths of a turn */
    pj->vx = (fx)(((int64_t)vx * isin(spread + 16) - (int64_t)vy * isin(spread)) / 127);
    pj->vy = (fx)(((int64_t)vx * isin(spread) + (int64_t)vy * isin(spread + 16)) / 127);
}

static void fire_projectiles(World *w, int idx, const SkillRT *s, const Monster *tgt)
{
    int k, n = MIN(s->count, 14);
    for (k = 0; k < n; k++) {
        Proj *pj = spawn_proj(w);
        bool burst = (s->flags & RF_BURST) != 0;
        if (!pj)
            return;
        pj->kind = PJ_SKILL;
        pj->x = pj->px = w->h.x - (burst ? w->h.face * FX_FROM_INT(k * 5) : 0);
        pj->y = pj->py = w->h.y - FX_FROM_INT(4);
        aim(pj, tgt->x, tgt->y, (s->flags & RF_WANDER) ? FX(2.2) : FX(4.0), burst ? 0 : (k - (n - 1) / 2) * 3);
        pj->life = (s->flags & RF_WANDER) ? 90 : 60;
        pj->hit = skill_hit(w, idx, 1.0);
        pj->radius = (int16_t)s->radius;
        pj->pierce = (s->flags & RF_PIERCE) != 0;
        pj->ground = (s->flags & RF_GROUND) != 0;
        pj->explode = (s->flags & RF_EXPLODE) != 0;
        pj->wander = (s->flags & RF_WANDER) != 0;
        pj->vfx = s->vfx;
    }
}

/* ------------------------------------------------------------ chains */

static void chain(World *w, Profile *p, int idx, const SkillRT *s, int first)
{
    uint64_t hit = 0;
    int cur = first, j, i, fxp = hx(w), fyp = hy(w);
    Hit h = skill_hit(w, idx, 1.0);
    for (j = 0; j <= s->count && cur >= 0; j++) {
        Monster *m = &w->mon[cur];
        int next = -1, bd = s->radius + 1;
        hit |= (uint64_t)1 << cur;
        tag(effect(w, FX_BOLT, fxp, fyp, FX_TO_INT(m->x), FX_TO_INT(m->y), 0, 8, element_color((Element)h.element)), s);
        fxp = FX_TO_INT(m->x);
        fyp = FX_TO_INT(m->y);
        deal_damage(w, p, cur, &h);
        for (i = 0; i < w->nmon; i++)
            if (w->mon[i].alive && !((hit >> i) & 1u)) {
                int d = dist_px(FX_FROM_INT(fxp), FX_FROM_INT(fyp), w->mon[i].x, w->mon[i].y);
                if (d < bd) { bd = d; next = i; }
            }
        /* Nobody new in reach: the bolt arcs back into the same foe, weaker. */
        if (next < 0 && m->alive && j < s->count) {
            next = cur;
            h.base *= 0.6;
        }
        cur = next;
    }
}

/* -------------------------------------------------------- behaviours */

static bool cast_buff(World *w, const SkillRT *s)
{
    bool threatened = count_near(w, hx(w), hy(w), 100) >= 1;
    if (w->h.buff_t[s->buff] > 0 || !threatened)
        return false;
    if (s->buff == BUFF_BARRIER) {
        if (w->h.hp > w->st.max_hp * 0.85 && count_near(w, hx(w), hy(w), 40) < 3)
            return false;                 /* save it for when it matters */
        hero_add_barrier(w, s->coef / 100.0);
        hero_buff(w, BUFF_BARRIER, s->coef, s->duration);
    } else if (s->buff == BUFF_IMBUE) {
        hero_buff(w, BUFF_IMBUE, s->coef, s->duration);
        w->h.imbue_el = s->element;
        w->h.imbue_st = s->status;
        w->h.imbue_flags = s->flags;
    } else {
        hero_buff(w, s->buff, s->coef, s->duration);
        if (s->buff == BUFF_ULT)
            hero_buff(w, BUFF_UNSTOP, 0, s->duration);
    }
    if (s->flags & RF_HEAL)
        w->h.hp = MIN(w->st.max_hp, w->h.hp + w->st.max_hp * 0.15);
    tag(effect(w, FX_LEVEL, hx(w), hy(w), 0, 0, 24, 24, s->color), s);
    return true;
}

static bool cast_ground(World *w, int idx, const SkillRT *s)
{
    int x = hx(w), y = hy(w), n, need = s->cat == CAT_CORE ? 1 : 2;
    Hit h = skill_hit(w, idx, 1.0);
    if (s->range > 0 && !(s->flags & RF_FOLLOW)) {
        int pk = densest_pack(w, s->range, s->radius, &n);
        if (pk < 0 || n < need)
            return false;
        x = FX_TO_INT(w->mon[pk].x);
        y = FX_TO_INT(w->mon[pk].y);
    } else if (count_near(w, x, y, s->radius) < need) {
        return false;
    }
    world_spawn_ground(w, x, y, s->radius, s->duration, &h, (s->flags & RF_FOLLOW) != 0, s->vfx);
    return true;
}

static bool cast_strike(World *w, int idx, const SkillRT *s)
{
    int n, pk = densest_pack(w, s->range, s->radius, &n);
    if (pk < 0 || n < (s->cat == CAT_CORE ? 1 : 3) || w->h.strike_t > 0)
        return false;
    w->h.strike_t = s->cat == CAT_CORE ? 8 : 15;
    w->h.strike_hits = s->count;
    w->h.strike_skill = idx;
    w->h.strike_x = FX_TO_INT(w->mon[pk].x);
    w->h.strike_y = FX_TO_INT(w->mon[pk].y);
    tag(effect(w, FX_WARN, w->h.strike_x, w->h.strike_y, hx(w), hy(w), s->radius, w->h.strike_t, s->color), s);
    return true;
}

static bool cast_dash(World *w, int idx, const SkillRT *s)
{
    int n, pk = densest_pack(w, s->range, 30, &n);
    fx tx, ty;
    if (pk < 0 || w->h.dash_t > 0 || !body_line_clear(w, w->h.x, w->h.y, w->mon[pk].x, w->mon[pk].y, HERO_HALF))
        return false;
    tx = w->mon[pk].x;
    ty = w->mon[pk].y;
    w->h.dash_t = 8;
    w->h.dash_skill = idx;
    w->h.dash_hit = 0;
    step_toward(w->h.x, w->h.y, tx, ty, FX_FROM_INT(MAX(dist_px(w->h.x, w->h.y, tx, ty) + 12, 16)) / 8,
                &w->h.dash_vx, &w->h.dash_vy);
    w->h.face = tx > w->h.x ? 1 : -1;
    tag(effect(w, FX_DASH, hx(w), hy(w), FX_TO_INT(tx), FX_TO_INT(ty), s->radius, 10, s->color), s);
    return true;
}

static bool cast_summon(World *w, int idx, const SkillRT *s)
{
    int kind = s->behavior == SB_SUMMON && s->tag == TAG_MINION && (s->flags & RF_ALT) ? AK_MAGE
             : w->cls == CLASS_DRUID ? AK_WOLF : AK_SKELETON;
    int i, have = 0;
    for (i = 0; i < MAX_ALLY; i++)
        have += w->al[i].alive && w->al[i].skill == idx;
    if (have >= MIN(s->count, MAX_ALLY) || !ally_summon(w, kind, s->element))
        return false;
    for (i = MAX_ALLY - 1; i >= 0; i--)
        if (w->al[i].alive && w->al[i].skill == 0xFF) {
            w->al[i].skill = (uint8_t)idx;
            break;
        }
    return true;
}

static bool cast_corpse(World *w, Profile *p, int idx, const SkillRT *s)
{
    int c = corpse_best(w, s->range, s->radius);
    int x, y;
    if (c < 0)
        return false;
    x = FX_TO_INT(w->co[c].x);
    y = FX_TO_INT(w->co[c].y);
    w->co[c].alive = 0;
    tag(effect(w, FX_BOOM, x, y, 0, 0, s->radius, 12, s->color), s);
    area_hit(w, p, x, y, s->radius, idx, 1.0);
    return true;
}

static bool cast_area(World *w, Profile *p, int idx, const SkillRT *s, bool nova)
{
    int x = hx(w), y = hy(w), n = count_near(w, x, y, s->radius);
    if (nova && s->cat == CAT_DEFENSIVE && n < 2 && !(n >= 1 && w->h.hp < w->st.max_hp * 0.6))
        return false;
    if (n < 1)
        return false;
    tag(effect(w, nova || (s->flags & RF_NOVA) ? FX_NOVA : FX_SLASH, x, y, w->h.face, 0, s->radius, nova ? 14 : 10,
               s->color), s);
    area_hit(w, p, x, y, s->radius, idx, 1.0);
    return true;
}

bool cast_skill(World *w, Profile *p, int i, const SkillRT *s, const Monster *tgt)
{
    bool ok;
    switch (s->behavior) {
    case SB_ARC:    ok = cast_area(w, p, i, s, false); break;
    case SB_NOVA:   ok = cast_area(w, p, i, s, true); break;
    case SB_PROJ:
        ok = tgt && dist_px(w->h.x, w->h.y, tgt->x, tgt->y) <= s->range
          && body_line_clear(w, w->h.x, w->h.y - FX_FROM_INT(4), tgt->x, tgt->y, 3);
        if (ok)
            fire_projectiles(w, i, s, tgt);
        break;
    case SB_CHAIN:
        ok = tgt && dist_px(w->h.x, w->h.y, tgt->x, tgt->y) <= MAX(s->range, 24);
        if (ok)
            chain(w, p, i, s, (int)(tgt - w->mon));
        break;
    case SB_CHANNEL:
        ok = w->h.channel_t == 0 && count_near(w, hx(w), hy(w), s->radius) >= 1;
        if (ok) {
            w->h.channel_t = s->duration;
            w->h.channel_skill = i;
        }
        break;
    case SB_STRIKE: ok = cast_strike(w, i, s); break;
    case SB_GROUND: ok = cast_ground(w, i, s); break;
    case SB_BUFF:   ok = cast_buff(w, s); break;
    case SB_SUMMON: ok = cast_summon(w, i, s); break;
    case SB_DASH:   ok = cast_dash(w, i, s); break;
    case SB_CORPSE: ok = cast_corpse(w, p, i, s); break;
    default:        ok = false; break;
    }
    if (ok) {
        static const uint8_t cast_snd[EL_COUNT] = {
            [EL_PHYS] = SND_CAST_PHYS, [EL_FIRE] = SND_CAST_FIRE, [EL_COLD] = SND_CAST_COLD,
            [EL_LIGHT] = SND_CAST_LIGHTNING, [EL_POISON] = SND_CAST_POISON, [EL_SHADOW] = SND_CAST_SHADOW,
        };
        cast_extras(w, s);
        world_sound(w, cast_snd[s->element % EL_COUNT]);
    }
    return ok;
}

/* ------------------------------------------- multi-tick skill states */

static void update_channel(World *w, Profile *p)
{
    const SkillRT *s = &w->st.b.skill[w->h.channel_skill % CLASS_SKILLS];
    if (w->h.channel_t <= 0 || --w->h.channel_t % 9 != 0)
        return;
    /* Channels pay per pulse and stop when the resource runs dry. */
    if (s->cost > 0 && w->h.res < s->cost) {
        w->h.channel_t = 0;
        return;
    }
    if (s->cost > 0)
        w->h.res -= s->cost;
    tag(effect(w, FX_WHIRL, hx(w), hy(w), 0, 0, s->radius, 9, s->color), s);
    area_hit(w, p, hx(w), hy(w), s->radius, w->h.channel_skill, 1.0);
    if (count_near(w, hx(w), hy(w), s->radius + 16) == 0)
        w->h.channel_t = 0;
}

static void update_strike(World *w, Profile *p)
{
    const SkillRT *s = &w->st.b.skill[w->h.strike_skill % CLASS_SKILLS];
    if (w->h.strike_t <= 0 || --w->h.strike_t > 0)
        return;
    tag(effect(w, FX_METEOR, w->h.strike_x, w->h.strike_y, 0, 0, s->radius, 16, s->color), s);
    area_hit(w, p, w->h.strike_x, w->h.strike_y, s->radius, w->h.strike_skill, 1.0);
    if (s->flags & RF_GROUND) {
        Hit h = skill_hit(w, w->h.strike_skill, 0.25);
        if (h.element == EL_FIRE)
            h.status = ST_BURN;
        world_spawn_ground(w, w->h.strike_x, w->h.strike_y, s->radius * 3 / 4, 120, &h, false,
                           h.element == EL_FIRE ? VX_BURN : VX_QUAKE_POOL);
    }
    if (--w->h.strike_hits > 0)
        w->h.strike_t = 10; /* aftershock */
}

static void update_dash(World *w, Profile *p)
{
    const SkillRT *s = &w->st.b.skill[w->h.dash_skill % CLASS_SKILLS];
    Hit h;
    int i;
    if (w->h.dash_t <= 0)
        return;
    w->h.dash_t--;
    move_body(w, &w->h.x, &w->h.y, w->h.dash_vx, w->h.dash_vy, HERO_HALF);
    h = skill_hit(w, w->h.dash_skill, 1.0);
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && !((w->h.dash_hit >> i) & 1u)
            && dist_px(w->h.x, w->h.y, w->mon[i].x, w->mon[i].y) <= s->radius + 6) {
            w->h.dash_hit |= (uint64_t)1 << i;
            deal_damage(w, p, i, &h);
        }
    if (w->h.dash_t == 0 && (s->flags & RF_GROUND)) {
        Hit g = skill_hit(w, w->h.dash_skill, 0.3);
        world_spawn_ground(w, hx(w), hy(w), 24, 90, &g, false, VX_POISON_POOL);
    }
}

void update_channel_strike_dash(World *w, Profile *p)
{
    update_channel(w, p);
    update_strike(w, p);
    update_dash(w, p);
}
