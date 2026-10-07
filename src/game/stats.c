#include "stats.h"
#include "goals.h"
#include "balance.h"
#include "items.h"
#include "paragon.h"
#include <math.h>
#include <string.h>

/* Passive resource regeneration per second, by ResourceKind. */
static const double res_regen_base[RES_COUNT] = { 0.0, 7.0, 10.0, 1.5, 0.0, 0.0 };

static void add_gear(BuildRT *b, double *weapon, double *armor, const Profile *p, const Item *cand, int cslot)
{
    Mod mods[24];
    int s, i, n;
    for (s = 0; s < SLOT_COUNT; s++) {
        const Item *it = (cand && s == cslot) ? cand : &p->equip[s];
        if (!it->used)
            continue;
        if (s == SLOT_WEAPON)
            *weapon += it->main * item_mw_mult(it);
        else
            *armor += it->main * item_mw_mult(it);
        n = item_mods(it, p->cls, mods, 24);
        for (i = 0; i < n; i++)
            build_add_mod(b, mods[i].kind, mods[i].arg, mods[i].value);
    }
}

static void add_elixir_and_upgrades(BuildRT *b, const Profile *p)
{
    if (p->elixir_secs > 0)
        switch (p->elixir) {
        case ELIX_FORTITUDE: build_add_mod(b, MOD_LIFE_PCT, 0, 10); break;
        case ELIX_PRECISION: build_add_mod(b, MOD_CRIT_DMG, 0, 15); break;
        case ELIX_ADVANTAGE: build_add_mod(b, MOD_ATK_SPD, 0, 8); break;
        case ELIX_IRONBARB:  build_add_mod(b, MOD_DR, 0, 5); build_add_mod(b, MOD_THORNS_PCT, 0, 25); break;
        case ELIX_WISDOM:    build_add_mod(b, MOD_XP, 0, 10); break;
        default: break;
        }
    /* MIGHT compounds (x1.12 a rank) so embers stay worth earning forever. */
    build_add_mod(b, MOD_X_ALL, 0, 100.0 * (pow(1.12, p->up[UP_MIGHT]) - 1.0));
    build_add_mod(b, MOD_LIFE_PCT, 0, 15.0 * p->up[UP_VIGOR]);
    build_add_mod(b, MOD_GOLD, 0, 20.0 * p->up[UP_GREED]);
    build_add_mod(b, MOD_XP, 0, 20.0 * p->up[UP_WISDOM]);
    build_add_mod(b, MOD_ATK_SPD, 0, 5.0 * p->up[UP_HASTE]);
    build_add_mod(b, MOD_MOVE, 0, 5.0 * p->up[UP_HASTE]);
    if (paragon_mastery(p->paragon_level) > 0) {
        double m = 100.0 * (pow(1.0 + MASTERY_PCT / 100.0, paragon_mastery(p->paragon_level)) - 1.0);
        build_add_mod(b, MOD_X_ALL, 0, m);
        build_add_mod(b, MOD_LIFE_PCT, 0, m);
    }
    if (renown_tier(p) > 0) {
        build_add_mod(b, MOD_X_ALL, 0, RENOWN_DMG_PCT * renown_tier(p));
        build_add_mod(b, MOD_LIFE_PCT, 0, RENOWN_DMG_PCT * renown_tier(p));
    }
}

static void core_stats(Stats *st, const Profile *p)
{
    const ClassDef *c = &class_defs[p->cls % CLASS_COUNT];
    const BuildRT *b = &st->b;
    int s, lv = MIN(p->level, LEVEL_CAP);
    for (s = 0; s < STAT_COUNT; s++)
        st->stat[s] = (s == c->main_stat ? 10.0 + 5.0 * lv : 8.0 + 2.0 * lv) + b->stat[s];
    st->stat[c->main_stat] += b->mainstat_flat;
    st->mainstat = st->stat[c->main_stat];
    st->stat_mult = 1.0 + st->mainstat / 1000.0;
    st->aps = MIN(4.0, 1.2 * (1.0 + b->atk_spd / 100.0));
    st->crit = MIN(0.80, 0.05 + b->crit / 100.0 + st->stat[STAT_DEX] * 0.0002);
    st->crit_dmg = b->crit_dmg / 100.0;
    st->vuln_dmg = b->vuln_dmg / 100.0;
    st->op_chance = MIN(0.6, 0.03 + b->op_chance / 100.0);
    st->op_dmg = (b->op_dmg + st->stat[STAT_WIL] * 0.1) / 100.0;
    st->lucky = b->lucky / 100.0;
}

static void defence_stats(Stats *st, const Profile *p, double armor)
{
    const ClassDef *c = &class_defs[p->cls % CLASS_COUNT];
    const BuildRT *b = &st->b;
    int e, lv = MIN(p->level, LEVEL_CAP);
    st->max_hp = (80.0 + 12.0 * lv + b->life) * (1.0 + b->life_pct / 100.0) * c->hp_mult * b->x_life;
    st->armor = (armor + b->armor) * (1.0 + (b->armor_pct + st->stat[STAT_STR] * 0.1) / 100.0);
    st->dr = MIN(b->dr, 80.0) / 100.0;
    st->dr_close = MIN(b->dr_close, 50.0) / 100.0;
    for (e = 0; e < EL_COUNT; e++)
        st->res[e] = MIN(70.0, b->res_all + b->res[e] + st->stat[STAT_INT] * 0.05) / 100.0;
    st->regen = st->max_hp * 0.004 + b->regen;
    st->life_hit = b->life_hit;
    st->life_kill = b->life_kill;
    st->heal_kill = b->heal_kill / 100.0;
    st->barrier_gen = b->barrier_gen / 100.0;
    st->thorns = (b->thorns + st->armor * 0.05) * (1.0 + b->thorns_pct / 100.0);
}

static void utility_stats(Stats *st, const Profile *p)
{
    const ClassDef *c = &class_defs[p->cls % CLASS_COUNT];
    const BuildRT *b = &st->b;
    st->max_res = 100.0 + b->max_res;
    st->res_regen = res_regen_base[c->res_kind % RES_COUNT] * (1.0 + b->res_gen / 100.0);
    st->res_kill = b->res_kill;
    st->cdr = MIN(b->cdr, 60.0);
    st->move_pct = MIN(80.0, b->move + (p->cls == CLASS_ROGUE ? 10.0 : 0.0));
    st->cc_dur = b->cc_dur / 100.0;
    st->gold_pct = b->gold;
    st->xp_pct = b->xp;
    st->potion_max = 4 + p->potion_lvl / 2;
    st->potion_heal = 0.35 + 0.05 * p->potion_lvl;
}

void stats_compute_with(Stats *st, const Profile *p, const Item *cand, int slot)
{
    double weapon = 0, armor = 0;
    memset(st, 0, sizeof *st);
    build_clear(&st->b);
    add_gear(&st->b, &weapon, &armor, p, cand, slot);
    paragon_mods(p, &st->b);
    add_elixir_and_upgrades(&st->b, p);
    resolve_build(p, &st->b);
    st->weapon = 4.0 + MIN(p->level, LEVEL_CAP) + weapon;
    core_stats(st, p);
    defence_stats(st, p, armor);
    utility_stats(st, p);
}

void stats_compute(Stats *st, const Profile *p)
{
    stats_compute_with(st, p, NULL, -1);
}

double stats_resist(const Stats *st, Element e)
{
    return e == EL_PHYS ? 0.0 : st->res[e % EL_COUNT];
}

double stats_damage_reduction(const Stats *st, int floor)
{
    double k = 40.0 * floor_scale(floor);
    double armor = st->armor / (st->armor + k);
    return 1.0 - (1.0 - armor) * (1.0 - st->dr);
}

/* ---------------------------------------------------------- estimates */

/* Additive bonus a typical hit of this build enjoys. */
static double typical_add(const Stats *st, const BuildPreset *pr, const SkillRT *core, bool melee)
{
    const BuildRT *b = &st->b;
    double a = b->add[ADD_ALL] + b->add[ADD_ELEM0 + pr->element % EL_COUNT] + 0.6 * b->add[ADD_CORE]
             + 0.2 * b->add[ADD_BASIC] + 0.8 * b->add[melee ? ADD_CLOSE : ADD_FAR] + 0.5 * b->add[ADD_CC]
             + 0.2 * b->add[ADD_ELITE] + 0.3 * b->add[ADD_DOT];
    if (core)
        a += b->add[ADD_TAG0 + core->tag % TAG_COUNT];
    return a;
}

static double typical_mult(const Stats *st, const BuildPreset *pr, const Profile *p)
{
    const BuildRT *b = &st->b;
    const SkillRT *core = &b->skill[pr->bar[1] % CLASS_SKILLS];
    double m = b->x_all * b->x_elem[pr->element % EL_COUNT] * pow(b->x_core, 0.6) * sqrt(b->x_vuln)
             * sqrt(b->x_cc) * pow(b->x_dot, 0.3);
    int i;
    m *= b->x_skill[pr->bar[1] % CLASS_SKILLS] * b->x_tag[core->tag % TAG_COUNT];
    for (i = 0; i < BAR_SLOTS; i++)
        if (p->bar[i] != NO_SKILL && b->skill[p->bar[i] % CLASS_SKILLS].behavior == SB_SUMMON)
            m *= pow(b->x_minion, 0.4);
    m *= 1.0 + st->crit * (0.5 + st->crit_dmg) * b->x_crit;
    m *= 1.0 + 0.5 * (0.2 + st->vuln_dmg);
    m *= 1.0 + st->op_chance * (0.5 + st->op_dmg) * b->x_op;
    return m;
}

double stats_dps(const Stats *st, const Profile *p)
{
    const ClassDef *c = &class_defs[p->cls % CLASS_COUNT];
    const BuildPreset *pr = &c->preset[p->preset % PRESETS];
    const SkillRT *core = &st->b.skill[pr->bar[1] % CLASS_SKILLS];
    double add = typical_add(st, pr, core, c->attack_range <= 30);
    double sig = st->b.sig ? 1.0 + st->b.sig_power / 100.0 : 1.0;
    double myth = 1.0;
    int k;
    for (k = MY_NONE + 1; k < MY_COUNT; k++)          /* a rough worth of each power, for comparing gear */
        if (st->b.myth[k] > 0)
            myth *= k == MY_ONENAME || k == MY_UNDYING ? 1.0 : 1.6;
    return st->weapon * st->stat_mult * (1.0 + add / 100.0) * typical_mult(st, pr, p) * st->aps * sig * myth;
}

double stats_power(const Stats *st, const Profile *p, int floor)
{
    double res = 0, ehp, sustain, util;
    int e;
    for (e = 1; e < EL_COUNT; e++)
        res += st->res[e] / (EL_COUNT - 1);
    ehp = st->max_hp / MAX(0.05, 1.0 - stats_damage_reduction(st, floor)) * (1.0 + 0.5 * res);
    sustain = 1.0 + (st->life_hit * st->aps + st->regen + st->life_kill * 0.5) / MAX(st->max_hp, 1.0)
            + st->heal_kill + st->barrier_gen * 0.2;
    util = 1.0 + (st->gold_pct + st->xp_pct + st->move_pct + st->cdr * 3.0) / 2000.0;
    return sqrt(stats_dps(st, p) * ehp) * sustain * util;
}

int item_target_slot(const Profile *p, const Item *cand)
{
    Stats a, b;
    int f = MAX(p->floor, 1);
    if (cand->slot != SLOT_RING1 && cand->slot != SLOT_RING2)
        return cand->slot;
    if (!p->equip[SLOT_RING1].used)
        return SLOT_RING1;
    if (!p->equip[SLOT_RING2].used)
        return SLOT_RING2;
    stats_compute_with(&a, p, cand, SLOT_RING1);
    stats_compute_with(&b, p, cand, SLOT_RING2);
    return stats_power(&a, p, f) >= stats_power(&b, p, f) ? SLOT_RING1 : SLOT_RING2;
}

double item_upgrade_ratio(const Profile *p, const Item *cand)
{
    Stats now, with;
    int f = MAX(p->floor, 1);
    double a, b;
    stats_compute(&now, p);
    stats_compute_with(&with, p, cand, item_target_slot(p, cand));
    a = stats_power(&now, p, f);
    b = stats_power(&with, p, f);
    return a > 0 ? b / a - 1.0 : 1.0;
}
