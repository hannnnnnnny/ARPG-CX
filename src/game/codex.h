/*
 * codex.h - where every unique comes from. The five themed guardians
 * (floors 10, 20 ... 50, then the cycle repeats every 50 floors) each
 * guard a share of the uniques and drop them far more often than the
 * depths do; mythics come from anywhere in torment, rarely. The CODEX
 * page lists all of them, found or not, and can take the hero back to a
 * guardian already reached: targeted farming.
 */
#ifndef AD_CODEX_H
#define AD_CODEX_H

#include "defs.h"
#include "../core/rng.h"

#define GUARDIANS 5                  /* one per dungeon theme */
#define GUARDIAN_UNIQUE_PCT 20       /* a guardian kill adds one of its own uniques */

int codex_guardian(int unique);                    /* theme 0..4, -1 = mythic (anywhere) */
int codex_guardian_floor_below(int theme, int floor);   /* deepest guardian floor <= floor, 0 none */
int codex_guardian_floor_from(int theme, int floor);    /* next guardian floor >= floor */
/* One of the theme guardian's uniques the class can use, 0 if none. */
int codex_pick(int theme, int cls, Rng *r);
/* Every unique id in codex order: mythics first, then by guardian. */
int codex_entries(int *out, int cap);

#endif
