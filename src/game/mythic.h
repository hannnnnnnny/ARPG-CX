/*
 * mythic.h - super ancestral uniques (mythic tier). Every mythic carries
 * a power with code-side behaviour (world_myth.c), its own effects and
 * outsized numbers, on top of four greater affixes. They are rare: a
 * sliver of every torment drop, a little more from guardians and the
 * Fleshrender. Any class can wear any of them, several at once.
 */
#ifndef AD_MYTHIC_H
#define AD_MYTHIC_H

#include "world.h"
#include "myth_ids.h"

/* Odds per 100000 of a mythic on a guardian / Fleshrender kill. */
#define MYTHIC_ODDS_ACT_BOSS  500
#define MYTHIC_ODDS_GUARDIAN  60
#define MYTHIC_ODDS_BUTCHER   1000

/* world_myth.c (World and Profile are the running floor and the hero) */
void myth_tick(World *w, Profile *p);
void myth_on_hit(World *w, Profile *p, int mi, const Hit *h, bool crit);
void myth_on_kill(World *w, Profile *p, const Monster *m);
/* [x] damage of time stop and kill streaks on this hit (1 = none). */
double myth_damage_mult(const World *w, const Monster *m);
/* Death refused (Aegis of the Undying): true if the hero lives on. */
bool myth_refuse_death(World *w);
/* Floors to skip after a cleared one (Abyssdiver Treads), 0 = none. */
int  myth_dive(World *w, const Profile *p);
/* Rolls 'odds' per 100000 for a random mythic at (x, y). */
void mythic_try_drop(World *w, Profile *p, fx x, fx y, int odds);
/* Unique id of a mythic power's item, 0 if none. */
int  mythic_unique(int power);

#endif
