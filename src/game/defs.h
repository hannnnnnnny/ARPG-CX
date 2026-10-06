/*
 * defs.h - core data model of Ashen Depths.
 *
 * Profile = everything that is saved (hero, look, gear, bag, skill tree,
 * paragon board, codex of power, crafting materials, meta progress and
 * options). The running dungeon floor lives in world.h and is never saved:
 * floors are always regenerated.
 */
#ifndef AD_DEFS_H
#define AD_DEFS_H

#include "../core/common.h"

/* ----------------------------------------------- classes, elements, stats */

typedef enum {
    CLASS_BARBARIAN, CLASS_SORCERER, CLASS_ROGUE, CLASS_NECRO, CLASS_DRUID, CLASS_SPIRITBORN, CLASS_COUNT
} HeroClass;
typedef enum { EL_PHYS, EL_FIRE, EL_COLD, EL_LIGHT, EL_POISON, EL_SHADOW, EL_COUNT } Element;
typedef enum { STAT_STR, STAT_INT, STAT_DEX, STAT_WIL, STAT_COUNT } MainStat;
typedef enum { RES_FURY, RES_MANA, RES_ENERGY, RES_ESSENCE, RES_SPIRIT, RES_VIGOR, RES_COUNT } ResourceKind;

#define LEVEL_CAP 60
#define PARAGON_START 50     /* paragon points start flowing at this level */

/* ------------------------------------------------------------- items */

typedef enum {
    SLOT_WEAPON, SLOT_OFFHAND, SLOT_HELM, SLOT_CHEST, SLOT_GLOVES, SLOT_PANTS, SLOT_BOOTS,
    SLOT_AMULET, SLOT_RING1, SLOT_RING2, SLOT_COUNT
} Slot;
typedef enum { RAR_COMMON, RAR_MAGIC, RAR_RARE, RAR_LEGEND, RAR_UNIQUE, RAR_MYTHIC, RAR_COUNT } Rarity;

typedef enum {
    /* offensive */
    AF_MAINSTAT,    /* + main stat (flat, scales)          */
    AF_CRIT,        /* + % critical strike chance          */
    AF_CRIT_DMG,    /* + % critical strike damage          */
    AF_VULN_DMG,    /* + % vulnerable damage               */
    AF_OP_DMG,      /* + % overpower damage                */
    AF_ATK_SPD,     /* + % attack speed                    */
    AF_LUCKY,       /* + % lucky hit chance                */
    AF_DMG_CLOSE,   /* + % damage to close enemies         */
    AF_DMG_FAR,     /* + % damage to distant enemies       */
    AF_DMG_CC,      /* + % damage to crowd controlled      */
    AF_DMG_ELITE,   /* + % damage to elites                */
    AF_DOT,         /* + % damage over time                */
    AF_CORE_DMG,    /* + % core skill damage               */
    AF_BASIC_DMG,   /* + % basic skill damage              */
    AF_PHYS, AF_FIRE, AF_COLD, AF_LIGHT, AF_POISON, AF_SHADOW, /* + % element damage */
    AF_RANKS,       /* + N ranks to a skill (arg = skill)  */
    AF_LH_VULN,     /* lucky hit: % chance to make vulnerable */
    /* defensive */
    AF_LIFE,        /* + maximum life (flat, scales)       */
    AF_ARMOR,       /* + armor (flat, scales)              */
    AF_DR,          /* + % damage reduction                */
    AF_DR_CLOSE,    /* + % damage reduction from close     */
    AF_RES_ALL,     /* + % resistance to all elements      */
    AF_LIFE_HIT,    /* + life on hit (flat, scales)        */
    AF_LIFE_KILL,   /* + life on kill (flat, scales)       */
    AF_REGEN,       /* + life per second (flat, scales)    */
    AF_BARRIER,     /* + % barrier generation              */
    AF_THORNS,      /* + thorns (flat, scales)             */
    /* utility */
    AF_CDR,         /* + % cooldown reduction              */
    AF_COST,        /* + % resource cost reduction         */
    AF_RES_GEN,     /* + % resource generation             */
    AF_MAX_RES,     /* + maximum resource                  */
    AF_MOVE,        /* + % movement speed                  */
    AF_CC_DUR,      /* + % crowd control duration          */
    AF_GOLD,        /* + % gold found                      */
    AF_XP,          /* + % experience                      */
    AF_COUNT
} AffixType;

enum {
    AFX_GREATER  = 1u << 0, /* ancestral greater affix: 1.5x value      */
    AFX_TEMPERED = 1u << 1, /* added at the blacksmith                  */
    AFX_IMPLICIT = 1u << 2, /* comes with the item base                 */
    AFX_ENCHANT  = 1u << 3  /* rerolled at the occultist                */
};

#define MAX_AFFIX 9          /* implicit + 4 + 2 tempered (+ room) */
#define MAX_TEMPER 2
#define TEMPER_CHARGES 5
#define MW_MAX 12            /* masterwork ranks */

typedef struct {
    uint8_t type;
    uint8_t flags;   /* AFX_* */
    uint8_t arg;     /* skill index for AF_RANKS */
    uint8_t mwcrit;  /* masterwork critical upgrades landed on this affix */
    double  value;   /* rolled value (masterworking multiplies on top) */
} Affix;

/* Gems: 7 kinds x 5 tiers; what a gem does depends on where it sits. */
typedef enum { GEM_RUBY, GEM_SAPPHIRE, GEM_EMERALD, GEM_TOPAZ, GEM_AMETHYST, GEM_DIAMOND, GEM_SKULL, GEM_KINDS } GemKind;
#define GEM_TIERS 5
#define GEM_NONE 0xFF        /* socket is empty */
static inline uint8_t gem_id(int kind, int tier) { return (uint8_t)(kind * GEM_TIERS + tier); }

typedef struct {
    uint8_t  used;
    uint8_t  slot, rarity, naff;
    uint8_t  base;        /* base type index for the slot (see items.c) */
    uint8_t  name;        /* rare name index */
    uint8_t  locked;      /* never auto-salvaged */
    uint8_t  ancestral;
    uint8_t  power;       /* aspect id (legendary) or unique id (unique / mythic) */
    uint16_t power_roll;  /* 0..1000 within the power's value range */
    uint8_t  mw;          /* masterwork rank 0..MW_MAX */
    uint8_t  temper_left; /* tempering charges left */
    uint8_t  enchant;     /* affix index locked in by enchanting, 255 = none */
    uint8_t  sockets;     /* 0..2 */
    uint8_t  gem[2];      /* gem ids or GEM_NONE */
    uint16_t ilvl;        /* floor it dropped on */
    double   main;        /* weapon damage, or armor for armor pieces */
    Affix    aff[MAX_AFFIX];
} Item;

#define BAG_SIZE 36

/* ------------------------------------------------------------ skills */

#define CLASS_SKILLS 10      /* active skills per class */
#define CLASS_PASSIVES 6
#define CLASS_KEYS 3         /* key passives: only one can be active */
#define BAR_SLOTS 6          /* skills on the action bar */
#define SKILL_MAX_RANK 5     /* points; gear adds ranks on top */
#define PASSIVE_MAX_RANK 3
#define NO_SKILL 0xFF

/* ---------------------------------------------------------- paragon */

#define PARAGON_BOARDS 4
#define BOARD_N 15           /* boards are 15 x 15 tiles */
#define BOARD_BYTES ((BOARD_N * BOARD_N + 7) / 8)
#define GLYPH_COUNT 12       /* 2 per class */
#define GLYPH_MAX_LEVEL 100

/* ---------------------------------------------------- rebirth shop */

typedef enum {
    UP_MIGHT, UP_VIGOR, UP_GREED, UP_WISDOM, UP_FORTUNE, UP_HASTE, UP_START, UP_PATIENCE, UP_COUNT
} UpgradeId;

/* ---------------------------------------------------------- options */

typedef enum { MODE_PUSH, MODE_FARM } FloorMode;
typedef enum { DMGNUM_ALL, DMGNUM_BIG, DMGNUM_OFF, DMGNUM_COUNT } DamageNumbers;

/* ------------------------------------------------------- appearance */

#define NAME_LEN 12
typedef struct {
    uint8_t skin, hair, hair_color, face, eyes, cloth;
    uint8_t show_helm;
    char    name[NAME_LEN + 1];
} Look;

/* ---------------------------------------------------------- elixirs */

typedef enum { ELIX_NONE, ELIX_FORTITUDE, ELIX_PRECISION, ELIX_ADVANTAGE, ELIX_IRONBARB, ELIX_WISDOM, ELIX_COUNT } ElixirKind;

/* ------------------------------------------------------------ goals */

#define BOUNTY_SLOTS 3
/* An auto-tracked bounty (see goals.h); kind 0 = empty slot. */
typedef struct { uint8_t kind, arg; uint16_t need, have; } Bounty;

/* ---------------------------------------------------------- profile */

#define ASPECT_MAX 64        /* codex capacity (aspects defined in aspects.c) */

typedef struct {
    /* hero */
    uint8_t  cls;                   /* HeroClass */
    Look     look;
    int      level;
    double   xp;
    int      paragon_level;         /* levels gained past the cap */
    double   gold;
    int      floor, best_floor;
    Item     equip[SLOT_COUNT];
    Item     bag[BAG_SIZE];
    /* skill tree */
    uint8_t  skill_rank[CLASS_SKILLS];
    uint8_t  skill_enh[CLASS_SKILLS];  /* enhancement learned */
    uint8_t  skill_upg[CLASS_SKILLS];  /* 0 none, 1 or 2 = chosen upgrade */
    uint8_t  passive[CLASS_PASSIVES];
    uint8_t  key_passive;              /* 0 none, else 1..CLASS_KEYS */
    uint8_t  bar[BAR_SLOTS];           /* skill index or NO_SKILL */
    uint8_t  preset;                   /* build the auto planner follows (0..2) */
    int      skill_points;
    /* paragon */
    uint8_t  para[PARAGON_BOARDS][BOARD_BYTES];
    uint8_t  glyph[PARAGON_BOARDS];    /* socketed glyph id + 1, 0 = none */
    uint8_t  glyph_lvl[GLYPH_COUNT];
    uint16_t glyph_xp[GLYPH_COUNT];    /* Torment floors toward the next level */
    /* crafting */
    uint16_t codex[ASPECT_MAX];        /* best roll + 1 for each known aspect, 0 = unknown */
    double   iron, souls;              /* salvage materials */
    uint16_t gems[GEM_KINDS][GEM_TIERS];
    uint8_t  potion_lvl;               /* alchemist upgrades */
    uint8_t  elixir;                   /* ElixirKind */
    uint32_t elixir_secs;              /* seconds left */
    /* meta */
    uint32_t story_seen;               /* see story.h */
    double   embers;
    uint8_t  up[UP_COUNT];
    int      rebirths;
    int      best_floor_ever;
    /* goals: bounties, achievements (renown), lost pages, counters */
    Bounty   bounty[BOUNTY_SLOTS];
    uint64_t ach;                      /* achievement bits (goals.h) */
    uint64_t lore;                     /* lost pages found (story.h) */
    uint32_t n_goblins, n_shrines, n_events, n_elites, n_bounties, n_ancestral, n_mythic;
    /* options */
    uint8_t  auto_equip;
    uint8_t  salvage_upto;             /* auto-salvage rarities <= this; 255 = off */
    uint8_t  mode;                     /* FloorMode */
    uint8_t  auto_skills;              /* follow the build preset */
    uint8_t  auto_paragon;
    uint8_t  auto_craft;               /* masterwork, temper, imprint, gems, elixirs */
    uint8_t  dmg_numbers;              /* DamageNumbers */
    uint8_t  story_pause;              /* story shows a full page (else a subtitle) */
    uint8_t  show_fps;
    uint8_t  low_power;
    uint8_t  lang;                     /* Lang (i18n.h) */
    uint8_t  auto_battle;              /* the hero fights on its own when not steered */
    uint8_t  sound_vol;                /* 0..SOUND_VOLUMES-1 (sound.h) */
    /* idle bookkeeping */
    uint32_t save_time;                /* unix seconds of last save */
    double   kpm;                      /* recent kills per minute (for offline gains) */
    double   total_kills;
    double   play_seconds;
    uint32_t seed;
} Profile;

#endif
