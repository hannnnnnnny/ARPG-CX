/*
 * bark.h - one-line remarks over the battle: Brother Aldric speaking
 * through the torch he gave the hero, and the hero answering back. They
 * react to what just happened (a goblin, a mythic, a near death...) and
 * have cooldowns so they stay rare enough to notice.
 *
 * Lines are picked from the world tick, not the world RNG, so remarks
 * never change what drops or how a fight goes.
 */
#ifndef AD_BARK_H
#define AD_BARK_H

#include "defs.h"

typedef enum {
    BK_RECORD, BK_ANCESTRAL, BK_UNIQUE, BK_MYTHIC, BK_LOW_HP, BK_DEATH, BK_BOSS_DOWN, BK_GOBLIN, BK_GOBLIN_KILL,
    BK_GOBLIN_GONE, BK_SHRINE, BK_AMBUSH, BK_CHEST, BK_LORE, BK_CHAMPION, BK_BOUNTY, BK_ACHIEVE, BK_LEVEL,
    BK_IDLE, BK_COUNT
} BarkKind;

typedef enum { SPK_ALDRIC, SPK_HERO } Speaker;

#define BARK_VARIANTS 3

typedef struct {
    uint8_t speaker;
    const char *text;
} BarkLine;

extern const BarkLine bark_lines[BK_COUNT][BARK_VARIANTS];

/* Lives in the World but survives floor changes (cooldowns, the line). */
typedef struct {
    const BarkLine *line;
    int16_t t;                  /* ticks left on screen */
    int16_t idle_t;             /* ticks until the next idle remark */
    uint16_t cd_all;            /* ticks before any ordinary remark */
    uint16_t cd[BK_COUNT];      /* ticks before this kind again */
} BarkState;

/* Say something about 'kind' if its cooldowns allow; 'salt' varies the line. */
void bark(BarkState *b, BarkKind kind, uint32_t salt);
void bark_tick(BarkState *b, uint32_t salt);

#endif
