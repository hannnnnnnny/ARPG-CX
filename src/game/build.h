/*
 * build.h - classes, build presets and the modifier model.
 *
 * Every build source - passives, key passives, legendary aspects, unique
 * powers, paragon nodes, glyphs, gems and item affixes - is reduced to
 * Mods ({kind, arg, value}). resolve_build() folds them into a BuildRT:
 * additive damage buckets, separate [x] multipliers (Diablo IV style),
 * defensive / utility totals and the final per-skill parameters.
 */
#ifndef AD_BUILD_H
#define AD_BUILD_H

#include "defs.h"
#include "skills.h"
#include "sig.h"
#include "myth_ids.h"

typedef enum {
    MOD_NONE,
    /* additive damage buckets (+%) */
    MOD_ADD_DMG, MOD_ADD_ELEM, MOD_ADD_CLOSE, MOD_ADD_FAR, MOD_ADD_CC, MOD_ADD_ELITE, MOD_ADD_DOT,
    MOD_ADD_CORE, MOD_ADD_BASIC, MOD_ADD_TAG,
    /* multiplicative [x] damage (each source multiplies separately) */
    MOD_X_ALL, MOD_X_ELEM, MOD_X_VULN, MOD_X_CRIT, MOD_X_OP, MOD_X_CC, MOD_X_DOT, MOD_X_CORE,
    MOD_X_SKILL, MOD_X_TAG, MOD_X_MINION,
    /* offence */
    MOD_MAINSTAT, MOD_STAT, MOD_CRIT, MOD_CRIT_DMG, MOD_VULN_DMG, MOD_OP_CHANCE, MOD_OP_DMG, MOD_ATK_SPD,
    MOD_LUCKY, MOD_LH_VULN, MOD_LH_STUN, MOD_LH_RES,
    /* defence */
    MOD_LIFE, MOD_LIFE_PCT, MOD_ARMOR, MOD_ARMOR_PCT, MOD_DR, MOD_DR_CLOSE, MOD_RES_ALL, MOD_RES_ELEM,
    MOD_LIFE_HIT, MOD_LIFE_KILL, MOD_HEAL_KILL, MOD_REGEN, MOD_BARRIER_GEN, MOD_THORNS, MOD_THORNS_PCT,
    MOD_BARRIER_CD,
    /* utility */
    MOD_CDR, MOD_COST, MOD_RES_GEN, MOD_MAX_RES, MOD_RES_KILL, MOD_MOVE, MOD_CC_DUR, MOD_GOLD, MOD_XP,
    /* skill reshapers (arg = class skill index) */
    MOD_SK_COUNT, MOD_SK_RADIUS, MOD_SK_CD, MOD_SK_COST, MOD_SK_DUR, MOD_SK_FLAGS, MOD_SK_STATUS, MOD_SK_ELEM,
    MOD_SK_RANKS, MOD_ALL_RANKS,
    /* summons */
    MOD_MINION_COUNT,
    /* signature builds (sig.h): arg = Signature / SigFacet */
    MOD_SIGNATURE, MOD_SIG_FACET,
    /* mythic powers (mythic.h): arg = MythicPower, value = its strength */
    MOD_MYTHIC,
    MOD_COUNT
} ModKind;

typedef struct { uint8_t kind, arg; double value; } Mod;

/* Key passives: one per profile, with code-side behaviour in combat. */
typedef enum {
    KP_NONE,
    KP_BERSERKER, KP_HEMORRHAGE, KP_TITAN,          /* barbarian */
    KP_COMBUSTION, KP_SHATTER, KP_OVERCHARGE,       /* sorcerer */
    KP_DEADEYE, KP_VIRULENCE, KP_HAIL,              /* rogue */
    KP_MARROW, KP_COMMANDER, KP_BLOODBOND,          /* necromancer */
    KP_EARTHSPIRIT, KP_STORMCALLER, KP_PRIMAL,      /* druid */
    KP_FEROCITY, KP_EAGLE, KP_PLAGUE,               /* spiritborn */
    KP_COUNT
} KeyPassive;

typedef struct {
    const char *name;
    const char *desc;
    uint8_t bar[BAR_SLOTS];          /* skills, in spending priority order */
    uint8_t upg[BAR_SLOTS];          /* upgrade picked for each (1 or 2) */
    uint8_t passives[CLASS_PASSIVES];/* passive priority (NO_SKILL ends) */
    uint8_t key;                     /* 1..CLASS_KEYS */
    uint8_t element;                 /* main damage element */
    uint8_t aspects[4];              /* aspect ids the auto crafter imprints */
    uint8_t glyph;                   /* preferred glyph (class-local 0/1) */
} BuildPreset;

#define PRESETS 3

typedef struct {
    const char *name;
    const char *desc;
    const char *style;               /* "MELEE - TOUGH" */
    uint8_t res_kind;                /* ResourceKind */
    uint8_t main_stat;               /* MainStat */
    double  hp_mult;
    int     attack_range;            /* px the hero closes to before attacking */
    uint16_t res_color;
    BuildPreset preset[PRESETS];
} ClassDef;

extern const ClassDef class_defs[CLASS_COUNT];
const char *resource_name(ResourceKind r);
const char *stat_name(MainStat s);

/* Additive bucket indices of BuildRT.add[] */
enum { ADD_ALL, ADD_ELEM0, ADD_CLOSE = ADD_ELEM0 + EL_COUNT, ADD_FAR, ADD_CC, ADD_ELITE, ADD_DOT, ADD_CORE,
       ADD_BASIC, ADD_TAG0, ADD_COUNT = ADD_TAG0 + TAG_COUNT };

typedef struct {
    SkillRT skill[CLASS_SKILLS];
    double  add[ADD_COUNT];           /* + % */
    double  x_all, x_vuln, x_crit, x_op, x_cc, x_dot, x_core, x_minion;   /* multipliers (1 = none) */
    double  x_elem[EL_COUNT], x_skill[CLASS_SKILLS], x_tag[TAG_COUNT];
    double  stat[STAT_COUNT], mainstat_flat;
    double  crit, crit_dmg, vuln_dmg, op_chance, op_dmg, atk_spd, lucky, lh_vuln, lh_stun, lh_res;
    double  life, life_pct, armor, armor_pct, dr, dr_close, res_all, res[EL_COUNT];
    double  life_hit, life_kill, heal_kill, regen, barrier_gen, thorns, thorns_pct, barrier_cd;
    double  cdr, cost, res_gen, max_res, res_kill, move, cc_dur, gold, xp;
    int     minion_add;
    int     ranks[CLASS_SKILLS];      /* bonus ranks from gear */
    /* per-skill reshapers from aspects / uniques */
    int     sk_count[CLASS_SKILLS], sk_dur[CLASS_SKILLS];
    double  sk_radius[CLASS_SKILLS], sk_cd[CLASS_SKILLS], sk_cost[CLASS_SKILLS];
    uint16_t sk_flags[CLASS_SKILLS];
    uint8_t sk_status[CLASS_SKILLS], sk_elem[CLASS_SKILLS];   /* 0 / 0xFF = keep */
    uint8_t key;                      /* KeyPassive */
    uint8_t sig;                      /* Signature from the build-defining unique */
    double  sig_power;                /* its % weapon damage */
    double  facet[SF_COUNT];          /* aspect facets feeding it */
    double  x_life;                   /* [x] maximum life (1 = none) */
    double  myth[MY_COUNT];           /* strength of each mythic power worn, 0 = none */
    uint8_t myth_any;                 /* at least one mythic power worn */
} BuildRT;

void build_clear(BuildRT *b);
/* Fold one modifier into the totals. */
void build_add_mod(BuildRT *b, int kind, int arg, double value);
/* Passives + key passive + skills of the profile; gear / paragon mods
 * must already be in 'b' (ranks, cdr and skill reshapers need them). */
void resolve_build(const Profile *p, BuildRT *b);
static inline bool has_key(const BuildRT *b, KeyPassive k) { return b->key == k; }

/* Auto planner: spend points along the class preset p->preset. */
void build_auto_spend(Profile *p);
/* Switch to another preset: full refund, then follow it. */
void build_apply_preset(Profile *p, int preset);

#endif
