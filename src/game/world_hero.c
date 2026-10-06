/*
 * world_hero.c - the hero plays itself: picks targets, walks the dungeon,
 * spends resource on core skills and builds it back with basic skills
 * (Diablo IV style), fires cooldown skills when useful, drinks potions,
 * loots and takes the stairs.
 */
#include "world_int.h"
#include "events.h"
#include "balance.h"
#include "progress.h"
#include "skills.h"
#include "build.h"
#include "items.h"
#include "aspects.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define PICKUP_RANGE 10
#define DROP_MAGNET (20 * TICK_HZ)   /* loot this old is collected from anywhere (failsafe) */
#define FAR 32000
#define POTION_CD (TICK_HZ * 3 / 2)

static int hx(const World *w) { return FX_TO_INT(w->h.x); }
static int hy(const World *w) { return FX_TO_INT(w->h.y); }

/* --------------------------------------------------------------- targets */

static int mon_path_dist(const World *w, const Monster *m)
{
    return w->fh[px_to_cell(m->y)][px_to_cell(m->x)];
}

static int nearest_monster(const World *w)
{
    int i, best = -1, bd = FAR;
    for (i = 0; i < w->nmon; i++) {
        const Monster *m = &w->mon[i];
        int d;
        if (!m->alive)
            continue;
        d = mon_path_dist(w, m) - (m->aggro ? 3 : 0) - (m->goblin ? 8 : 0);   /* chase the goblin first */
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

static int nearest_drop(const World *w)
{
    int i, best = -1, bd = FAR;
    for (i = 0; i < MAX_DROP; i++)
        if (w->dr[i].alive && w->dr[i].t > TICK_HZ / 2) {   /* let the loot land first */
            int d = w->fh[px_to_cell(w->dr[i].y)][px_to_cell(w->dr[i].x)];
            if (d < bd) {
                bd = d;
                best = i;
            }
        }
    return best;
}

static int object_dist(const World *w)
{
    return events_object_pending(w) ? w->fh[w->ev.cy][w->ev.cx] : FAR;
}

/* Loot first, then a nearby event object, then monsters (the whole event
 * wave even past the quota), then the floor's object, then the stairs. */
static void choose_target(World *w)
{
    int m = nearest_monster(w), d = nearest_drop(w), od = object_dist(w);
    int md = m >= 0 ? mon_path_dist(w, &w->mon[m]) : FAR;
    if (d >= 0 && w->fh[px_to_cell(w->dr[d].y)][px_to_cell(w->dr[d].x)] + 2 < md) {
        w->h.target_kind = TGT_DROP;
        w->h.target_idx = d;
    } else if (od < FAR && (od + 4 < md || (m < 0 || (w->kills >= w->quota && md >= 8)))
               && !events_wave_active(w)) {
        w->h.target_kind = TGT_OBJECT;
        w->h.target_idx = 0;
    } else if (m >= 0 && (w->kills < w->quota || md < 8 || events_wave_active(w))) {
        w->h.target_kind = TGT_MON;
        w->h.target_idx = m;
    } else {
        w->h.target_kind = TGT_STAIRS;
        w->h.target_idx = 0;
    }
}

static void target_pos(const World *w, fx *tx, fx *ty)
{
    switch (w->h.target_kind) {
    case TGT_MON:  *tx = w->mon[w->h.target_idx].x; *ty = w->mon[w->h.target_idx].y; break;
    case TGT_DROP: *tx = w->dr[w->h.target_idx].x; *ty = w->dr[w->h.target_idx].y; break;
    case TGT_OBJECT: *tx = cell_center(w->ev.cx); *ty = cell_center(w->ev.cy); break;
    default:       *tx = cell_center(w->stairs_x); *ty = cell_center(w->stairs_y); break;
    }
}

void hero_walk_to(World *w, fx tx, fx ty)
{
    double move = 1.0 + w->st.move_pct / 100.0 + (hero_has(w, BUFF_BERSERK) ? 0.15 : 0.0);
    fx speed = (fx)(FX(1.6) * move), dx, dy, wx = tx, wy = ty;
    fx ox = w->h.x, oy = w->h.y;
    int cell = px_to_cell(ty) * MAP_W + px_to_cell(tx);
    bool direct = w->h.stuck == 0 && body_line_clear(w, w->h.x, w->h.y, tx, ty, HERO_HALF);
    if (!direct) {
        if (cell != w->ft_target_cell || w->tick % 15 == 0) {
            bfs_field(w, w->ft, px_to_cell(tx), px_to_cell(ty));
            w->ft_target_cell = cell;
        }
        if (!smooth_waypoint(w, w->ft, w->h.x, w->h.y, HERO_HALF, &wx, &wy)) {
            wx = tx; /* already in the target cell */
            wy = ty;
        }
    }
    step_toward(w->h.x, w->h.y, wx, wy, speed, &dx, &dy);
    if (dx)
        w->h.face = dx > 0 ? 1 : -1;
    move_body(w, &w->h.x, &w->h.y, dx, dy, HERO_HALF);
    w->h.moving = true;
    /* Blocked anyway? Follow the grid path for a while. */
    if (w->h.x == ox && w->h.y == oy)
        w->h.stuck = 30;
    else if (w->h.stuck > 0)
        w->h.stuck--;
}

/* ------------------------------------------------------------- attacks */

static double hero_aps(const World *w)
{
    double aps = w->st.aps * (1.0 + 0.04 * w->h.stacks);
    if (w->h.haste_t > 0)
        aps *= 1.20;
    if (hero_has(w, BUFF_SPEED))
        aps *= 1.0 + w->h.buff_val[BUFF_SPEED] / 100.0;
    if (hero_has(w, BUFF_ULT))
        aps *= 1.25;
    if (world_shrine(w, SH_FRENZY))
        aps *= 1.5;
    return MIN(aps, 6.0);
}

/* Can this primary skill do something right now? */
static bool in_reach(const World *w, const SkillRT *s, const Monster *tgt)
{
    int n, d = tgt ? dist_px(w->h.x, w->h.y, tgt->x, tgt->y) : FAR;
    switch (s->behavior) {
    case SB_PROJ:
    case SB_CHAIN:   return d <= MAX(s->range, 24);
    case SB_ARC:
    case SB_NOVA:
    case SB_CHANNEL: return count_near(w, hx(w), hy(w), s->radius) >= 1;
    case SB_STRIKE:
    case SB_GROUND:  return densest_pack(w, s->range > 0 ? s->range : s->radius, s->radius, &n) >= 0 && n >= 1;
    case SB_DASH:    return d <= s->range;
    default:         return false;
    }
}

/* Core skills while the resource lasts, basic skills to build it back. */
static int choose_primary(const World *w, const Monster *tgt)
{
    const BuildRT *b = &w->st.b;
    int cat, j;
    for (cat = CAT_CORE; cat >= CAT_BASIC; cat--)
        for (j = 0; j < CLASS_SKILLS; j++) {
            const SkillRT *s = &b->skill[j];
            if (!s->usable || s->cat != cat || (s->cost > 0 && w->h.res < s->cost))
                continue;
            if (s->behavior == SB_CHANNEL && w->h.channel_t > 0)
                continue;
            if (in_reach(w, s, tgt))
                return j;
        }
    return -1;
}

static void weapon_swing(World *w, Profile *p, int mi)
{
    Hit h = { 0 };
    h.base = w->st.weapon * 0.5;
    h.element = EL_PHYS;
    h.skill = NO_SKILL;
    h.lucky = 0.2;
    if (dist_px(w->h.x, w->h.y, w->mon[mi].x, w->mon[mi].y) <= 26)
        deal_damage(w, p, mi, &h);
}

void hero_primary_attack(World *w, Profile *p, int mi)
{
    const Monster *m = &w->mon[mi];
    int idx = choose_primary(w, m);
    w->h.attack_t = 8;
    w->h.atk_cd = MAX(3, (int)(TICK_HZ / hero_aps(w)));
    w->h.face = m->x > w->h.x ? 1 : -1;
    if (idx >= 0 && cast_skill(w, p, idx, &w->st.b.skill[idx], m)) {
        const SkillRT *s = &w->st.b.skill[idx];
        if (s->cost > 0 && s->behavior != SB_CHANNEL)
            w->h.res -= s->cost;
        else if (s->cost < 0)
            hero_gain_res(w, -s->cost);
        return;
    }
    weapon_swing(w, p, mi);
}

/* Defensive, mastery and ultimate skills fire on their own cooldowns
 * (autocast); under manual control they only count down. */
void hero_cooldowns(World *w, Profile *p, const Monster *tgt, bool autocast)
{
    int s;
    for (s = 0; s < CLASS_SKILLS; s++) {
        const SkillRT *rt = &w->st.b.skill[s];
        if (w->h.skill_cd[s] > 0) {
            w->h.skill_cd[s]--;
            continue;
        }
        if (!autocast || !rt->usable || rt->cat <= CAT_CORE)
            continue;
        if (rt->cost > 0 && w->h.res < rt->cost)
            continue;
        if (cast_skill(w, p, s, rt, tgt)) {
            w->h.skill_cd[s] = rt->cd_ticks;
            if (rt->cost > 0)
                w->h.res -= rt->cost;
        }
    }
    update_channel_strike_dash(w, p);
}

/* ------------------------------------------------------------- the rest */

void hero_pick_up(World *w, Profile *p, int di)
{
    Drop *d = &w->dr[di];
    char name[64], buf[96];
    double g;
    bool learned = d->item.rarity == RAR_LEGEND && !codex_known(p, d->item.power);
    LootResult r = prog_handle_loot(p, &d->item, &g);
    item_name(name, sizeof name, &d->item);
    tjoin(buf, sizeof buf, r == LOOT_EQUIPPED ? "EQUIPPED" : r == LOOT_BAGGED ? "" : "SALVAGED", name);
    if (r != LOOT_SALVAGED || d->item.rarity >= RAR_RARE)
        world_message(w, buf, rarity_color((Rarity)d->item.rarity));
    if (learned)
        floater(w, hx(w), hy(w) - 26, "NEW ASPECT IN CODEX", RGB565(255, 140, 30));
    if (r == LOOT_EQUIPPED)
        world_refresh_stats(w, p);
    d->alive = 0;
    w->h.idle_ticks = 0;
}

void hero_act_on_target(World *w, Profile *p)
{
    fx tx, ty;
    int d;
    target_pos(w, &tx, &ty);
    d = dist_px(w->h.x, w->h.y, tx, ty);
    if (w->h.target_kind == TGT_MON) {
        /* Ranged attacks need a clear lane for the projectile itself, or
         * arrows would clip a wall corner forever. */
        bool clear = w->h.engage <= 30 ? line_of_sight(w, w->h.x, w->h.y, tx, ty)
                                       : body_line_clear(w, w->h.x, w->h.y - FX_FROM_INT(4), tx, ty, 3);
        if (d <= w->h.engage && clear) {
            if (w->h.atk_cd == 0 && w->h.channel_t == 0)
                hero_primary_attack(w, p, w->h.target_idx);
            return;
        }
    } else if (w->h.target_kind == TGT_DROP && (d <= PICKUP_RANGE || w->dr[w->h.target_idx].t > DROP_MAGNET)) {
        hero_pick_up(w, p, w->h.target_idx);
        w->h.target_kind = TGT_NONE;
        return;
    } else if (w->h.target_kind == TGT_OBJECT && d <= 10) {
        events_touch(w, p);
        w->h.target_kind = TGT_NONE;
        return;
    } else if (w->h.target_kind == TGT_STAIRS && d <= 6) {
        w->ev_floor_done = true;
        world_sound(w, SND_STAIRS);
        return;
    }
    if (w->h.channel_t == 0 || w->h.target_kind != TGT_MON)
        hero_walk_to(w, tx, ty);
    else if (d > 12)                     /* whirlwind keeps moving into the pack */
        hero_walk_to(w, tx, ty);
}

static bool target_valid(const World *w)
{
    switch (w->h.target_kind) {
    case TGT_MON:    return w->mon[w->h.target_idx].alive;
    case TGT_DROP:   return w->dr[w->h.target_idx].alive;
    case TGT_STAIRS: return true;
    case TGT_OBJECT: return events_object_pending(w);
    default:         return false;
    }
}

static void tick_hero_timers(World *w)
{
    HeroRT *h = &w->h;
    int b;
    h->moving = false;
    h->anim++;
    if (h->flash > 0) h->flash--;
    if (h->attack_t > 0) h->attack_t--;
    if (h->cast_t > 0) h->cast_t--;
    if (h->atk_cd > 0) h->atk_cd--;
    if (h->potion_cd > 0) h->potion_cd--;
    if (h->haste_t > 0) h->haste_t--;
    for (b = 0; b < BUFF_COUNT; b++)
        if (h->buff_t[b] > 0) h->buff_t[b]--;
    if (h->barrier_t > 0 && --h->barrier_t == 0)
        h->barrier = 0;
    if (h->stacks_t > 0 && --h->stacks_t == 0)
        h->stacks = 0;
}

/* Automatic below 40% life; 'force' (the potion key) drinks at any loss. */
void hero_drink_potion(World *w, bool force)
{
    if (w->h.potion_cd > 0 || w->h.potions <= 0 || w->h.hp >= w->st.max_hp * (force ? 1.0 : 0.4))
        return;
    w->h.hp = MIN(w->st.max_hp, w->h.hp + w->st.max_hp * w->st.potion_heal);
    w->h.potions--;
    w->h.potion_cd = POTION_CD;
    world_sound(w, SND_POTION);
    effect(w, FX_HEAL, hx(w), hy(w), 0, 0, 14, 20, RGB565(255, 80, 90));
    floater(w, hx(w), hy(w) - 16, "POTION", RGB565(255, 120, 130));
}

void hero_update(World *w, Profile *p)
{
    HeroRT *h = &w->h;
    tick_hero_timers(w);
    if (h->dead_t > 0) {
        if (--h->dead_t == 0)
            w->ev_died = true;
        return;
    }
    h->hp = MIN(w->st.max_hp, h->hp + w->st.regen / TICK_HZ);
    hero_gain_res(w, w->st.res_regen / TICK_HZ);
    hero_drink_potion(w, false);
    if (w->cmd.active)
        w->ctl.manual_t = MANUAL_HOLD;
    if (hero_manual(w, p)) {
        h->idle_ticks = 0;                    /* the player is in charge, no failsafe */
        control_update(w, p);
        return;
    }
    /* Failsafe: if nothing was killed or looted for 45 s, the session
     * regenerates the floor. */
    if (++h->idle_ticks > 45 * TICK_HZ) {
        w->ev_stuck = true;
        return;
    }
    /* Re-plan every 10 ticks, but once walking to loot finish the trip: two
     * drops at similar path lengths would otherwise swap forever. */
    if (!target_valid(w) || (w->tick % 10 == 0 && h->target_kind != TGT_DROP))
        choose_target(w);
    hero_cooldowns(w, p, h->target_kind == TGT_MON ? &w->mon[h->target_idx] : NULL, true);
    if (h->dash_t == 0)
        hero_act_on_target(w, p);
}
