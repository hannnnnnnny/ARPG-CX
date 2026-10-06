#include "items.h"
#include "aspects.h"
#include "balance.h"
#include "skills.h"
#include "../core/bignum.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------- tables */

enum { SC_NONE, SC_FLOOR, SC_STAT, SC_INT };   /* how an affix value scales */

typedef struct {
    const char *label;
    double lo, hi;
    uint8_t scale;
    uint8_t mod, arg;   /* the build modifier it becomes */
    const char *word;   /* magic item name fragment */
} AffixInfo;

static const AffixInfo affix_info[AF_COUNT] = {
    [AF_MAINSTAT]  = { "", 20, 45, SC_STAT, MOD_MAINSTAT, 0, "MIGHTY" },
    [AF_CRIT]      = { "CRITICAL STRIKE CHANCE", 2, 5, SC_NONE, MOD_CRIT, 0, "PRECISE" },
    [AF_CRIT_DMG]  = { "CRITICAL STRIKE DAMAGE", 10, 25, SC_NONE, MOD_CRIT_DMG, 0, "SAVAGE" },
    [AF_VULN_DMG]  = { "VULNERABLE DAMAGE", 10, 25, SC_NONE, MOD_VULN_DMG, 0, "CRUEL" },
    [AF_OP_DMG]    = { "OVERPOWER DAMAGE", 15, 35, SC_NONE, MOD_OP_DMG, 0, "CRUSHING" },
    [AF_ATK_SPD]   = { "ATTACK SPEED", 3, 8, SC_NONE, MOD_ATK_SPD, 0, "QUICK" },
    [AF_LUCKY]     = { "LUCKY HIT CHANCE", 3, 7, SC_NONE, MOD_LUCKY, 0, "LUCKY" },
    [AF_DMG_CLOSE] = { "DAMAGE TO CLOSE ENEMIES", 8, 18, SC_NONE, MOD_ADD_CLOSE, 0, "BRUTAL" },
    [AF_DMG_FAR]   = { "DAMAGE TO DISTANT ENEMIES", 8, 18, SC_NONE, MOD_ADD_FAR, 0, "FARSIGHTED" },
    [AF_DMG_CC]    = { "DAMAGE TO CROWD CONTROLLED", 8, 18, SC_NONE, MOD_ADD_CC, 0, "TYRANT'S" },
    [AF_DMG_ELITE] = { "DAMAGE TO ELITES", 8, 18, SC_NONE, MOD_ADD_ELITE, 0, "SLAYER'S" },
    [AF_DOT]       = { "DAMAGE OVER TIME", 10, 22, SC_NONE, MOD_ADD_DOT, 0, "FESTERING" },
    [AF_CORE_DMG]  = { "CORE SKILL DAMAGE", 8, 18, SC_NONE, MOD_ADD_CORE, 0, "ARCANE" },
    [AF_BASIC_DMG] = { "BASIC SKILL DAMAGE", 10, 25, SC_NONE, MOD_ADD_BASIC, 0, "HONED" },
    [AF_PHYS]      = { "PHYSICAL DAMAGE", 8, 18, SC_NONE, MOD_ADD_ELEM, EL_PHYS, "HEAVY" },
    [AF_FIRE]      = { "FIRE DAMAGE", 8, 18, SC_NONE, MOD_ADD_ELEM, EL_FIRE, "SEARING" },
    [AF_COLD]      = { "COLD DAMAGE", 8, 18, SC_NONE, MOD_ADD_ELEM, EL_COLD, "FROZEN" },
    [AF_LIGHT]     = { "LIGHTNING DAMAGE", 8, 18, SC_NONE, MOD_ADD_ELEM, EL_LIGHT, "CHARGED" },
    [AF_POISON]    = { "POISON DAMAGE", 8, 18, SC_NONE, MOD_ADD_ELEM, EL_POISON, "VENOMOUS" },
    [AF_SHADOW]    = { "SHADOW DAMAGE", 8, 18, SC_NONE, MOD_ADD_ELEM, EL_SHADOW, "UMBRAL" },
    [AF_RANKS]     = { "RANKS TO", 1, 2, SC_INT, MOD_SK_RANKS, 0, "ADEPT'S" },
    [AF_LH_VULN]   = { "", 5, 12, SC_NONE, MOD_LH_VULN, 0, "EXPOSING" },
    [AF_LIFE]      = { "MAXIMUM LIFE", 12, 32, SC_FLOOR, MOD_LIFE, 0, "OF THE BEAR" },
    [AF_ARMOR]     = { "ARMOR", 5, 15, SC_FLOOR, MOD_ARMOR, 0, "OF IRON" },
    [AF_DR]        = { "DAMAGE REDUCTION", 2, 5, SC_NONE, MOD_DR, 0, "OF WARDING" },
    [AF_DR_CLOSE]  = { "DAMAGE REDUCTION FROM CLOSE", 4, 9, SC_NONE, MOD_DR_CLOSE, 0, "OF THE WALL" },
    [AF_RES_ALL]   = { "RESISTANCE TO ALL ELEMENTS", 3, 8, SC_NONE, MOD_RES_ALL, 0, "OF THE PRISM" },
    [AF_LIFE_HIT]  = { "LIFE ON HIT", 0.6, 2.0, SC_FLOOR, MOD_LIFE_HIT, 0, "OF LEECHING" },
    [AF_LIFE_KILL] = { "LIFE ON KILL", 3, 8, SC_FLOOR, MOD_LIFE_KILL, 0, "OF FEASTING" },
    [AF_REGEN]     = { "LIFE PER SECOND", 1.0, 3.5, SC_FLOOR, MOD_REGEN, 0, "OF MENDING" },
    [AF_BARRIER]   = { "BARRIER GENERATION", 6, 15, SC_NONE, MOD_BARRIER_GEN, 0, "OF SHELTER" },
    [AF_THORNS]    = { "THORNS", 2, 6, SC_FLOOR, MOD_THORNS, 0, "OF BRAMBLES" },
    [AF_CDR]       = { "COOLDOWN REDUCTION", 3, 7, SC_NONE, MOD_CDR, 0, "OF FOCUS" },
    [AF_COST]      = { "RESOURCE COST REDUCTION", 3, 8, SC_NONE, MOD_COST, 0, "OF THRIFT" },
    [AF_RES_GEN]   = { "RESOURCE GENERATION", 5, 12, SC_NONE, MOD_RES_GEN, 0, "OF THE SPRING" },
    [AF_MAX_RES]   = { "MAXIMUM RESOURCE", 4, 10, SC_INT, MOD_MAX_RES, 0, "OF DEPTH" },
    [AF_MOVE]      = { "MOVEMENT SPEED", 4, 9, SC_NONE, MOD_MOVE, 0, "OF THE WIND" },
    [AF_CC_DUR]    = { "CROWD CONTROL DURATION", 5, 12, SC_NONE, MOD_CC_DUR, 0, "OF BINDING" },
    [AF_GOLD]      = { "GOLD FOUND", 8, 25, SC_NONE, MOD_GOLD, 0, "OF GREED" },
    [AF_XP]        = { "EXPERIENCE", 5, 15, SC_NONE, MOD_XP, 0, "OF WISDOM" },
};

#define B(x) (1ULL << (x))
#define ELEMENTS (B(AF_PHYS) | B(AF_FIRE) | B(AF_COLD) | B(AF_LIGHT) | B(AF_POISON) | B(AF_SHADOW))

/* Which affixes may roll on each slot. */
static const uint64_t slot_pool[SLOT_COUNT] = {
    [SLOT_WEAPON]  = B(AF_MAINSTAT) | B(AF_CRIT_DMG) | B(AF_VULN_DMG) | B(AF_OP_DMG) | B(AF_DMG_CLOSE) | B(AF_DMG_FAR)
                   | B(AF_DMG_CC) | B(AF_DMG_ELITE) | B(AF_DOT) | B(AF_CORE_DMG) | B(AF_BASIC_DMG) | ELEMENTS,
    [SLOT_OFFHAND] = B(AF_MAINSTAT) | B(AF_CRIT) | B(AF_LUCKY) | B(AF_CDR) | B(AF_COST) | B(AF_RANKS) | B(AF_VULN_DMG)
                   | B(AF_DOT) | B(AF_LIFE) | B(AF_LH_VULN) | B(AF_CORE_DMG),
    [SLOT_HELM]    = B(AF_MAINSTAT) | B(AF_LIFE) | B(AF_CDR) | B(AF_MAX_RES) | B(AF_ARMOR) | B(AF_RES_ALL)
                   | B(AF_RANKS) | B(AF_COST),
    [SLOT_CHEST]   = B(AF_MAINSTAT) | B(AF_LIFE) | B(AF_ARMOR) | B(AF_DR) | B(AF_DR_CLOSE) | B(AF_RES_ALL)
                   | B(AF_THORNS) | B(AF_REGEN),
    [SLOT_GLOVES]  = B(AF_MAINSTAT) | B(AF_ATK_SPD) | B(AF_CRIT) | B(AF_LUCKY) | B(AF_LH_VULN) | B(AF_RANKS)
                   | B(AF_CRIT_DMG) | B(AF_VULN_DMG) | B(AF_LIFE_HIT),
    [SLOT_PANTS]   = B(AF_MAINSTAT) | B(AF_LIFE) | B(AF_ARMOR) | B(AF_DR) | B(AF_DR_CLOSE) | B(AF_RES_ALL)
                   | B(AF_BARRIER) | B(AF_CC_DUR),
    [SLOT_BOOTS]   = B(AF_MAINSTAT) | B(AF_MOVE) | B(AF_RES_GEN) | B(AF_COST) | B(AF_LIFE) | B(AF_ARMOR)
                   | B(AF_RES_ALL) | B(AF_GOLD) | B(AF_XP),
    [SLOT_AMULET]  = B(AF_MAINSTAT) | B(AF_CRIT) | B(AF_CRIT_DMG) | B(AF_VULN_DMG) | B(AF_CDR) | B(AF_COST)
                   | B(AF_MOVE) | B(AF_DR) | B(AF_RANKS) | B(AF_XP) | B(AF_GOLD) | B(AF_RES_ALL) | B(AF_ATK_SPD),
    [SLOT_RING1]   = B(AF_CRIT) | B(AF_CRIT_DMG) | B(AF_VULN_DMG) | B(AF_OP_DMG) | B(AF_LUCKY) | B(AF_LIFE)
                   | B(AF_MAX_RES) | B(AF_COST) | B(AF_REGEN) | ELEMENTS | B(AF_LIFE_HIT) | B(AF_LIFE_KILL)
                   | B(AF_XP) | B(AF_GOLD) | B(AF_RES_GEN),
};

static const char *const weapon_names[WK_COUNT] = {
    "SWORD", "AXE", "MACE", "STAFF", "WAND", "BOW", "CROSSBOW", "DAGGER", "SCYTHE", "GLAIVE", "QUARTERSTAFF"
};
static const char *const offhand_names[OK_COUNT] = { "SHIELD", "FOCUS", "TOTEM", "DAGGER", "HAND AXE" };
static const char *const armor_names[SLOT_COUNT][4] = {
    [SLOT_HELM]   = { "HOOD", "SKULLCAP", "HORNED HELM", "GREAT HELM" },
    [SLOT_CHEST]  = { "ROBE", "LEATHERS", "CHAIN MAIL", "PLATE" },
    [SLOT_GLOVES] = { "WRAPS", "GLOVES", "GAUNTLETS", "WARGRIPS" },
    [SLOT_PANTS]  = { "LEGGINGS", "TROUSERS", "CHAINLEGS", "GREAVES" },
    [SLOT_BOOTS]  = { "SANDALS", "BOOTS", "WARBOOTS", "TREADS" },
    [SLOT_AMULET] = { "CHARM", "AMULET", "TALISMAN", "PENDANT" },
    [SLOT_RING1]  = { "BAND", "RING", "SIGNET", "LOOP" },
};

/* Implicit affix of each weapon / off-hand type: {type, lo, hi} */
static const struct { uint8_t type; double lo, hi; } weapon_implicit[WK_COUNT] = {
    { AF_CRIT_DMG, 10, 20 }, { AF_DMG_CLOSE, 10, 20 }, { AF_OP_DMG, 15, 30 }, { AF_DMG_CC, 10, 20 },
    { AF_LUCKY, 4, 8 }, { AF_DMG_FAR, 10, 20 }, { AF_VULN_DMG, 10, 20 }, { AF_CRIT, 2, 4 },
    { AF_LIFE_KILL, 3, 6 }, { AF_ATK_SPD, 4, 8 }, { AF_DMG_CC, 10, 20 },
}, offhand_implicit[OK_COUNT] = {
    { AF_DR, 4, 8 }, { AF_COST, 5, 10 }, { AF_CDR, 4, 8 }, { AF_CRIT, 2, 4 }, { AF_DMG_CLOSE, 8, 15 },
};

static const uint8_t class_weapons[CLASS_COUNT][3] = {
    { WK_AXE, WK_SWORD, WK_MACE }, { WK_STAFF, WK_WAND, WK_STAFF }, { WK_BOW, WK_CROSSBOW, WK_DAGGER },
    { WK_SCYTHE, WK_SWORD, WK_WAND }, { WK_STAFF, WK_MACE, WK_AXE }, { WK_GLAIVE, WK_QSTAFF, WK_GLAIVE },
};
static const uint8_t class_offhand[CLASS_COUNT] = { OK_AXE, OK_FOCUS, OK_DAGGER, OK_SHIELD, OK_TOTEM, OK_TOTEM };

static const char *const rare_a[12] = { "GRIM", "DOOM", "BLOOD", "STORM", "GHOUL", "RUNE",
                                        "DUSK", "ASH", "BONE", "DREAD", "VILE", "HEX" };
static const char *const rare_b[12] = { "FANG", "BITE", "SHROUD", "WARD", "GRASP", "HOWL",
                                        "MARK", "EDGE", "VEIL", "BRAND", "COIL", "SCAR" };

/* ------------------------------------------------------------- helpers */

static double frand(Rng *r, double lo, double hi)
{
    return lo + (hi - lo) * (double)(rng_next(r) % 10001u) / 10000.0;
}

static Slot pool_slot(Slot s) { return s == SLOT_RING2 ? SLOT_RING1 : s; }

bool affix_allowed(Slot s, AffixType t)
{
    return t < AF_COUNT && ((slot_pool[pool_slot(s)] >> t) & 1u);
}

bool affix_is_percent(AffixType t)
{
    return affix_info[t % AF_COUNT].scale == SC_NONE;
}

int class_weapon_kind(int cls, int pick)
{
    return class_weapons[CLAMP(cls, 0, CLASS_COUNT - 1)][((pick % 3) + 3) % 3];
}

int item_weapon_kind(const Item *it)
{
    return it->used && it->slot == SLOT_WEAPON ? it->base % WK_COUNT : -1;
}

bool item_fits_slot(const Item *it, Slot s)
{
    if (!it->used)
        return false;
    if (it->slot == SLOT_RING1 || it->slot == SLOT_RING2)
        return s == SLOT_RING1 || s == SLOT_RING2;
    return it->slot == s;
}

static bool has_affix(const Item *it, int t)
{
    int i;
    for (i = 0; i < it->naff; i++)
        if (it->aff[i].type == t && !(it->aff[i].flags & AFX_IMPLICIT))
            return true;
    return false;
}

/* Value of a fresh affix roll. */
static double roll_value(Rng *r, int t, int ilvl, double mult)
{
    const AffixInfo *a = &affix_info[t];
    double v = frand(r, a->lo, a->hi) * mult;
    switch (a->scale) {
    case SC_FLOOR: return v * floor_scale(ilvl);
    case SC_STAT:  return v * (1.0 + ilvl / 40.0);
    case SC_INT:   return (double)(int)(v + 0.5);
    default:       return v;
    }
}

static void set_affix(Affix *a, Rng *r, int t, int ilvl, double mult, int cls)
{
    memset(a, 0, sizeof *a);
    a->type = (uint8_t)t;
    a->value = roll_value(r, t, ilvl, mult);
    if (t == AF_RANKS)                      /* +ranks to one of the class's non-ultimate skills */
        a->arg = (uint8_t)rng_range(r, 0, CLASS_SKILLS - 2);
    (void)cls;
}

/* Pick an affix type from the slot pool that the item doesn't have yet. */
static int pick_affix(const Item *it, Rng *r, uint64_t pool)
{
    int tries;
    for (tries = 0; tries < 64; tries++) {
        int t = rng_range(r, 0, AF_COUNT - 1);
        if (((pool >> t) & 1u) && !has_affix(it, t))
            return t;
    }
    return -1;
}

static bool add_affix(Item *it, Rng *r, double mult, int cls)
{
    int t = pick_affix(it, r, slot_pool[pool_slot((Slot)it->slot)]);
    if (t < 0 || it->naff >= MAX_AFFIX)
        return false;
    set_affix(&it->aff[it->naff++], r, t, it->ilvl, mult, cls);
    return true;
}

/* ------------------------------------------------------------- rolling */

static Rarity roll_rarity(Rng *r, int ilvl, int luck, Rarity min_rarity)
{
    /* Per 100000: mythic 20 (torment), unique 300, legendary 2500, rare 12000, magic 30000. */
    int roll = (int)(rng_next(r) % 100000u);
    double boost = 1.0 + 0.10 * luck;
    int my = ilvl > 50 ? (int)(20 * boost) : 0, un = ilvl >= 5 ? (int)(300 * boost) : 0;
    int lg = (int)(2500 * boost), ra = (int)(12000 * boost), ma = (int)(30000 * boost);
    Rarity res = roll < my ? RAR_MYTHIC : roll < my + un ? RAR_UNIQUE : roll < my + un + lg ? RAR_LEGEND
               : roll < my + un + lg + ra ? RAR_RARE : roll < my + un + lg + ra + ma ? RAR_MAGIC : RAR_COMMON;
    return res < min_rarity ? min_rarity : res;
}

static bool roll_ancestral(Rng *r, int ilvl)
{
    int pct = ilvl > 50 ? MIN(40, 10 + (ilvl - 51) / 2) : ilvl > 30 ? 3 : 0;
    return rng_range(r, 0, 99) < pct;
}

static void roll_base(Item *it, Rng *r, int cls)
{
    int tier = it->ilvl / 15 - rng_range(r, 0, 1);
    if (it->slot == SLOT_WEAPON)
        it->base = (uint8_t)class_weapon_kind(cls, rng_range(r, 0, 2));
    else if (it->slot == SLOT_OFFHAND)
        it->base = class_offhand[CLAMP(cls, 0, CLASS_COUNT - 1)];
    else
        it->base = (uint8_t)CLAMP(tier, 0, 3);
}

static void roll_main(Item *it, Rng *r)
{
    static const double armor_share[SLOT_COUNT] = { 0, 0.4, 0.8, 1.4, 0.6, 0.9, 0.7, 0, 0, 0 };
    static const double rar_mult[RAR_COUNT] = { 1.0, 1.1, 1.2, 1.35, 1.45, 1.6 };
    double s = floor_scale(it->ilvl) * rar_mult[it->rarity] * (it->ancestral ? 1.15 : 1.0);
    if (it->slot == SLOT_WEAPON)
        it->main = 10.0 * s * frand(r, 0.85, 1.15);
    else
        it->main = 8.0 * s * armor_share[it->slot] * frand(r, 0.85, 1.15);
}

static void roll_implicit(Item *it, Rng *r)
{
    Affix *a = &it->aff[0];
    double lo, hi;
    int t;
    if (it->slot == SLOT_WEAPON) {
        t = weapon_implicit[it->base % WK_COUNT].type, lo = weapon_implicit[it->base % WK_COUNT].lo;
        hi = weapon_implicit[it->base % WK_COUNT].hi;
    } else if (it->slot == SLOT_OFFHAND) {
        t = offhand_implicit[it->base % OK_COUNT].type, lo = offhand_implicit[it->base % OK_COUNT].lo;
        hi = offhand_implicit[it->base % OK_COUNT].hi;
    } else if (it->slot == SLOT_RING1 || it->slot == SLOT_RING2 || it->slot == SLOT_AMULET) {
        t = AF_RES_ALL, lo = it->slot == SLOT_AMULET ? 6 : 4, hi = it->slot == SLOT_AMULET ? 12 : 8;
    } else if (it->slot == SLOT_BOOTS) {
        t = AF_MOVE, lo = 4, hi = 8;
    } else {
        return;
    }
    memset(a, 0, sizeof *a);
    a->type = (uint8_t)t;
    a->flags = AFX_IMPLICIT;
    a->value = frand(r, lo, hi) * (affix_info[t].scale == SC_FLOOR ? floor_scale(it->ilvl) : 1.0);
    it->naff = 1;
}

/* 1-3 random non-implicit affixes become greater (x1.5). */
static void make_greater(Item *it, Rng *r, int count)
{
    int tries;
    for (tries = 0; tries < 40 && count > 0; tries++) {
        int i = rng_range(r, 0, it->naff - 1);
        Affix *a = &it->aff[i];
        if (a->flags & (AFX_IMPLICIT | AFX_GREATER))
            continue;
        a->flags |= AFX_GREATER;
        a->value = a->type == AF_RANKS || a->type == AF_MAX_RES ? a->value + 1 : a->value * 1.5;
        count--;
    }
}

static int greater_count(Rng *r)
{
    int roll = rng_range(r, 0, 99);
    return roll < 70 ? 1 : roll < 95 ? 2 : 3;
}

static int pick_aspect(Rng *r, Slot s, int cls)
{
    int tries;
    for (tries = 0; tries < 200; tries++) {
        int id = rng_range(r, 1, aspect_count);
        const AspectDef *a = aspect_def(id);
        bool own = a->cls == cls, any = a->cls == ANY_CLASS;
        /* Class aspects show up more often than generic ones. */
        if (aspect_fits_slot(a, s) && (own || (any && rng_range(r, 0, 99) < 60)))
            return id;
    }
    return 1;
}

static int pick_unique(Rng *r, Slot s, int cls, bool mythic)
{
    int ids[32], n = 0, i;
    for (i = 1; i <= unique_count && n < 32; i++) {
        const UniqueDef *u = unique_def(i);
        bool slot_ok = u->slot == pool_slot(s);
        if (slot_ok && (bool)u->mythic == mythic && (u->cls == ANY_CLASS || u->cls == cls))
            ids[n++] = i;
    }
    return n ? ids[rng_range(r, 0, n - 1)] : 0;
}

static void finish_item(Item *it, Rng *r)
{
    int roll = rng_range(r, 0, 99);
    it->temper_left = TEMPER_CHARGES;
    it->enchant = 0xFF;
    it->gem[0] = it->gem[1] = GEM_NONE;
    if (it->rarity < RAR_RARE)
        it->sockets = 0;
    else if (it->slot == SLOT_WEAPON)
        it->sockets = (uint8_t)(roll < 30 ? 2 : roll < 70 ? 1 : 0);
    else
        it->sockets = (uint8_t)(roll < 35 ? 1 : 0);
    it->name = (uint8_t)rng_range(r, 0, 143);
}

/* Raise an item to a deeper item level, keeping every roll, temper and
 * masterwork: values that grow with depth grow with it. */
void item_rescale(Item *it, int ilvl)
{
    double k;
    int i;
    if (!it->used || ilvl <= it->ilvl)
        return;
    k = floor_scale(ilvl) / floor_scale(it->ilvl);
    it->main *= k;
    for (i = 0; i < it->naff; i++)
        if (affix_info[it->aff[i].type % AF_COUNT].scale == SC_FLOOR)
            it->aff[i].value *= k;
    it->ilvl = (uint16_t)ilvl;
}

void item_make_unique(Item *it, Rng *r, int ilvl, int unique, bool ancestral, int cls)
{
    const UniqueDef *u = unique_def(unique);
    int i;
    if (!u)
        return;
    memset(it, 0, sizeof *it);
    it->used = 1;
    it->slot = u->slot;
    it->base = u->base;
    it->ilvl = (uint16_t)CLAMP(ilvl, 1, 65535);
    it->rarity = u->mythic ? RAR_MYTHIC : RAR_UNIQUE;
    it->ancestral = (uint8_t)(ancestral || u->mythic);
    it->power = (uint8_t)unique;
    it->power_roll = (uint16_t)rng_range(r, 0, 1000);
    roll_main(it, r);
    roll_implicit(it, r);
    for (i = 0; i < 4 && it->naff < MAX_AFFIX; i++)
        set_affix(&it->aff[it->naff++], r, u->aff[i], it->ilvl, 1.2, cls);
    if (u->mythic)
        make_greater(it, r, 4);
    else if (it->ancestral)
        make_greater(it, r, greater_count(r));
    finish_item(it, r);
}

static void roll_affixes(Item *it, Rng *r, int cls)
{
    static const int count[RAR_COUNT] = { 0, 1, 2, 3, 0, 0 };
    static const double mult[RAR_COUNT] = { 1.0, 0.8, 1.0, 1.1, 1.2, 1.2 };
    int i;
    for (i = 0; i < count[it->rarity]; i++)
        add_affix(it, r, mult[it->rarity], cls);
}

void item_roll_slot(Item *it, Rng *r, int ilvl, int luck, Rarity min_rarity, Slot slot, int cls)
{
    Rarity rar = roll_rarity(r, ilvl, luck, min_rarity);
    bool anc = roll_ancestral(r, ilvl);
    if (rar >= RAR_UNIQUE) {
        int u = pick_unique(r, slot, cls, rar == RAR_MYTHIC);
        if (u) {
            item_make_unique(it, r, ilvl, u, anc, cls);
            return;
        }
        rar = RAR_LEGEND;                      /* no unique for this slot: a legendary instead */
    }
    memset(it, 0, sizeof *it);
    it->used = 1;
    it->slot = (uint8_t)pool_slot(slot);
    it->ilvl = (uint16_t)CLAMP(ilvl, 1, 65535);
    it->rarity = (uint8_t)rar;
    it->ancestral = (uint8_t)(rar == RAR_LEGEND && anc);
    roll_base(it, r, cls);
    roll_main(it, r);
    roll_implicit(it, r);
    roll_affixes(it, r, cls);
    if (it->ancestral)
        make_greater(it, r, greater_count(r));
    if (rar == RAR_LEGEND) {
        it->power = (uint8_t)pick_aspect(r, (Slot)it->slot, cls);
        it->power_roll = (uint16_t)rng_range(r, 0, 1000);
    }
    finish_item(it, r);
}

void item_roll(Item *it, Rng *r, int ilvl, int luck, Rarity min_rarity, int cls)
{
    /* Rings are twice as likely: there are two ring slots. */
    int s = rng_range(r, 0, SLOT_COUNT - 1);
    item_roll_slot(it, r, ilvl, luck, min_rarity, (Slot)(s == SLOT_RING2 ? SLOT_RING1 : s), cls);
}

/* ------------------------------------------------------------- values */

int item_power(const Item *it)
{
    return 100 + 8 * it->ilvl + (it->ancestral ? 100 : 0);
}

double item_mw_mult(const Item *it)
{
    return 1.0 + 0.05 * it->mw;
}

double item_affix_value(const Item *it, int i)
{
    const Affix *a = &it->aff[i];
    double v = a->value;
    if (a->flags & AFX_IMPLICIT || a->type == AF_RANKS || a->type == AF_MAX_RES)
        return v;
    return v * item_mw_mult(it) * (1.0 + 0.25 * a->mwcrit);
}

static int gem_mods(int gem, Slot s, int ilvl, Mod *out, int cap);

int item_mods(const Item *it, int cls, Mod *out, int cap)
{
    int i, n = 0;
    if (!it->used)
        return 0;
    for (i = 0; i < it->naff && n < cap; i++) {
        const Affix *a = &it->aff[i];
        const AffixInfo *info = &affix_info[a->type % AF_COUNT];
        out[n].kind = info->mod;
        out[n].arg = a->type == AF_RANKS ? a->arg : info->arg;
        out[n].value = item_affix_value(it, i);
        n++;
    }
    n += item_power_mods(it, cls, out + n, cap - n);
    for (i = 0; i < it->sockets && i < 2; i++)
        if (it->gem[i] != GEM_NONE)
            n += gem_mods(it->gem[i], (Slot)it->slot, it->ilvl, out + n, cap - n);
    return n;
}

/* --------------------------------------------------------------- text */

const char *item_base_name(const Item *it)
{
    if (it->slot == SLOT_WEAPON)
        return weapon_names[it->base % WK_COUNT];
    if (it->slot == SLOT_OFFHAND)
        return offhand_names[it->base % OK_COUNT];
    return armor_names[pool_slot((Slot)it->slot)][it->base & 3];
}

static void magic_name(char *out, size_t cap, const Item *it)
{
    const char *pre = NULL, *suf = NULL;
    int i;
    for (i = 0; i < it->naff; i++) {
        const char *w = affix_info[it->aff[i].type % AF_COUNT].word;
        if (it->aff[i].flags & AFX_IMPLICIT)
            continue;
        if (!strncmp(w, "OF ", 3)) suf = w;
        else pre = w;
    }
    if (lang_modifier_first()) {           /* "SEARING OF THE BEAR BOOTS" order */
        char mods[64];
        tjoin(mods, sizeof mods, pre ? pre : "", suf ? suf : "");
        tjoin(out, cap, mods, item_base_name(it));
        return;
    }
    snprintf(out, cap, "%s%s%s%s%s", pre ? pre : "", pre ? " " : "", item_base_name(it), suf ? " " : "",
             suf ? suf : "");
}

void item_name(char *out, size_t cap, const Item *it)
{
    switch (it->rarity) {
    case RAR_MAGIC:
        magic_name(out, cap, it);
        break;
    case RAR_RARE:
        tjoin(out, cap, rare_a[(it->name / 12) % 12], rare_b[it->name % 12]);
        break;
    case RAR_LEGEND: {
        const AspectDef *a = aspect_def(it->power);
        /* "ASPECT OF THE WARDEN" -> "GREAT HELM OF THE WARDEN" */
        if (lang_modifier_first())
            tjoin(out, cap, a ? a->name + 7 : "", item_base_name(it));
        else
            snprintf(out, cap, "%s %s", item_base_name(it), a ? a->name + 7 : "");
        break;
    }
    case RAR_UNIQUE:
    case RAR_MYTHIC: {
        const UniqueDef *u = unique_def(it->power);
        snprintf(out, cap, "%s", T(u ? u->name : item_base_name(it)));
        break;
    }
    default:
        snprintf(out, cap, "%s", item_base_name(it));
        break;
    }
}

static void pct_text(char *num, size_t cap, double v)
{
    if (v < 10.0)
        snprintf(num, cap, "%d.%d", (int)v, (int)(v * 10 + 0.5) % 10);
    else
        snprintf(num, cap, "%d", (int)(v + 0.5));
}

void affix_text(char *out, size_t cap, const Item *it, int i, int cls)
{
    const Affix *a = &it->aff[i];
    const AffixInfo *info = &affix_info[a->type % AF_COUNT];
    double v = item_affix_value(it, i);
    char num[16];
    if (a->type == AF_RANKS) {
        snprintf(out, cap, T("+%d RANKS TO %s"), (int)v, T(skill_def(cls, a->arg)->name));
    } else if (a->type == AF_LH_VULN) {
        pct_text(num, sizeof num, v);
        snprintf(out, cap, T("LUCKY HIT: %s%% CHANCE TO MAKE VULNERABLE"), num);
    } else if (a->type == AF_MAINSTAT) {
        fmt_num(num, sizeof num, v);
        snprintf(out, cap, "+%s %s", num, T(stat_name((MainStat)class_defs[cls % CLASS_COUNT].main_stat)));
    } else if (affix_is_percent((AffixType)a->type)) {
        pct_text(num, sizeof num, v);
        snprintf(out, cap, "+%s%% %s", num, T(info->label));
    } else {
        fmt_num(num, sizeof num, v);
        snprintf(out, cap, "+%s %s", num, T(info->label));
    }
}

void item_main_text(char *out, size_t cap, const Item *it)
{
    char num[16];
    if (it->main <= 0) {
        out[0] = '\0';
        return;
    }
    fmt_num(num, sizeof num, it->main * item_mw_mult(it));
    snprintf(out, cap, "%s %s", num, T(it->slot == SLOT_WEAPON ? "DAMAGE PER HIT" : "ARMOR"));
}

const char *slot_name(Slot s)
{
    static const char *const n[SLOT_COUNT] = { "WEAPON", "OFF-HAND", "HELM", "CHEST", "GLOVES", "PANTS", "BOOTS",
                                               "AMULET", "RING", "RING" };
    return n[s % SLOT_COUNT];
}

const char *rarity_name(Rarity r)
{
    static const char *const n[RAR_COUNT] = { "COMMON", "MAGIC", "RARE", "LEGENDARY", "UNIQUE", "MYTHIC UNIQUE" };
    return n[r % RAR_COUNT];
}

uint16_t rarity_color(Rarity r)
{
    switch (r) {
    case RAR_MAGIC:  return RGB565(110, 150, 255);
    case RAR_RARE:   return RGB565(255, 230, 90);
    case RAR_LEGEND: return RGB565(255, 140, 30);
    case RAR_UNIQUE: return RGB565(215, 185, 120);
    case RAR_MYTHIC: return RGB565(200, 120, 255);
    default:         return RGB565(220, 220, 220);
    }
}

/* ------------------------------------------------------------ tempering */

static const uint64_t temper_pool[TR_COUNT] = {
    [TR_WEAPONRY]  = B(AF_CRIT_DMG) | B(AF_VULN_DMG) | B(AF_OP_DMG) | B(AF_CORE_DMG) | B(AF_BASIC_DMG) | B(AF_DMG_CC),
    [TR_FINESSE]   = B(AF_CRIT) | B(AF_ATK_SPD) | B(AF_LUCKY) | B(AF_LH_VULN) | B(AF_DMG_CLOSE) | B(AF_DMG_FAR),
    [TR_ELEMENTS]  = ELEMENTS | B(AF_DOT),
    [TR_ENDURANCE] = B(AF_LIFE) | B(AF_ARMOR) | B(AF_DR) | B(AF_DR_CLOSE) | B(AF_RES_ALL) | B(AF_BARRIER),
    [TR_SUSTAIN]   = B(AF_LIFE_HIT) | B(AF_LIFE_KILL) | B(AF_REGEN) | B(AF_THORNS),
    [TR_PROFITEER] = B(AF_CDR) | B(AF_COST) | B(AF_RES_GEN) | B(AF_MOVE) | B(AF_GOLD) | B(AF_XP),
};

const char *temper_name(TemperRecipe t)
{
    static const char *const n[TR_COUNT] = { "WEAPONRY", "FINESSE", "ELEMENTS", "ENDURANCE", "SUSTENANCE",
                                             "PROFITEER" };
    return n[t % TR_COUNT];
}

bool temper_allowed(const Item *it, TemperRecipe t)
{
    Slot s = pool_slot((Slot)it->slot);
    bool offence = s == SLOT_WEAPON || s == SLOT_GLOVES || s == SLOT_RING1 || s == SLOT_AMULET || s == SLOT_OFFHAND;
    if (!it->used || t >= TR_COUNT)
        return false;
    if (t == TR_WEAPONRY || t == TR_FINESSE || t == TR_ELEMENTS)
        return offence;
    if (t == TR_ENDURANCE)
        return !offence || s == SLOT_AMULET || s == SLOT_OFFHAND;
    return true;
}

static int tempered_index(const Item *it, TemperRecipe t, int *count)
{
    int i, found = -1;
    *count = 0;
    for (i = 0; i < it->naff; i++)
        if (it->aff[i].flags & AFX_TEMPERED) {
            (*count)++;
            if ((temper_pool[t] >> it->aff[i].type) & 1u)
                found = i;
        }
    return found;
}

bool item_temper(Item *it, Rng *r, TemperRecipe t)
{
    int count, slot_i = tempered_index(it, t, &count), type;
    Item probe = *it;
    if (!temper_allowed(it, t) || it->temper_left == 0)
        return false;
    if (slot_i < 0 && (count >= MAX_TEMPER || it->naff >= MAX_AFFIX))
        return false;                     /* both temper slots hold other recipes */
    if (slot_i >= 0)
        probe.aff[slot_i].type = 0xFF;    /* the old roll may come back */
    type = pick_affix(&probe, r, temper_pool[t]);
    if (type < 0)
        return false;
    if (slot_i < 0)
        slot_i = it->naff++;
    set_affix(&it->aff[slot_i], r, type, it->ilvl, 0.8, 0);
    it->aff[slot_i].flags = AFX_TEMPERED;
    it->temper_left--;
    return true;
}

/* ---------------------------------------------------------- masterwork */

bool item_masterwork(Item *it, Rng *r)
{
    int tries;
    if (!it->used || it->mw >= MW_MAX)
        return false;
    it->mw++;
    if (it->mw % 4 != 0)
        return true;
    /* Ranks 4, 8 and 12: a critical upgrade on one random affix. */
    for (tries = 0; tries < 40; tries++) {
        int i = rng_range(r, 0, it->naff - 1);
        if (!(it->aff[i].flags & AFX_IMPLICIT) && it->aff[i].type != AF_RANKS) {
            it->aff[i].mwcrit++;
            break;
        }
    }
    return true;
}

/* ------------------------------------------------------------ enchanting */

bool item_enchant_options(const Item *it, int i, Rng *r, Affix out[2], int cls)
{
    const Affix *a;
    int k;
    if (!it->used || i < 0 || i >= it->naff)
        return false;
    a = &it->aff[i];
    if (a->flags & (AFX_IMPLICIT | AFX_TEMPERED) || it->rarity >= RAR_UNIQUE)
        return false;
    if (it->enchant != 0xFF && it->enchant != i)
        return false;                     /* only one affix per item can ever be enchanted */
    for (k = 0; k < 2; k++) {
        Item probe = *it;
        int t;
        if (k == 1)
            probe.aff[probe.naff++ % MAX_AFFIX] = out[0];
        t = pick_affix(&probe, r, slot_pool[pool_slot((Slot)it->slot)]);
        if (t < 0)
            return false;
        set_affix(&out[k], r, t, it->ilvl, it->rarity == RAR_MAGIC ? 0.8 : 1.0, cls);
        out[k].flags = AFX_ENCHANT;
    }
    return true;
}

void item_enchant_apply(Item *it, int i, const Affix *a)
{
    uint8_t crit;
    if (i < 0 || i >= it->naff)
        return;
    crit = it->aff[i].mwcrit;
    it->aff[i] = *a;
    it->aff[i].mwcrit = crit;             /* masterwork bonuses stay with the slot */
    it->enchant = (uint8_t)i;
}

/* ------------------------------------------------------------ imprinting */

bool item_can_imprint(const Item *it, int aspect, int cls)
{
    const AspectDef *a = aspect_def(aspect);
    return it->used && a && (it->rarity == RAR_RARE || it->rarity == RAR_LEGEND)
        && aspect_fits_slot(a, (Slot)it->slot) && aspect_usable(a, cls);
}

void item_imprint(Item *it, int aspect, int roll)
{
    it->rarity = RAR_LEGEND;
    it->power = (uint8_t)aspect;
    it->power_roll = (uint16_t)CLAMP(roll, 0, 1000);
}

/* ------------------------------------------------------------------ gems */

static const char *const gem_kind_names[GEM_KINDS] = { "RUBY", "SAPPHIRE", "EMERALD", "TOPAZ", "AMETHYST",
                                                       "DIAMOND", "SKULL" };
static const char *const gem_tier_names[GEM_TIERS] = { "CHIPPED", "FLAWED", "", "FLAWLESS", "ROYAL" };
static const double gem_tier_mult[GEM_TIERS] = { 1.0, 1.5, 2.0, 3.0, 4.0 };

/* [kind][group]: group 0 weapon, 1 armor, 2 jewelry. */
static const struct { uint8_t mod, arg; double v; bool scaled; } gem_table[GEM_KINDS][3] = {
    [GEM_RUBY]     = { { MOD_OP_DMG, 0, 8, false }, { MOD_LIFE_PCT, 0, 2, false }, { MOD_RES_ELEM, EL_FIRE, 4, false } },
    [GEM_SAPPHIRE] = { { MOD_ADD_CC, 0, 4, false }, { MOD_DR, 0, 1.5, false }, { MOD_RES_ELEM, EL_COLD, 4, false } },
    [GEM_EMERALD]  = { { MOD_VULN_DMG, 0, 6, false }, { MOD_THORNS_PCT, 0, 10, false },
                       { MOD_RES_ELEM, EL_POISON, 4, false } },
    [GEM_TOPAZ]    = { { MOD_ADD_BASIC, 0, 8, false }, { MOD_DR_CLOSE, 0, 2, false },
                       { MOD_RES_ELEM, EL_LIGHT, 4, false } },
    [GEM_AMETHYST] = { { MOD_ADD_DOT, 0, 6, false }, { MOD_REGEN, 0, 0.6, true }, { MOD_RES_ELEM, EL_SHADOW, 4, false } },
    [GEM_DIAMOND]  = { { MOD_ADD_CORE, 0, 5, false }, { MOD_BARRIER_GEN, 0, 4, false }, { MOD_RES_ALL, 0, 2, false } },
    [GEM_SKULL]    = { { MOD_LIFE_KILL, 0, 2, true }, { MOD_ARMOR_PCT, 0, 3, false }, { MOD_ARMOR_PCT, 0, 2, false } },
};

static int gem_group(Slot s)
{
    if (s == SLOT_WEAPON) return 0;
    if (s == SLOT_OFFHAND || s == SLOT_AMULET || s == SLOT_RING1 || s == SLOT_RING2) return 2;
    return 1;
}

static int gem_mods(int gem, Slot s, int ilvl, Mod *out, int cap)
{
    int kind = gem / GEM_TIERS, tier = gem % GEM_TIERS, g = gem_group(s);
    if (cap <= 0 || kind >= GEM_KINDS)
        return 0;
    out->kind = gem_table[kind][g].mod;
    out->arg = gem_table[kind][g].arg;
    out->value = gem_table[kind][g].v * gem_tier_mult[tier] * (gem_table[kind][g].scaled ? floor_scale(ilvl) : 1.0);
    return 1;
}

const char *gem_name(int gem)
{
    static char buf[48];
    int kind = (gem / GEM_TIERS) % GEM_KINDS, tier = gem % GEM_TIERS;
    tjoin(buf, sizeof buf, gem_tier_names[tier], gem_kind_names[kind]);
    return buf;
}

uint16_t gem_color(int kind)
{
    static const uint16_t c[GEM_KINDS] = {
        RGB565(235, 40, 60), RGB565(60, 110, 255), RGB565(40, 210, 90), RGB565(250, 200, 40),
        RGB565(170, 80, 230), RGB565(230, 240, 255), RGB565(200, 200, 180),
    };
    return c[kind % GEM_KINDS];
}

void gem_effect_text(char *out, size_t cap, int gem, Slot s)
{
    static const char *const label[GEM_KINDS][3] = {
        { "OVERPOWER DAMAGE", "MAXIMUM LIFE", "FIRE RESISTANCE" },
        { "DAMAGE TO CROWD CONTROLLED", "DAMAGE REDUCTION", "COLD RESISTANCE" },
        { "VULNERABLE DAMAGE", "THORNS", "POISON RESISTANCE" },
        { "BASIC SKILL DAMAGE", "DR FROM CLOSE ENEMIES", "LIGHTNING RESISTANCE" },
        { "DAMAGE OVER TIME", "LIFE PER SECOND", "SHADOW RESISTANCE" },
        { "CORE SKILL DAMAGE", "BARRIER GENERATION", "ALL RESISTANCE" },
        { "LIFE ON KILL", "ARMOR", "ARMOR" },
    };
    Mod m;
    char num[16];
    int kind = (gem / GEM_TIERS) % GEM_KINDS, g = gem_group(s);
    gem_mods(gem, s, 1, &m, 1);
    pct_text(num, sizeof num, m.value);
    snprintf(out, cap, "+%s%s %s", num, gem_table[kind][g].scaled ? "" : "%", T(label[kind][g]));
}
