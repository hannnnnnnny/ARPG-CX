/*
 * balance.h - every growth curve and every price in one place.
 *
 * Monsters and loot both scale with floor_scale(f) so gear found on a floor
 * is roughly "on level" for that floor; the hero pulls ahead through
 * rarity, crafting (masterwork, tempering, gems), skills, paragon and
 * rebirth upgrades. tests.c fast-forwards hours of idle play to check the
 * curve keeps moving.
 */
#ifndef AD_BALANCE_H
#define AD_BALANCE_H

#include "defs.h"

double floor_scale(int floor);
double monster_scale(int floor);
double xp_to_next(int level);
/* Kill-equivalents (experience / kill_xp of the floor) per paragon level. */
#ifndef PARA_BASE
#define PARA_BASE 40.0
#endif
#ifndef PARA_STEP
#define PARA_STEP 2.2
#endif
double paragon_kills(int paragon_level);
double kill_xp(int floor);
double kill_gold(int floor);
double ember_reward(int best_floor);
double upgrade_cost(int rank);
int    upgrade_max(UpgradeId id);

/* Salvage yields */
double salvage_gold(const Item *it);
double salvage_iron(const Item *it);
double salvage_souls(const Item *it);

/* Prices (gold unless noted). best_floor scales prices so gold always matters. */
double masterwork_gold(const Item *it);
double masterwork_iron(const Item *it);
double masterwork_souls(const Item *it);
double temper_gold(const Item *it);
double enchant_gold(const Item *it);
double imprint_gold(const Item *it);
double socket_gold(const Item *it);
double gem_craft_gold(int tier);       /* 3 gems of 'tier' -> 1 of tier + 1 */
double gamble_gold(int best_floor);
double elixir_gold(int best_floor);
double potion_upgrade_gold(int lvl);
double respec_gold(int level, int best_floor);
#define POTION_MAX_LVL 6

/* Monsters required to open the stairs on a floor. */
int    floor_quota(int floor);
int    floor_monsters(int floor);
static inline bool is_boss_floor(int floor) { return floor % 10 == 0; }

#define REBIRTH_MIN_FLOOR 20
#define OFFLINE_BASE_HOURS 24
#define OFFLINE_PATIENCE_HOURS 4       /* per PATIENCE rank */
#define TORMENT_FLOOR 51               /* past the campaign: ancestral loot, glyph levels */
#define TORMENT_TIER_FLOORS 50         /* Torment I = 51-100, II = 101-150, ... */
#define GAP_EARLY 1.05                 /* monster lead over gear per floor up to GAP_SPLIT, then GAP_LATE */
#define GAP_SPLIT 100
#ifndef GAP_LATE
#define GAP_LATE 1.06                 /* overridable for balance experiments */
#endif

/* Signature builds resonate with the depths: they shrug off this power of
 * the monsters' lead over gear (sig_resonance), in damage dealt and taken,
 * so they push about half again as deep as any other build. */
#ifndef SIG_RESONANCE
#define SIG_RESONANCE 0.35
#endif
double sig_resonance(int floor);

/* Paragon mastery: every paragon level past MASTERY_FROM adds this much
 * damage and life (compounding), the endless engine of the late game. */
#define MASTERY_FROM 100
#ifndef MASTERY_PCT
#define MASTERY_PCT 1.0
#endif
static inline int paragon_mastery(int paragon_level) { return paragon_level > MASTERY_FROM ? paragon_level - MASTERY_FROM : 0; }

/* Torment tier of a floor: 0 before floor 51, then I, II, ... every 50. */
int    torment_tier(int floor);
void   torment_name(char *out, size_t cap, int floor);   /* "I", "II", ... */
int    glyph_floors(int level);

#endif
