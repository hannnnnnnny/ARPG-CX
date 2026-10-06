/*
 * world_dmg.c - the damage pipeline (Diablo IV style buckets), status
 * effects, lucky hits, damage numbers and kill rewards.
 */
#include "world_int.h"
#include "balance.h"
#include "progress.h"
#include "items.h"
#include "build.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include "../gfx/gfx.h"
#include <stdio.h>
#include <string.h>

#define DOT_TICKS 90         /* DoTs last 3 s unless the skill says otherwise */
#define DOT_EVERY 10         /* and pulse 3x per second */
#define VULN_TICKS 90        /* Vulnerable lasts 3 s */

#define C_NORMAL RGB565(240, 240, 240)
#define C_CRIT   RGB565(255, 220, 50)
#define C_VULN   RGB565(200, 110, 255)
#define C_VCRIT  RGB565(255, 100, 210)
#define C_OP     RGB565(90, 220, 255)
#define C_OPCRIT RGB565(255, 150, 40)

static int dot_index(int status) { return status - ST_BURN; }
static int dot_element(int k)
{
    static const uint8_t el[DOT_KINDS] = { EL_FIRE, EL_POISON, EL_PHYS, EL_SHADOW };
    return el[k % DOT_KINDS];
}

/* ------------------------------------------------------------- buckets */

static double additive(const World *w, const Monster *m, const Hit *h)
{
    const BuildRT *b = &w->st.b;
    bool close = dist_px(w->h.x, w->h.y, m->x, m->y) <= CLOSE_PX;
    double a = b->add[ADD_ALL] + b->add[ADD_ELEM0 + h->element % EL_COUNT];
    a += close ? b->add[ADD_CLOSE] : b->add[ADD_FAR];
    if (mon_cc(m)) a += b->add[ADD_CC];
    if (m->elite || m->boss) a += b->add[ADD_ELITE];
    if (h->dot) a += b->add[ADD_DOT];
    if (h->skill < CLASS_SKILLS) {
        const SkillRT *s = &b->skill[h->skill];
        if (s->cat == CAT_CORE) a += b->add[ADD_CORE];
        if (s->cat == CAT_BASIC) a += b->add[ADD_BASIC];
        a += b->add[ADD_TAG0 + s->tag % TAG_COUNT];
    }
    return a;
}

static double key_mult(const World *w, const Monster *m, const Hit *h)
{
    const BuildRT *b = &w->st.b;
    const SkillRT *s = h->skill < CLASS_SKILLS ? &b->skill[h->skill] : NULL;
    double x = 1.0;
    switch (b->key) {
    case KP_BERSERKER:  if (hero_has(w, BUFF_BERSERK)) x = 1.25; break;
    case KP_HEMORRHAGE: if (m->dot_t[dot_index(ST_BLEED)] > 0 && s && s->cat == CAT_CORE) x = 1.30; break;
    case KP_COMBUSTION: if (h->dot && h->element == EL_FIRE) x = 1.20; break;
    case KP_SHATTER:    if (m->freeze > 0) x = 1.60; break;
    case KP_VIRULENCE:  if (h->dot && h->element == EL_POISON) x = 2.0; break;
    case KP_BLOODBOND:  if (s && s->tag == TAG_BLOOD && w->h.hp >= w->st.max_hp * 0.8) x = 1.30; break;
    case KP_FEROCITY:   x = 1.0 + 0.06 * w->h.stacks; break;
    case KP_PLAGUE:
        x = m->dot_t[dot_index(ST_POISON)] > 0 ? 1.20 : 1.0;
        if (h->dot && h->element == EL_POISON) x *= 1.5;
        break;
    default: break;
    }
    return x;
}

static double multipliers(const World *w, const Monster *m, const Hit *h)
{
    const BuildRT *b = &w->st.b;
    double x = b->x_all * b->x_elem[h->element % EL_COUNT];
    if (mon_cc(m)) x *= b->x_cc;
    if (h->dot) x *= b->x_dot;
    if (h->minion) x *= b->x_minion;
    if (m->vuln > 0) x *= b->x_vuln;
    if (h->skill < CLASS_SKILLS) {
        const SkillRT *s = &b->skill[h->skill];
        if (s->cat == CAT_CORE) x *= b->x_core;
        x *= b->x_skill[h->skill] * b->x_tag[s->tag % TAG_COUNT];
    }
    if (hero_has(w, BUFF_BERSERK)) x *= 1.0 + w->h.buff_val[BUFF_BERSERK] / 100.0;
    if (hero_has(w, BUFF_ULT)) x *= 1.0 + w->h.buff_val[BUFF_ULT] / 100.0;
    return x * key_mult(w, m, h);
}

static double crit_chance(const World *w, const Monster *m, const Hit *h)
{
    double c = w->st.crit + h->crit_add;
    if (hero_has(w, BUFF_CRIT))
        c += w->h.buff_val[BUFF_CRIT] / 100.0;
    if (has_key(&w->st.b, KP_DEADEYE) && m->hp >= m->max_hp * 0.8)
        c += 0.30;
    return MIN(c, 0.95);
}

static bool roll(World *w, double chance)
{
    return (int)(rng_next(&w->rng) % 10000u) < (int)(chance * 10000.0);
}

/* ------------------------------------------------------------- statuses */

static void apply_dot(World *w, Monster *m, const Hit *h)
{
    int k = dot_index(h->status), ticks = h->status_dur > 0 ? h->status_dur : DOT_TICKS;
    /* 25% of the hit's base per second; the pipeline applies bonuses per pulse. */
    double dps = h->base * 0.25;
    (void)w;
    if (dps > m->dot_dps[k] || m->dot_t[k] < DOT_TICKS / 3)
        m->dot_dps[k] = dps;
    m->dot_t[k] = (int16_t)MAX(m->dot_t[k], ticks);
}

static void apply_status(World *w, Monster *m, const Hit *h)
{
    double cc = 1.0 + w->st.cc_dur;
    int d = (int)((h->status_dur ? h->status_dur : 30) * cc);
    switch (h->status) {
    case ST_FREEZE:     m->freeze = (int16_t)MAX(m->freeze, m->boss ? 10 : (h->status_dur ? d : (int)(45 * cc))); break;
    case ST_STUN:       m->stun = (int16_t)MAX(m->stun, m->boss ? d / 3 : d); break;
    case ST_CHILL:      m->chill = (int16_t)MAX(m->chill, h->status_dur ? d : (int)(90 * cc)); break;
    case ST_IMMOBILIZE: m->immob = (int16_t)MAX(m->immob, m->boss ? d / 2 : d); break;
    default:
        if (status_is_dot(h->status))
            apply_dot(w, m, h);
        break;
    }
}

static void make_vulnerable(World *w, Monster *m)
{
    if (m->vuln == 0 && w->dmg_numbers == DMGNUM_ALL && (w->tick & 3) == 0)
        floater(w, FX_TO_INT(m->x), FX_TO_INT(m->y) - 20, "VULNERABLE", C_VULN);
    m->vuln = VULN_TICKS;
}

static void lucky_hit(World *w, Monster *m, const Hit *h)
{
    const Stats *st = &w->st;
    if (!roll(w, h->lucky + st->lucky))
        return;
    if (st->b.lh_vuln > 0 && roll(w, st->b.lh_vuln / 100.0))
        make_vulnerable(w, m);
    if (st->b.lh_stun > 0 && roll(w, st->b.lh_stun / 100.0))
        m->stun = (int16_t)MAX(m->stun, m->boss ? 6 : 18);
    if (st->b.lh_res > 0)
        hero_gain_res(w, st->b.lh_res);
}

/* ------------------------------------------------------ damage numbers */

static void damage_floater(World *w, const Monster *m, double dmg, bool crit, bool vuln, bool op, const Hit *h)
{
    char buf[16];
    uint16_t c;
    int x = FX_TO_INT(m->x) + rng_range(&w->rng, -5, 5), y = FX_TO_INT(m->y) - (h->dot ? 6 : 12);
    if (w->dmg_numbers == DMGNUM_OFF || (w->dmg_numbers == DMGNUM_BIG && !crit && !op && !m->boss))
        return;
    fmt_num(buf, sizeof buf, dmg);
    if (h->dot)
        c = h->element == EL_PHYS ? RGB565(230, 60, 60) : element_color((Element)h->element);
    else if (op)
        c = crit ? C_OPCRIT : C_OP;
    else if (crit)
        c = vuln ? C_VCRIT : C_CRIT;
    else
        c = vuln ? C_VULN : C_NORMAL;
    floater_kind(w, x, y, buf, c, op ? FL_BIG : FL_DMG);
    if (op)
        floater(w, x, y - 12, "OVERPOWER", C_OP);
}

static void log_hit(World *w, const HitLog *l)
{
    HitStats *s = &w->hs;
    s->hits += 1;
    s->crits += l->is_crit;
    s->vulns += l->is_vuln;
    s->ops += l->is_op;
    s->total += l->total;
    if (l->total >= w->last_big.total)
        w->last_big = *l;
}

/* ------------------------------------------------------------- pipeline */

static double compute(World *w, const Monster *m, const Hit *h, HitLog *l)
{
    const Stats *st = &w->st;
    double dmg;
    l->base = h->base;
    l->stat = st->stat_mult;
    l->add = 1.0 + additive(w, m, h) / 100.0;
    l->mult = multipliers(w, m, h);
    l->is_vuln = m->vuln > 0;
    l->vuln = l->is_vuln ? 1.2 + st->vuln_dmg : 1.0;
    l->is_crit = !h->dot && roll(w, crit_chance(w, m, h));
    l->crit = l->is_crit ? (1.5 + st->crit_dmg) * st->b.x_crit : 1.0;
    if (l->is_crit && h->element == EL_LIGHT && has_key(&st->b, KP_OVERCHARGE))
        l->crit = 1.0 + (0.5 + st->crit_dmg) * 3.0 * st->b.x_crit;
    l->is_op = !h->dot && !h->minion && roll(w, st->op_chance + h->op_add);
    dmg = h->base;
    if (l->is_op)   /* overpower adds your current life and barrier to the hit */
        dmg += (w->h.hp + w->h.barrier) * 0.2;
    l->op = l->is_op ? (1.5 + st->op_dmg) * st->b.x_op : 1.0;
    dmg *= l->stat * l->add * l->mult * l->vuln * l->crit * l->op;
    if (!h->dot)
        dmg *= 0.9 + 0.2 * (double)(rng_next(&w->rng) % 1000u) / 1000.0;
    l->total = dmg;
    l->skill = h->skill;
    l->element = h->element;
    return dmg;
}

static void on_hit_effects(World *w, Monster *m, const Hit *h)
{
    const Stats *st = &w->st;
    if (h->flags & RF_VULN)
        make_vulnerable(w, m);
    apply_status(w, m, h);
    lucky_hit(w, m, h);
    if (st->life_hit > 0 && !h->minion)
        w->h.hp = MIN(st->max_hp, w->h.hp + st->life_hit);
    if (h->skill < CLASS_SKILLS && (st->b.skill[h->skill].flags & RF_HEAL) && st->b.skill[h->skill].behavior
        == SB_CHANNEL)
        w->h.hp = MIN(st->max_hp, w->h.hp + st->max_hp * 0.004);
    if (has_key(&st->b, KP_BLOODBOND) && h->skill < CLASS_SKILLS && st->b.skill[h->skill].tag == TAG_BLOOD)
        w->h.hp = MIN(st->max_hp, w->h.hp + st->max_hp * 0.03 / 4);
    if (has_key(&st->b, KP_BERSERKER) && w->last_big.is_crit && !h->dot)
        hero_buff(w, BUFF_BERSERK, MAX(w->h.buff_val[BUFF_BERSERK], 25), 30);
}

void deal_damage(World *w, Profile *p, int i, const Hit *h)
{
    Monster *m = &w->mon[i];
    HitLog l;
    double dmg;
    if (!m->alive)
        return;
    dmg = compute(w, m, h, &l);
    m->hp -= dmg;
    m->aggro = 1;
    if (!h->dot) {
        m->flash = 2;
        log_hit(w, &l);
        on_hit_effects(w, m, h);
    }
    if (!h->dot || (w->tick % 30) < DOT_EVERY)
        damage_floater(w, m, dmg, l.is_crit, l.is_vuln, l.is_op, h);
    if (m->hp <= 0)
        kill_rewards(w, p, m);
}

void tick_dots(World *w, Profile *p, int i)
{
    Monster *m = &w->mon[i];
    int k;
    for (k = 0; k < DOT_KINDS && m->alive; k++) {
        if (m->dot_t[k] <= 0)
            continue;
        m->dot_t[k]--;
        if (m->dot_t[k] % DOT_EVERY == 0) {
            Hit h = { 0 };
            h.base = m->dot_dps[k] * DOT_EVERY / TICK_HZ;
            h.element = (uint8_t)dot_element(k);
            h.skill = NO_SKILL;
            h.dot = true;
            deal_damage(w, p, i, &h);
        }
    }
}

/* ------------------------------------------------------------ the hero */

void hurt_hero(World *w, Profile *p, double raw, int element, int attacker)
{
    const Stats *st = &w->st;
    double d = raw * (1.0 - st->dr), armor = stats_damage_reduction(st, w->floor);
    char buf[16];
    if (w->h.dead_t > 0)
        return;
    armor = 1.0 - (1.0 - armor) / (1.0 - st->dr);          /* armor part only */
    d *= element == EL_PHYS ? 1.0 - armor : (1.0 - stats_resist(st, (Element)element)) * (1.0 - armor * 0.5);
    if (attacker >= 0 && dist_px(w->h.x, w->h.y, w->mon[attacker].x, w->mon[attacker].y) <= CLOSE_PX)
        d *= 1.0 - st->dr_close;
    if (hero_has(w, BUFF_UNSTOP))
        d *= 1.0 - w->h.buff_val[BUFF_UNSTOP] / 100.0;
    if (w->h.barrier > 0) {
        double a = MIN(w->h.barrier, d);
        w->h.barrier -= a;
        d -= a;
    }
    w->h.hp -= d;
    if (d >= st->max_hp * 0.05)
        w->h.flash = 2; /* small chip damage doesn't strobe the hero */
    if (d > 0 && w->dmg_numbers != DMGNUM_OFF) {
        fmt_num(buf, sizeof buf, d);
        floater(w, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y) - 18, buf, RGB565(255, 70, 70));
    }
    if (attacker >= 0 && st->thorns > 0) {
        Hit h = { 0 };
        h.base = st->thorns;
        h.element = EL_PHYS;
        h.skill = NO_SKILL;
        h.dot = true;                         /* thorns never crit */
        deal_damage(w, p, attacker, &h);
    }
    if (w->h.hp <= 0) {
        w->h.hp = 0;
        w->h.dead_t = 2 * TICK_HZ;
        world_message(w, "YOU HAVE FALLEN... RETREATING", RGB565(255, 80, 80));
    }
}

/* ------------------------------------------------------------ rewards */

static void announce_drop(World *w, const Item *it)
{
    char buf[80];
    if (it->rarity < RAR_UNIQUE && !it->ancestral)
        return;
    tjoin(buf, sizeof buf - 2, it->ancestral && it->rarity != RAR_MYTHIC ? "ANCESTRAL" : "",
          rarity_name((Rarity)it->rarity));
    strcat(buf, "!");
    world_banner(w, buf, rarity_color((Rarity)it->rarity));
}

static void drop_item(World *w, const Profile *p, const Monster *m)
{
    int i, chance = m->boss ? 100 : m->elite ? 40 : 7;
    Rarity min = m->boss ? RAR_RARE : m->elite ? RAR_MAGIC : RAR_COMMON;
    int count = m->boss ? 3 : 1, k;
    for (k = 0; k < count; k++) {
        if (rng_range(&w->rng, 0, 99) >= chance)
            return;
        for (i = 0; i < MAX_DROP; i++)
            if (!w->dr[i].alive) {
                Drop *d = &w->dr[i];
                d->alive = 1;
                d->t = 0;
                d->x = m->x + FX_FROM_INT(rng_range(&w->rng, -8, 8));
                d->y = m->y + FX_FROM_INT(rng_range(&w->rng, -8, 8));
                item_roll(&d->item, &w->rng, w->floor, p->up[UP_FORTUNE] + (m->boss ? 6 : m->elite ? 2 : 0), min,
                          p->cls);
                announce_drop(w, &d->item);
                break;
            }
    }
}

static void drop_gem(World *w, Profile *p, const Monster *m)
{
    int chance = m->boss ? 100 : m->elite ? 15 : 3, kind, tier;
    char buf[24];
    if (rng_range(&w->rng, 0, 99) >= chance)
        return;
    kind = rng_range(&w->rng, 0, GEM_KINDS - 1);
    tier = CLAMP(w->floor / 12 - rng_range(&w->rng, 0, 1), 0, GEM_TIERS - 1);
    p->gems[kind][tier]++;
    snprintf(buf, sizeof buf, "+%s", gem_name(gem_id(kind, tier)));
    floater(w, FX_TO_INT(m->x), FX_TO_INT(m->y) - 24, buf, gem_color(kind));
}

static void combust(World *w, Profile *p, const Monster *m)
{
    int i;
    Hit h = { 0 };
    Effect *e;
    h.base = w->st.weapon * 0.8;
    h.element = EL_FIRE;
    h.status = ST_BURN;
    h.skill = NO_SKILL;
    e = effect(w, FX_BOOM, FX_TO_INT(m->x), FX_TO_INT(m->y), 0, 0, 24, 10, element_color(EL_FIRE));
    if (e)
        e->vfx = VX_CORPSE_BOOM;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && dist_px(m->x, m->y, w->mon[i].x, w->mon[i].y) <= 24)
            deal_damage(w, p, i, &h);
}

static void kill_buffs(World *w, Profile *p, const Monster *m)
{
    const Stats *st = &w->st;
    bool burning = m->dot_t[dot_index(ST_BURN)] > 0;
    if (has_key(&st->b, KP_FEROCITY)) {
        w->h.stacks = MIN(w->h.stacks + 1, 5);
        w->h.stacks_t = 150;
    }
    if (st->heal_kill > 0 || st->life_kill > 0)
        w->h.hp = MIN(st->max_hp, w->h.hp + st->max_hp * st->heal_kill + st->life_kill);
    if (st->res_kill > 0)
        hero_gain_res(w, st->res_kill);
    if (burning && has_key(&st->b, KP_COMBUSTION))
        combust(w, p, m);
}

static void level_up_message(World *w, Profile *p, int levels)
{
    char buf[80];
    world_refresh_stats(w, p);
    w->h.hp = w->st.max_hp;
    if (p->level >= LEVEL_CAP && p->paragon_level > 0)
        snprintf(buf, sizeof buf, T("PARAGON LEVEL %d"), p->paragon_level);
    else
        snprintf(buf, sizeof buf, T("LEVEL UP! NOW LEVEL %d"), p->level);
    world_message(w, buf, RGB565(190, 140, 255));
    effect(w, FX_LEVEL, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y), 0, 0, 20, 24, RGB565(190, 140, 255));
    (void)levels;
}

void kill_rewards(World *w, Profile *p, Monster *m)
{
    double mult = m->boss ? 25.0 : m->elite ? 3.0 : 1.0;
    int levels;
    m->alive = 0;
    w->kills++;
    w->minute_kills++;
    w->h.idle_ticks = 0;
    p->total_kills += 1;
    prog_add_gold(p, kill_gold(w->floor) * mult * (1.0 + w->st.gold_pct / 100.0));
    levels = prog_add_xp(p, kill_xp(w->floor) * mult * (1.0 + w->st.xp_pct / 100.0));
    effect(w, FX_PUFF, FX_TO_INT(m->x), FX_TO_INT(m->y), 0, 0, m->boss ? 20 : 8, 10, RGB565(200, 200, 200));
    corpse_add(w, m->x, m->y);
    p->souls += m->boss ? 5 : m->elite ? 1 : 0;   /* forgotten souls for the blacksmith */
    drop_item(w, p, m);
    drop_gem(w, p, m);
    if (rng_range(&w->rng, 0, 99) < (m->boss ? 100 : m->elite ? 30 : 4))
        orb_drop(w, m->x, m->y);
    kill_buffs(w, p, m);
    if (levels > 0)
        level_up_message(w, p, levels);
}
