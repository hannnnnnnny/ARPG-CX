/*
 * stats.h - the hero's final numbers, Diablo IV style.
 *
 * Damage of a hit (see world_mon.c deal_damage):
 *   weapon damage x skill damage
 *   x (1 + main stat / 1000)
 *   x (1 + sum of additive +% bonuses that apply)
 *   x each [x] multiplier that applies (aspects, paragon, key passive...)
 *   x Vulnerable (1.2 + vulnerable damage), x Critical (1.5 + crit damage)
 *   Overpower: + (life + barrier) x 0.3, then x (1.5 + overpower damage)
 */
#ifndef AD_STATS_H
#define AD_STATS_H

#include "defs.h"
#include "build.h"

typedef struct {
    double weapon;               /* weapon damage per hit */
    double stat[STAT_COUNT];
    double mainstat;             /* the class main stat */
    double stat_mult;            /* 1 + main stat / 1000 */
    double aps;                  /* attacks per second */
    double crit, crit_dmg;       /* chance 0..1, bonus over the base x1.5 (0.3 = +30%) */
    double vuln_dmg;             /* bonus over the base x1.2 */
    double op_chance, op_dmg;
    double lucky;                /* added to every skill's lucky hit chance */
    double max_hp, armor, dr, dr_close, res[EL_COUNT];
    double regen, life_hit, life_kill, heal_kill, barrier_gen, thorns;
    double max_res, res_regen, res_kill;
    double cdr, move_pct, cc_dur, gold_pct, xp_pct;
    int    potion_max;
    double potion_heal;          /* fraction of life per potion */
    BuildRT b;                   /* buckets, multipliers, resolved skills */
} Stats;

void   stats_compute(Stats *st, const Profile *p);
/* Same, but as if 'cand' were equipped in 'slot'. */
void   stats_compute_with(Stats *st, const Profile *p, const Item *cand, int slot);
double stats_dps(const Stats *st, const Profile *p);
double stats_damage_reduction(const Stats *st, int floor);   /* armor + DR */
double stats_resist(const Stats *st, Element e);              /* 0..0.7 */
double stats_power(const Stats *st, const Profile *p, int floor);
/* Which slot an item would replace (rings: the weaker ring). */
int    item_target_slot(const Profile *p, const Item *cand);
/* Power delta (ratio - 1) if 'cand' replaced the current item. */
double item_upgrade_ratio(const Profile *p, const Item *cand);

#endif
