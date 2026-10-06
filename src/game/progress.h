/*
 * progress.h - every change to the saved Profile goes through here:
 * experience and paragon, gold, loot decisions, salvage, the town's
 * crafting services (blacksmith, occultist, jeweler, alchemist, gambler),
 * respec, floors, death, rebirth and offline gains. Keeping it in one
 * module makes it easy to test.
 */
#ifndef AD_PROGRESS_H
#define AD_PROGRESS_H

#include "defs.h"
#include "items.h"
#include "../core/rng.h"

void prog_new(Profile *p, uint32_t seed, int cls);
void prog_default_look(Look *l, int cls, uint32_t seed);
/* Returns number of levels (and paragon levels) gained. */
int  prog_add_xp(Profile *p, double xp);
double prog_xp_needed(const Profile *p);
void prog_add_gold(Profile *p, double gold);
int  prog_paragon_available(const Profile *p);

typedef enum { LOOT_EQUIPPED, LOOT_BAGGED, LOOT_SALVAGED, LOOT_BAG_FULL_SALVAGED } LootResult;
LootResult prog_handle_loot(Profile *p, const Item *it, double *gold_gained);
/* True if picking up 'it' taught the codex a new (or better) aspect. */
bool prog_learn_aspect(Profile *p, const Item *it);

bool prog_equip_from_bag(Profile *p, int idx);
bool prog_salvage_bag(Profile *p, int idx, double *gold_gained);
int  prog_salvage_all_unlocked(Profile *p, double *gold_gained);

/* ---- town services (all pay with gold and materials) */
typedef enum { CRAFT_OK, CRAFT_NO_GOLD, CRAFT_NO_MATS, CRAFT_INVALID } CraftResult;
CraftResult prog_masterwork(Profile *p, Slot s, Rng *r);
CraftResult prog_temper(Profile *p, Slot s, TemperRecipe t, Rng *r);
CraftResult prog_enchant(Profile *p, Slot s, int affix, Affix out[2], Rng *r); /* pays, gives 2 options */
CraftResult prog_imprint(Profile *p, Slot s, int aspect);
CraftResult prog_add_socket(Profile *p, Slot s);
bool        prog_socket_gem(Profile *p, Slot s, int socket, int gem);   /* takes one from the pouch */
bool        prog_unsocket(Profile *p, Slot s, int socket);              /* the gem returns to the pouch */
CraftResult prog_craft_gem(Profile *p, int kind, int tier);
CraftResult prog_gamble(Profile *p, Slot s, Rng *r, Item *out);
CraftResult prog_buy_elixir(Profile *p, ElixirKind k);
CraftResult prog_upgrade_potion(Profile *p);
/* Idle crafting: masterwork, temper, imprint, gems, elixirs. Returns actions done. */
int  prog_auto_craft(Profile *p, Rng *r);

/* ---- respec */
double prog_respec_cost(const Profile *p);
bool   prog_respec_skills(Profile *p);              /* refund; auto planner refills */
bool   prog_switch_preset(Profile *p, int preset);  /* respec into another build */
bool   prog_respec_paragon(Profile *p);

void prog_floor_cleared(Profile *p, bool boss);
void prog_died(Profile *p);
void prog_tick_second(Profile *p);                  /* elixir timers */

bool   prog_can_rebirth(const Profile *p);
double prog_rebirth(Profile *p, int new_cls);       /* returns embers gained */
bool   prog_buy_upgrade(Profile *p, UpgradeId id);

typedef struct {
    uint32_t seconds;     /* credited (after cap) */
    uint32_t raw_seconds; /* actually elapsed */
    double kills, gold, xp;
    int levels;
    int items_kept, items_salvaged;
} OfflineReport;

/* Credit time away since p->save_time. Returns false if nothing credited. */
bool prog_offline(Profile *p, uint32_t now, OfflineReport *rep);
uint32_t prog_offline_cap_seconds(const Profile *p);

#endif
