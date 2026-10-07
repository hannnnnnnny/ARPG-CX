/*
 * events.h - something worth watching on most floors (world internals):
 *
 *   treasure goblin   flees with a sack of gold, portals away after 12 s
 *   shrine            a 40 s blessing that carries over to the next floors
 *   ambush            a pack with a champion jumps the hero mid-floor
 *   cursed chest      touching it wakes guardians; beat them for the loot
 *   fallen adventurer carries one of the sixteen lost pages
 *   blood harvest, cursed shrine, bloodmarked hunt, hell rift (events_d4.c)
 *
 * plus champion elites with named affixes, and the glue that turns
 * dungeon happenings into bounty progress, achievements and remarks.
 */
#ifndef AD_EVENTS_H
#define AD_EVENTS_H

#include "world.h"
#include "goals.h"

#define SHRINE_TICKS (40 * TICK_HZ)

void events_init(World *w, const Profile *p);
void events_tick(World *w, Profile *p);
void events_on_kill(World *w, Profile *p, Monster *m);
/* The floor's object (shrine, chest, adventurer) is waiting to be touched. */
bool events_object_pending(const World *w);
void events_touch(World *w, Profile *p);
bool events_wave_active(const World *w);
void events_goblin_act(World *w, Monster *m);
/* HUD line for a running event ("" when there is nothing to show). */
void events_d4_status(const World *w, char *out, size_t cap);

int  champion_affixes(int floor);
void champion_roll(World *w, Monster *m);
void champion_name(char *out, size_t cap, const Monster *m);
const char *shrine_name(int kind);
const char *shrine_tag(int kind);    /* short HUD label */

/* Count a goal event; pays out and announces finished bounties. */
void world_goal(World *w, Profile *p, GoalEvent e, int arg);

#endif
