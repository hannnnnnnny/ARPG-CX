/*
 * trial.h - the daily trial. One per calendar day (UTC): a seed and a
 * rule pair (one curse, one boon from the door pool) picked by the date,
 * so everyone faces the same floors and rules. The hero starts 20 floors
 * above its best and has ten minutes to go as deep as it can. The run is
 * played on a copy of the profile: nothing it finds or spends is kept,
 * only the day's best and a share code to post.
 */
#ifndef AD_TRIAL_H
#define AD_TRIAL_H

#include "game.h"

#define TRIAL_TICKS (10 * 60 * TICK_HZ)
#define TRIAL_BACK 20               /* floors above the best the trial starts */

uint32_t trial_day(uint32_t now);
Door trial_rule(uint32_t day);
uint32_t trial_seed(uint32_t day);
int  trial_start_floor(const Profile *p);
/* "AD-4E2B-180-27-3-5C1": day, start floor, floors gone down, class, check. */
void trial_code(char *out, size_t cap, uint32_t day, int start, int floors, int cls);
bool trial_code_valid(uint32_t day, int start, int floors, int cls, unsigned check);

void trial_begin(Game *g, uint32_t now);
void trial_tick(Game *g);            /* once per battle tick while running */
void trial_end(Game *g);             /* time is up or the player stops */
/* The profile saves go to: the real one, never the trial's copy. */
Profile *trial_saved_profile(Game *g);

void trial_render_hud(const Game *g);
void trial_render_card(const Game *g, uint32_t now);   /* today's rules, from GOALS */
void trial_render_result(const Game *g);

#endif
