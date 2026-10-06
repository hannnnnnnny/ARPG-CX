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
double paragon_xp(int paragon_level);  /* experience for the next paragon level */
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
#define OFFLINE_BASE_HOURS 8
#define TORMENT_FLOOR 51               /* past the campaign: ancestral loot, glyph levels */

#endif
