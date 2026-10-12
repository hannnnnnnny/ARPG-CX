/*
 * doors.h - the choice between floors. After a cleared floor three doors
 * open, each pairing a curse on the next floor with a reward for braving
 * it. A player at the controls picks one; an idle hero takes the door its
 * DoorPref names. Boss floors and the first floors have no doors.
 */
#ifndef AD_DOORS_H
#define AD_DOORS_H

#include "world.h"

typedef enum { DC_NONE, DC_MON_HP, DC_MON_DMG, DC_NO_ORBS, DC_ELITES, DC_COUNT } DoorCurse;
typedef enum { DR_NONE, DR_LOOT, DR_GOLD, DR_XP, DR_GOBLIN, DR_ELITE_LEGEND, DR_MYTHIC, DR_COUNT } DoorReward;
typedef enum { DP_SAFEST, DP_LOOT, DP_GOLD, DP_XP, DP_COUNT } DoorPref;

#define DOORS 3
#define DOOR_FLOOR 5                 /* doors open from this floor on */
#define DOOR_WAIT (10 * TICK_HZ)     /* then the hero takes its preferred door */
#define MYTHIC_ODDS_DOOR 25          /* per 100000 elite kills behind the mythic door */
#define DOOR_LOOT_LUCK 10            /* +100% to every non-common rarity weight */

/* Curse multipliers on the monsters of the floor. */
#define DOOR_HP_MULT 1.5
#define DOOR_DMG_MULT 1.4
#define DOOR_ELITE_PCT 25

/* Three doors with different rewards; each curse weighs about what its reward is worth. */
void doors_roll(Door out[DOORS], Rng *r);
int  doors_auto_pick(const Door d[DOORS], int pref);
bool doors_open_on(int floor);       /* a choice is offered before this floor */

const char *door_name(const Door *d);          /* "DOOR OF GREED" (named by its reward) */
const char *door_curse_text(int curse);
const char *door_reward_text(int reward);
const char *door_pref_name(int pref);
uint16_t door_color(const Door *d);

/* Monsters spawned later on a cursed floor (waves, ambushes) get the curse too. */
void door_on_spawn(World *w, Monster *m);
/* Puts the door's curse and reward on a freshly generated floor. */
void world_apply_door(World *w, Profile *p, Door d);

#endif
