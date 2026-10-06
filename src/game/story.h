/*
 * story.h - the original campaign of Ashen Depths: ten acts that follow
 * the five dungeon themes twice (acts VI-X are the Torment campaign), each
 * with an intro, a named act boss on its tenth floor and a victory text,
 * plus an epilogue after act V, a finale after act X and sixteen lost
 * pages found on the floors. Past floor 100 the descent goes on as endless
 * "Torment" with generated boss names.
 */
#ifndef AD_STORY_H
#define AD_STORY_H

#include "defs.h"

#define ACT_COUNT 10
#define CAMPAIGN_ACTS 5      /* the first campaign ends with the epilogue */
#define STORY_LINES 7
#define LORE_COUNT 16

typedef struct {
    const char *title;
    const char *intro[STORY_LINES];   /* NULL-terminated */
    const char *boss_name;
    uint8_t     boss_type;            /* MonType */
    const char *victory[STORY_LINES];
} ActDef;

typedef struct {
    const char *title;
    const char *lines[4];             /* NULL-terminated */
} LorePage;

extern const ActDef act_defs[ACT_COUNT];
extern const char *const epilogue[STORY_LINES];
extern const char *const finale[STORY_LINES];
extern const LorePage lore_pages[LORE_COUNT];

/* Story events: what the subtitle queue, full pages and the journal show. */
enum {
    STORY_INTRO = 0, STORY_VICTORY = 10, STORY_EPILOGUE = 20, STORY_FINALE = 21,
    STORY_LORE = 32, STORY_EVENT_END = STORY_LORE + LORE_COUNT
};

/* Act index for a floor (0..9), or -1 past the campaign (endless). */
int  story_act(int floor);
bool story_is_act_start(int floor);
bool story_is_act_boss(int floor);
/* Boss display name for a guardian floor. */
void story_boss_name(char *out, size_t cap, int floor);
int  story_boss_type(int floor);

/* An event counts as seen once it has been shown (lost pages: found). */
bool story_event_seen(const Profile *p, int ev);
void story_mark_seen(Profile *p, int ev);
int  story_lore_found(const Profile *p);
/* A lost page the hero has not read yet (picked with 'roll'), or -1. */
int  story_unread_lore(const Profile *p, uint32_t roll);

#endif
