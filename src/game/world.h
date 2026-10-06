/*
 * world.h - the dungeon floor currently being fought through.
 *
 * Nothing here is saved: floors are regenerated from the profile. All
 * storage is fixed-size, so a floor transition never allocates.
 */
#ifndef AD_WORLD_H
#define AD_WORLD_H

#include "defs.h"
#include "stats.h"
#include "skills.h"
#include "bark.h"
#include "../core/sound.h"
#include "../core/fixed.h"
#include "../core/rng.h"

#define MAP_W 48
#define MAP_H 32
#define MAX_MON    40
#define MAX_PROJ   48
#define MAX_DROP   12
#define MAX_FLOAT  40
#define MAX_FX     48
#define MAX_GROUND 12
#define MAX_ALLY   9
#define MAX_CORPSE 16
#define MAX_ORB    6
#define DOT_KINDS  4            /* burn, poison, bleed, shadow */

enum { CELL_WALL, CELL_FLOOR, CELL_STAIRS };

typedef enum { MT_SKELETON, MT_BAT, MT_GHOUL, MT_SPIDER, MT_IMP, MT_CULTIST, MT_GOLEM, MT_COUNT } MonType;

typedef struct {
    const char *name;
    double hp, dmg;      /* multipliers on the floor's base values */
    fx speed;            /* px per tick */
    uint8_t ranged;
    uint8_t element;     /* element of its attacks (resistances apply) */
    int min_floor;
    int atk_ticks;
} MonDef;

extern const MonDef mon_defs[MT_COUNT];

/* Champion affixes (elites): one below floor 30, two from there on. */
enum { CH_FAST = 1, CH_STURDY = 2, CH_VAMPIRIC = 4, CH_VOLATILE = 8, CH_WARDED = 16, CH_FRENZIED = 32 };
#define CH_KINDS 6

typedef struct {
    uint8_t alive, type, elite, boss, aggro;
    uint8_t champ;       /* CH_* bits */
    uint8_t goblin;      /* treasure goblin: flees, then portals away */
    uint8_t wave;        /* part of an event wave (ambush, cursed chest) */
    int8_t  face;
    fx      x, y;
    fx      px, py;      /* position at the previous tick (render interpolation) */
    double  hp, max_hp, dmg;
    int16_t atk_cd, flash, anim, slam_cd;
    int16_t freeze, stun, chill, immob, vuln;          /* status timers (ticks) */
    int16_t dot_t[DOT_KINDS];
    double  dot_dps[DOT_KINDS];
} Monster;

static inline bool mon_cc(const Monster *m) { return m->freeze > 0 || m->stun > 0 || m->chill > 0 || m->immob > 0; }
static inline bool mon_dotted(const Monster *m)
{
    return m->dot_t[0] > 0 || m->dot_t[1] > 0 || m->dot_t[2] > 0 || m->dot_t[3] > 0;
}

/* One hit, before the damage pipeline (see deal_damage). */
typedef struct {
    double   base;        /* weapon damage x skill damage */
    uint8_t  element, status;
    int16_t  status_dur;
    uint8_t  skill;       /* class skill index, NO_SKILL for weapon / thorns */
    uint16_t flags;       /* RF_VULN ... */
    bool     dot, minion;
    uint8_t  sig;         /* thrown by a signature power: never triggers another */
    double   crit_add, op_add, lucky;
} Hit;

typedef enum { PJ_SKILL, PJ_ENEMY, PJ_MINION } ProjKind;
typedef struct {
    uint8_t alive, kind;
    uint8_t pierce, ground, vfx, explode, wander;
    uint8_t sig;             /* SIGP_* (world_sig.c) */
    int16_t radius;          /* explosion radius, 0 = single target */
    int16_t life;
    fx x, y, vx, vy, px, py;
    uint64_t hit_mask;       /* monsters already pierced */
    Hit hit;
    double enemy_dmg;
    uint8_t enemy_el;
} Proj;

/* A damaging patch on the floor (earthquake, poison cloud, burning ground...). */
typedef struct {
    uint8_t alive, follow, vfx;
    int16_t r, t, dur, every;
    fx x, y;
    Hit hit;                  /* per pulse */
    uint16_t color;
} Ground;

typedef struct { uint8_t alive; fx x, y; int16_t t; Item item; } Drop;

typedef enum { FL_TEXT, FL_DMG, FL_BIG, FL_MEGA } FloatKind;   /* FL_MEGA: signature hits */
typedef struct { uint8_t alive, kind; int16_t x, y, t; uint16_t color; char text[32]; } Floater;

typedef enum { FX_SLASH, FX_BOOM, FX_NOVA, FX_BOLT, FX_WHIRL, FX_WARN, FX_METEOR, FX_PUFF, FX_HEAL, FX_LEVEL,
               FX_DASH, FX_RAISE, FX_SKYBOLT, FX_SHARDS, FX_FIRERING, FX_FALL } FxKind;
typedef struct { uint8_t alive, kind, vfx; int16_t x, y, x2, y2, t, dur, r; uint16_t color; } Effect;

typedef enum { AK_SKELETON, AK_MAGE, AK_WOLF } AllyKind;
typedef struct {
    uint8_t alive, kind, element, skill;
    int8_t  face;
    fx x, y, px, py;
    int16_t atk_cd, anim, flash, rise;
} Ally;

typedef struct { uint8_t alive; fx x, y; int16_t t; } Corpse;
typedef struct { uint8_t alive; fx x, y; int16_t t; } Orb;  /* health potion on the ground */

typedef enum { TGT_NONE, TGT_MON, TGT_DROP, TGT_STAIRS, TGT_OBJECT } TargetKind;

/* Signature builds (world_sig.c): meteors on their way, screen shake and
 * flash, and per-second limits that keep the effects readable. */
#define MAX_METEOR 8
typedef struct { uint8_t alive; int16_t t, x, y; } SigMeteor;
typedef struct {
    SigMeteor met[MAX_METEOR];
    int16_t shake_t, shake_px, flash_t;
    uint16_t flash_color;
    int16_t heal_t;          /* ticks left in the current healing window */
    double healed;           /* fraction of life healed in it (capped) */
    int last_bolt, last_nova, last_boom, booms;  /* tick + 1 of the last of each (0 = never) */
    int spears;              /* bone spears cast (every sixth is a giant) */
} SigState;

/* What the player asks of the hero this tick (set by the game layer from
 * keyboard and mouse; all zero on the calculator and in simulations). */
typedef struct {
    bool active;          /* any steering input this tick */
    int8_t mx, my;        /* WASD direction, -1..1 */
    bool click;           /* left button went down this tick */
    bool hold;            /* left button held */
    fx tx, ty;            /* cursor in world pixels */
    int8_t cast;          /* skill bar slot to cast, -1 none */
    bool potion;
} HeroCommand;

/* Manual control state, kept while the player steers. */
typedef struct {
    int manual_t;         /* ticks of manual control left (auto battle resumes at 0) */
    int target;           /* monster to attack, -1 none */
    int drop;             /* drop to pick up, -1 none */
    bool dest;            /* walking to (dx, dy) */
    fx dx, dy;
    int8_t cast;          /* queued bar slot, -1 none */
    int16_t cast_t;       /* ticks the queued cast keeps trying */
    int16_t stairs_msg_t;
} ControlRT;

/* One event per floor at most (events.c). */
typedef enum { EV_NONE, EV_GOBLIN, EV_SHRINE, EV_AMBUSH, EV_CHEST, EV_FALLEN, EV_COUNT } FloorEventKind;
typedef enum { SH_BLESSED, SH_LETHAL, SH_GREED, SH_WISDOM, SH_FRENZY, SH_PROTECT, SH_COUNT } ShrineKind;
typedef enum { ES_WAITING, ES_RUNNING, ES_DONE } EventState;
typedef struct {
    uint8_t kind, state, shrine;  /* FloorEventKind, EventState, ShrineKind */
    int16_t t;                    /* goblin: ticks left before its portal */
    int cx, cy;                   /* the object's cell (shrine, chest, fallen adventurer) */
    int wave_left;                /* event monsters still alive */
} FloorEvent;

/* Last notable hit, broken down by damage bucket (HERO > COMBAT page). */
typedef struct {
    double base, stat, add, mult, vuln, crit, op, total;
    uint8_t skill, element;
    bool is_vuln, is_crit, is_op;
} HitLog;

typedef struct {
    double hits, crits, vulns, ops, total;
    double sig;               /* damage from signature powers */
} HitStats;

typedef struct {
    fx  x, y;
    fx  px, py;           /* previous tick position (render interpolation) */
    double hp;
    double barrier;       /* absorbs damage first */
    int barrier_t;
    double res;           /* fury / mana / energy / essence / spirit / vigor */
    int atk_cd;
    int skill_cd[CLASS_SKILLS];
    int channel_t, channel_skill;
    int strike_t, strike_x, strike_y, strike_skill, strike_hits;
    int dash_t, dash_skill;
    fx dash_vx, dash_vy;
    uint64_t dash_hit;
    int buff_t[BUFF_COUNT];
    double buff_val[BUFF_COUNT];
    uint8_t imbue_el, imbue_st;
    uint16_t imbue_flags;
    uint8_t imbue_skill;
    int haste_t;
    int potions, potion_cd;
    int stacks, stacks_t; /* ferocity / berserker stacks */
    int target_kind, target_idx;
    int8_t face;
    int anim, flash, dead_t, attack_t, cast_t;
    int stuck;            /* ticks left following the grid path after a snag */
    int idle_ticks;       /* ticks without a kill/pickup: failsafe floor reset */
    int engage;           /* px the hero closes to before attacking */
    bool moving;
} HeroRT;

typedef struct {
    uint8_t cell[MAP_H][MAP_W];
    uint8_t var[MAP_H][MAP_W];
    uint8_t seen[MAP_H][MAP_W];
    int16_t fh[MAP_H][MAP_W];  /* path distance from the hero (monsters chase with it) */
    int16_t ft[MAP_H][MAP_W];  /* path distance from the hero's target */
    int floor, theme;
    int cls;
    char boss_name[64];
    uint16_t accent;           /* build colour: staff orb, buff glow */
    bool boss_floor;
    int quota, kills;
    int stairs_x, stairs_y;    /* cell */
    HeroRT h;
    Monster mon[MAX_MON];
    int nmon;
    Proj pj[MAX_PROJ];
    Drop dr[MAX_DROP];
    Floater fl[MAX_FLOAT];
    Effect fx[MAX_FX];
    Ground gr[MAX_GROUND];
    Ally al[MAX_ALLY];
    Corpse co[MAX_CORPSE];
    Orb orb[MAX_ORB];
    Stats st;
    Rng rng;
    int tick;
    int ft_target_cell;
    uint8_t dmg_numbers;       /* DamageNumbers option */
    /* Message line shown over the battle view (loot, level ups...). */
    char msg[112];
    uint16_t msg_color;
    int msg_t;
    /* Big centred banner for ancestral / unique / mythic drops. */
    char banner[80];
    uint16_t banner_color;
    int banner_t;
    HitLog last_big;
    HitStats hs;               /* this floor's hit statistics */
    FloorEvent ev;
    SigState sig;
    HeroCommand cmd;
    ControlRT ctl;
    /* Carried across floors: the shrine blessing and the remarks. */
    uint8_t shrine;            /* ShrineKind of the active blessing */
    int shrine_t;              /* ticks left, 0 = none */
    BarkState bark;
    /* Kill-rate tracking for offline gains. */
    int minute_ticks, minute_kills;
    /* Events for the game layer, cleared each tick. */
    bool ev_died, ev_floor_done, ev_stuck;
    int ev_lore;               /* lost page found: page + 1 (the session queues it) */
    uint64_t snd;              /* SoundId bits of this tick, played by the game layer */
} World;

void world_init_floor(World *w, const Profile *p, int floor);
void world_tick(World *w, Profile *p);
void world_refresh_stats(World *w, const Profile *p);
void world_message(World *w, const char *text, uint16_t color);
void world_banner(World *w, const char *text, uint16_t color);
static inline void world_sound(World *w, int id) { w->snd |= (uint64_t)1 << id; }
static inline bool world_shrine(const World *w, int kind) { return w->shrine_t > 0 && w->shrine == kind; }
/* Remember current positions as "previous" (start of tick / after spawning). */
void world_snapshot_positions(World *w);
double world_max_res(const World *w);

/* Exposed for tests. */
bool world_walkable(const World *w, int cx, int cy);
int  world_alive_monsters(const World *w);

static inline int px_to_cell(fx v) { return FX_TO_INT(v) >> TILE_SHIFT; }
static inline fx cell_center(int c) { return FX_FROM_INT(c * TILE_SIZE + TILE_SIZE / 2); }

#endif
