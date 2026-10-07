#include "balance.h"
#include <math.h>
#include <stdio.h>

double floor_scale(int floor)
{
    double f = (double)MAX(floor, 1) - 1.0;
    return pow(1.075, f) * (1.0 + 0.04 * f);
}

double monster_scale(int floor)
{
    /* Monsters outgrow gear a little on every floor: that gap is the idle
     * "wall" that crafting, skills, paragon and rebirth push through. Past
     * the campaign the endless sources (paragon mastery, glyph levels,
     * renown, embers) carry the descent, and GAP_LATE sets its pace: tuned
     * with the long-run simulator so every build keeps descending for 300+
     * hours, reaching about floor 300. */
    int f = MAX(floor, 1);
    return floor_scale(f) * pow(GAP_EARLY, (double)(MIN(f, GAP_SPLIT) - 1))
         * pow(GAP_LATE, (double)MAX(f - GAP_SPLIT, 0));
}

double sig_resonance(int floor)
{
    int f = MAX(floor, 1);
    return pow(monster_scale(f) / floor_scale(f), SIG_RESONANCE);
}

double xp_to_next(int level)
{
    return 25.0 * pow(1.13, (double)(MIN(MAX(level, 1), LEVEL_CAP) - 1));
}

/* Torment floors cleared to take a glyph from 'level' to the next. */
int glyph_floors(int level)
{
    return 3 + MAX(level, 1) / 2;
}

void torment_name(char *out, size_t cap, int floor)
{
    static const char *const roman[] = {
        "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X",
        "XI", "XII", "XIII", "XIV", "XV", "XVI", "XVII", "XVIII", "XIX", "XX",
    };
    int t = torment_tier(floor);
    if (t < (int)(sizeof roman / sizeof roman[0]))
        snprintf(out, cap, "%s", roman[t]);
    else
        snprintf(out, cap, "%d", t);
}

int torment_tier(int floor)
{
    return floor < TORMENT_FLOOR ? 0 : 1 + (floor - TORMENT_FLOOR) / TORMENT_TIER_FLOORS;
}

double paragon_kills(int paragon_level)
{
    /* Paragon levels cost kills, not raw experience: going deeper must not
     * speed up the levels that make you go deeper (a runaway loop), so the
     * late game climbs with play time, a little slower every level. */
    return PARA_BASE + PARA_STEP * (double)MAX(paragon_level, 0);
}

double kill_xp(int floor)   { return 6.0 * floor_scale(floor); }
double kill_gold(int floor) { return 3.0 * floor_scale(floor); }

double salvage_gold(const Item *it)
{
    static const double mult[RAR_COUNT] = { 2.0, 5.0, 14.0, 40.0, 60.0, 100.0 };
    return mult[it->rarity % RAR_COUNT] * floor_scale(it->ilvl);
}

double salvage_iron(const Item *it)
{
    static const double n[RAR_COUNT] = { 1, 2, 4, 5, 5, 10 };
    return n[it->rarity % RAR_COUNT];
}

double salvage_souls(const Item *it)
{
    static const double n[RAR_COUNT] = { 0, 0, 0.5, 2, 3, 10 };
    return n[it->rarity % RAR_COUNT] * (it->ancestral ? 2 : 1);
}

double masterwork_gold(const Item *it)  { return 15.0 * pow(1.35, it->mw) * floor_scale(it->ilvl); }
double masterwork_iron(const Item *it)  { return 3.0 + 2.0 * it->mw; }
double masterwork_souls(const Item *it) { return it->mw >= 8 ? (it->mw - 7) * 2.0 : it->mw >= 4 ? 1.0 : 0.0; }
double temper_gold(const Item *it)      { return 10.0 * floor_scale(it->ilvl); }
double enchant_gold(const Item *it)     { return 12.0 * floor_scale(it->ilvl); }
double imprint_gold(const Item *it)     { return 25.0 * floor_scale(it->ilvl); }
double socket_gold(const Item *it)      { return 30.0 * floor_scale(it->ilvl); }
double gem_craft_gold(int tier)         { return 40.0 * pow(4.0, tier); }
double gamble_gold(int best_floor)      { return 60.0 * floor_scale(best_floor); }
double elixir_gold(int best_floor)      { return 40.0 * floor_scale(best_floor); }
double potion_upgrade_gold(int lvl)     { return 500.0 * pow(6.0, lvl); }

double respec_gold(int level, int best_floor)
{
    return level < 15 ? 0.0 : floor(20.0 * floor_scale(best_floor));
}

double ember_reward(int best_floor)
{
    if (best_floor < REBIRTH_MIN_FLOOR)
        return 0;
    return floor(pow(best_floor / 10.0, 2.2));
}

double upgrade_cost(int rank)
{
    return ceil(pow(1.55, (double)rank));
}

int upgrade_max(UpgradeId id)
{
    switch (id) {
    case UP_START:    return 10;
    case UP_PATIENCE: return 8;
    case UP_FORTUNE:  return 20;
    case UP_HASTE:    return 20;
    default:          return 99;
    }
}

/* Packed floors (Diablo IV density): bigger packs, more of them deeper down. */
int floor_monsters(int floor) { return 26 + MIN(floor / 4, 22); }
int floor_quota(int floor)    { return floor_monsters(floor) * 3 / 4; }
