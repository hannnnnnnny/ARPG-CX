/*
 * session.h - drives the world forever: floor clears, deaths, retreats.
 * Shared by the real game and the headless balance simulator, so the
 * simulator tests exactly the logic that ships.
 */
#ifndef AD_SESSION_H
#define AD_SESSION_H

#include "world.h"

typedef struct {
    World w;
    int floors_cleared;
    int deaths;
    int stuck_resets;   /* failsafe floor regenerations (should stay ~0) */
    int story_pending;  /* STORY_* event at the head of the queue, -1 = none */
    int story_queue[8]; /* further events waiting their turn */
    int story_count;
} Session;

enum { STORY_INTRO = 0, STORY_VICTORY = 10, STORY_EPILOGUE = 20 };

void session_start(Session *s, Profile *p);
void session_tick(Session *s, Profile *p);
/* Profile changed outside the world (equip, forge, rebirth...). */
void session_profile_changed(Session *s, const Profile *p);
/* Call once the pending story page has been shown: marks it seen and
 * advances the queue. */
void session_story_shown(Session *s, Profile *p);
int  story_seen_bit(int event);

#endif
