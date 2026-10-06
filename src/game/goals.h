/*
 * goals.h - short and long goals for an idle game that never ends:
 *
 *   bounties      three auto-tracked tasks ("hunt 60 ghouls"); a finished
 *                 one pays out and is replaced at once
 *   achievements  37 milestones; every 4 earned raise the hero's renown,
 *                 which adds damage and life permanently
 *
 * Pure profile logic (no world access) so it is unit-tested directly.
 */
#ifndef AD_GOALS_H
#define AD_GOALS_H

#include "defs.h"
#include "../core/rng.h"

typedef enum {
    BT_NONE, BT_KILL_TYPE, BT_ELITES, BT_FLOORS, BT_GOBLINS, BT_SHRINES, BT_EVENTS, BT_LEGENDARY, BT_BOSS,
    BT_COUNT
} BountyKind;

/* Things that happen in the dungeon; goals_note() counts them. */
typedef enum {
    GE_KILL, GE_ELITE, GE_FLOOR, GE_GOBLIN, GE_SHRINE, GE_EVENT, GE_LEGENDARY, GE_BOSS, GE_ANCESTRAL, GE_MYTHIC
} GoalEvent;

#define ACH_COUNT 37
#define RENOWN_PER_TIER 4
#define RENOWN_DMG_PCT 2.0    /* damage and life per renown tier */

typedef enum {
    AK_FLOOR, AK_LEVEL, AK_PARAGON, AK_KILLS, AK_ELITES, AK_GOBLINS, AK_SHRINES, AK_EVENTS, AK_BOUNTIES,
    AK_LORE, AK_CODEX, AK_ANCESTRAL, AK_MYTHIC, AK_STORY, AK_REBIRTH, AK_HOURS, AK_COUNT
} AchKind;

typedef struct {
    const char *name;
    uint8_t     kind;          /* AchKind */
    uint32_t    need;
} AchDef;

extern const AchDef ach_defs[ACH_COUNT];

/* Fill empty bounty slots with new, distinct bounties. */
void goals_refill(Profile *p, Rng *r);
/* Count an event; returns one bit per bounty slot that just completed
 * (the slot keeps have == need until goals_complete() rerolls it). */
int  goals_note(Profile *p, GoalEvent e, int arg);
/* Pay out a finished bounty (gold and materials) and roll its successor. */
double goals_complete(Profile *p, int slot, Rng *r);
void bounty_text(char *out, size_t cap, const Bounty *b);
bool bounty_sane(const Bounty *b);

/* Achievements newly earned are marked; returns the first new id or -1. */
int  goals_check_achievements(Profile *p);
uint32_t ach_progress(const Profile *p, int id);
void ach_text(char *out, size_t cap, int id);
static inline bool ach_earned(const Profile *p, int id) { return (p->ach >> id) & 1u; }
int  ach_count(const Profile *p);
static inline int renown_tier(const Profile *p) { return ach_count(p) / RENOWN_PER_TIER; }

#endif
