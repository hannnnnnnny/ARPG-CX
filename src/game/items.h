/*
 * items.h - loot in the Diablo IV style.
 *
 *   Common: nothing       Magic: 1 affix           Rare: 2 affixes
 *   Legendary: 3 affixes + a legendary aspect
 *   Unique: 4 fixed affixes + a unique power      Mythic: unique, all greater
 *   Ancestral (deep floors): 1-3 greater affixes (x1.5) and +100 item power
 *
 * Every base has an implicit affix (weapons by type, rings / amulet
 * resistances, boots movement). Afterwards an item can be tempered (up to
 * 2 extra affixes from a recipe, limited charges), masterworked (12 ranks,
 * +5% per rank and a critical +25% on one affix at ranks 4/8/12),
 * enchanted (reroll one affix: pick 1 of 2 or keep), imprinted with an
 * aspect from the Codex of Power, and socketed with gems.
 */
#ifndef AD_ITEMS_H
#define AD_ITEMS_H

#include "defs.h"
#include "build.h"
#include "../core/rng.h"

/* Weapon and off-hand base types (Item.base for those slots). */
typedef enum {
    WK_SWORD, WK_AXE, WK_MACE, WK_STAFF, WK_WAND, WK_BOW, WK_CROSSBOW, WK_DAGGER, WK_SCYTHE, WK_GLAIVE,
    WK_QSTAFF, WK_COUNT
} WeaponKind;
typedef enum { OK_SHIELD, OK_FOCUS, OK_TOTEM, OK_DAGGER, OK_AXE, OK_COUNT } OffhandKind;

typedef enum { TR_WEAPONRY, TR_FINESSE, TR_ELEMENTS, TR_ENDURANCE, TR_SUSTAIN, TR_PROFITEER, TR_COUNT } TemperRecipe;

/* cls: hero class, used for weapon types, skill ranks and class aspects. */
void item_roll(Item *it, Rng *r, int ilvl, int luck, Rarity min_rarity, int cls);
void item_roll_slot(Item *it, Rng *r, int ilvl, int luck, Rarity min_rarity, Slot slot, int cls);
/* A specific unique (id from aspects.c). */
void item_make_unique(Item *it, Rng *r, int ilvl, int unique, bool ancestral, int cls);
void item_rescale(Item *it, int ilvl);      /* to a deeper item level, rolls kept */

int  item_power(const Item *it);
double item_mw_mult(const Item *it);                      /* masterwork multiplier on the base stat */
double item_affix_value(const Item *it, int i);          /* incl. masterwork */
/* All modifiers an equipped item grants (affixes, aspect / unique, gems). */
int  item_mods(const Item *it, int cls, Mod *out, int cap);
bool item_fits_slot(const Item *it, Slot s);            /* rings fit both ring slots */
bool affix_is_percent(AffixType t);
bool affix_allowed(Slot s, AffixType t);

void item_name(char *out, size_t cap, const Item *it);
void affix_text(char *out, size_t cap, const Item *it, int i, int cls);
void item_main_text(char *out, size_t cap, const Item *it);
const char *item_base_name(const Item *it);
const char *slot_name(Slot s);
const char *rarity_name(Rarity r);
uint16_t rarity_color(Rarity r);
int  item_weapon_kind(const Item *it);                   /* WK_* or -1 */
int  class_weapon_kind(int cls, int pick);               /* pick-th weapon type of the class */

/* ---------------------------------------------------------- crafting */

const char *temper_name(TemperRecipe t);
bool temper_allowed(const Item *it, TemperRecipe t);
bool item_temper(Item *it, Rng *r, TemperRecipe t);       /* uses a charge */
bool item_masterwork(Item *it, Rng *r);                   /* one rank */
/* Two candidate affixes to replace affix i; false if i cannot be enchanted. */
bool item_enchant_options(const Item *it, int i, Rng *r, Affix out[2], int cls);
void item_enchant_apply(Item *it, int i, const Affix *a);
bool item_can_imprint(const Item *it, int aspect, int cls);
void item_imprint(Item *it, int aspect, int roll);

const char *gem_name(int gem);                            /* "ROYAL RUBY" */
uint16_t gem_color(int kind);
void gem_effect_text(char *out, size_t cap, int gem, Slot s);

#endif
