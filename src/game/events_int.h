/* events_int.h - helpers shared by events.c and events_d4.c (not public API). */
#ifndef AD_EVENTS_INT_H
#define AD_EVENTS_INT_H

#include "world.h"

/* A reachable floor cell at least 'min_path' steps from the hero. */
bool event_find_cell(World *w, int min_path, int *cx, int *cy);
int  event_wave_type(World *w);
/* A pack that knows where the hero is, around cell (cx, cy). */
void spawn_wave(World *w, int cx, int cy, int n, int champs);
/* Gold worth 'kills' monsters, with a floating number at (x, y). */
void event_bonus_gold(World *w, Profile *p, double kills, int x, int y);

/* events_d4.c: blood harvest, cursed shrine, bloodmarked hunt, hell rift */
void events_d4_init(World *w, int kind);
void events_d4_tick(World *w, Profile *p);
void events_d4_on_kill(World *w, Profile *p, const Monster *m);
void events_d4_touch(World *w, Profile *p);
/* True when it handled the end of an event wave. */
bool events_d4_wave_cleared(World *w, Profile *p);
/* A timed event is under way: the hero sees it through before the stairs. */
bool events_d4_running(const World *w);

#endif
