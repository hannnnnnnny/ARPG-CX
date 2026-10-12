/*
 * session.h - drives the world forever: floor clears, deaths, retreats.
 * Shared by the real game and the headless balance simulator, so the
 * simulator tests exactly the logic that ships.
 */
#ifndef AD_SESSION_H
#define AD_SESSION_H

#include "world.h"
#include "doors.h"

typedef struct {
    World w;
    int floors_cleared;
    int deaths;
    int stuck_resets;   /* failsafe floor regenerations (should stay ~0) */
    int story_pending;  /* STORY_* event (story.h) at the head of the queue, -1 = none */
    int story_queue[8]; /* further events waiting their turn */
    int story_count;
    Door doors[DOORS];  /* the choice before the current floor */
    int door_t;         /* ticks left to choose (world held still), 0 = no choice pending */
    int door_sel;       /* highlighted door; taken when time runs out */
} Session;

void session_start(Session *s, Profile *p);
void session_tick(Session *s, Profile *p);
/* Profile changed outside the world (equip, forge, rebirth...). */
void session_profile_changed(Session *s, const Profile *p);
/* Call once the pending story page has been shown: marks it seen and
 * advances the queue. */
void session_story_shown(Session *s, Profile *p);
/* Go through door i (0..DOORS-1) of the pending choice. */
void session_take_door(Session *s, Profile *p, int i);

#endif
