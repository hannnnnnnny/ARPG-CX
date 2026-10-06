/*
 * story.h - the original campaign of Ashen Depths: five acts that follow
 * the five dungeon themes, each with an intro, a named act boss on its
 * tenth floor, and a victory text. Past floor 50 the descent continues as
 * endless "Torment" with generated boss names.
 */
#ifndef AD_STORY_H
#define AD_STORY_H

#include "defs.h"

#define ACT_COUNT 5
#define STORY_LINES 7

typedef struct {
    const char *title;
    const char *intro[STORY_LINES];   /* NULL-terminated */
    const char *boss_name;
    uint8_t     boss_type;            /* MonType */
    const char *victory[STORY_LINES];
} ActDef;

extern const ActDef act_defs[ACT_COUNT];
extern const char *const epilogue[STORY_LINES];

/* Act index for a floor (0..4), or -1 past the campaign (endless). */
int  story_act(int floor);
bool story_is_act_start(int floor);
bool story_is_act_boss(int floor);
/* Boss display name for a guardian floor. */
void story_boss_name(char *out, size_t cap, int floor);
int  story_boss_type(int floor);

/* story_seen bits: 0-4 act intros, 5-9 act victories, 10 epilogue. */
static inline bool story_seen(uint16_t seen, int bit) { return (seen >> bit) & 1u; }

#endif
