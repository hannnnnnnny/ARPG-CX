/*
 * skills.h - the class skill trees (Diablo IV style).
 *
 * Every class has 10 active skills in five clusters (basic, core,
 * defensive, mastery, ultimate), 6 passives and 3 key passives. An active
 * skill takes up to 5 rank points, then an Enhancement point, then a
 * choice between two Upgrades. Clusters open after enough points are spent
 * in the tree. Six skills fit on the action bar.
 *
 * Skills are data: a behaviour (arc, projectile, nova, chain, channel,
 * strike, ground, buff, summon, dash, corpse) plus numbers. Enhancements,
 * upgrades, passives, aspects and paragon only change numbers and flags
 * (see build.c), so combat has one code path per behaviour.
 */
#ifndef AD_SKILLS_H
#define AD_SKILLS_H

#include "defs.h"

typedef enum {
    SB_ARC, SB_PROJ, SB_NOVA, SB_CHAIN, SB_CHANNEL, SB_STRIKE, SB_GROUND, SB_BUFF, SB_SUMMON, SB_DASH, SB_CORPSE
} SkillBehavior;

/* Crowd control first, then damage over time kinds. */
typedef enum {
    ST_NONE, ST_FREEZE, ST_STUN, ST_CHILL, ST_IMMOBILIZE, ST_BURN, ST_POISON, ST_BLEED, ST_SHADOW, ST_COUNT
} StatusKind;
static inline bool status_is_dot(int s) { return s >= ST_BURN; }

/* Hero buffs: several can run at once (each has its own timer). */
typedef enum {
    BUFF_NONE, BUFF_BERSERK, BUFF_SPEED, BUFF_BARRIER, BUFF_CRIT, BUFF_IMBUE, BUFF_UNSTOP, BUFF_ULT, BUFF_COUNT
} BuffKind;

typedef enum { CAT_BASIC, CAT_CORE, CAT_DEFENSIVE, CAT_MASTERY, CAT_ULTIMATE, CAT_COUNT } SkillCat;

/* Skill families that key passives and aspects care about. */
typedef enum {
    TAG_NONE, TAG_BONE, TAG_BLOOD, TAG_EARTH, TAG_STORM, TAG_BEAST, TAG_JAGUAR, TAG_EAGLE, TAG_CENTIPEDE,
    TAG_MINION, TAG_SHOUT, TAG_IMBUE, TAG_COUNT
} SkillTag;

enum {
    RF_GROUND  = 1u << 0,  /* hits leave a damaging patch */
    RF_PIERCE  = 1u << 1,  /* projectiles pass through */
    RF_HEAL    = 1u << 2,  /* also heals 15% life */
    RF_FOLLOW  = 1u << 3,  /* ground effect follows the hero */
    RF_NOVA    = 1u << 4,  /* arc becomes a full circle */
    RF_VULN    = 1u << 5,  /* hits make enemies Vulnerable */
    RF_BERSERK = 1u << 6,  /* casting grants Berserking */
    RF_BARRIER = 1u << 7,  /* casting grants a Barrier */
    RF_UNSTOP  = 1u << 8,  /* casting grants Unstoppable */
    RF_WANDER  = 1u << 9,  /* projectiles wander toward enemies */
    RF_EXPLODE = 1u << 10, /* projectiles explode at the end */
    RF_HASTE   = 1u << 11, /* casting grants +20% attack speed */
    RF_BURST   = 1u << 12, /* projectiles fly in one line (no fan) */
    RF_ALT     = 1u << 13  /* summon: the alternate minion kind */
};

/* An Enhancement or Upgrade: deltas on top of the skill. */
typedef struct {
    const char *name;
    const char *desc;
    int8_t  d_count;      /* + projectiles / chains / hits / minions */
    int16_t d_radius;     /* + % radius */
    int16_t d_coef;       /* + % damage */
    int16_t d_cd;         /* + % cooldown (negative = faster) */
    int16_t d_dur;        /* + ticks duration */
    int8_t  status;       /* StatusKind applied (0 = keep) */
    int8_t  element;      /* -1 keep, else Element */
    uint16_t flags;       /* RF_* */
    int8_t  d_cost;       /* + % resource cost */
    int8_t  d_crit;       /* + % crit chance for this skill */
    int8_t  d_op;         /* + % overpower chance for this skill */
} RuneDef;

typedef struct {
    const char *name;
    const char *desc;
    uint8_t     cat;          /* SkillCat */
    uint8_t     behavior;     /* SkillBehavior */
    uint8_t     element;      /* Element */
    uint8_t     status;       /* StatusKind */
    uint8_t     buff;         /* BuffKind for SB_BUFF */
    uint8_t     tag;          /* SkillTag */
    uint8_t     icon;         /* IC_* glyph */
    double      coef;         /* weapon damage per hit (buff: strength in %) */
    double      cost;         /* resource spent; negative = generated */
    double      cooldown;     /* seconds */
    int16_t     radius, range, count, duration; /* duration in ticks */
    uint8_t     lucky;        /* lucky hit chance % */
    uint16_t    flags;        /* RF_* the skill always has */
    uint16_t    color;
    RuneDef     enh;
    RuneDef     upg[2];
} SkillDef;

typedef struct {
    const char *name;
    const char *desc;     /* per rank */
    uint8_t icon;
    uint8_t mod;          /* ModKind (build.h) */
    uint8_t arg;
    double  per_rank;
} PassiveDef;

typedef struct {
    const char *name;
    const char *desc;
} KeyPassiveDef;

/* Visual style of each class skill: class * CLASS_SKILLS + index. */
#define VX_CLASS(cls, i) ((uint8_t)((cls) * CLASS_SKILLS + (i)))
enum {
    VX_BURN = 70,      /* burning ground left by fire runes / powers */
    VX_POISON_POOL,    /* poison ground */
    VX_QUAKE_POOL,     /* earthquake ground from Leap Slam powers */
    VX_CORPSE_BOOM,    /* corpse explosion from Combustion etc. */
    VX_ENEMY,          /* enemy projectiles */
    VX_MINION_BOLT,    /* skeleton mage */
    VX_NONE = 255
};

/* Fully resolved parameters used by combat (built by build.c). */
typedef struct {
    bool     usable;       /* learned and on the bar */
    uint8_t  cat, behavior, element, status, buff, tag;
    uint16_t flags;
    uint8_t  vfx;          /* VX_* */
    int      rank;         /* points + gear ranks */
    double   coef;
    double   cost;         /* > 0 spends, < 0 generates */
    int      cd_ticks;
    int      radius, range, count, duration;
    double   crit_add, op_add, lucky;
    uint16_t color;
} SkillRT;

extern const SkillDef skill_defs[CLASS_COUNT][CLASS_SKILLS];
extern const PassiveDef passive_defs[CLASS_COUNT][CLASS_PASSIVES];
extern const KeyPassiveDef key_defs[CLASS_COUNT][CLASS_KEYS];
const SkillDef *skill_def(int cls, int i);
const char *skill_cat_name(SkillCat c);

/* Points that must be spent in the tree before a cluster opens. */
int  skill_cat_gate(SkillCat c);
int  skill_points_spent(const Profile *p);
bool skill_can_rank(const Profile *p, int i);
bool skill_rank_up(Profile *p, int i);
bool skill_can_enhance(const Profile *p, int i);
bool skill_enhance(Profile *p, int i);
bool skill_set_upgrade(Profile *p, int i, int upg);     /* 1 or 2; switching is free */
bool passive_can_rank(const Profile *p, int i);
bool passive_rank_up(Profile *p, int i);
bool key_passive_set(Profile *p, int k);                /* 1..CLASS_KEYS */
int  key_passive_gate(void);
bool skill_on_bar(const Profile *p, int i);
/* Put skill i on the bar (replacing bar slot 'slot'); false if not learned. */
bool skill_bar_set(Profile *p, int slot, int i);
void skill_refund_all(Profile *p);

const char *element_name(Element e);
uint16_t element_color(Element e);
const char *status_name(StatusKind s);

#endif
