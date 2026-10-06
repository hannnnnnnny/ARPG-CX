/*
 * sound.h - sound effect ids shared by the game and the platforms.
 *
 * Game code never plays audio itself: the world collects the sounds of a
 * tick in a bitmask (so a pack of twenty hits is one hit sound) and the
 * game layer hands them to plat_sound(). Platforms without audio (the
 * calculator, the headless runner, tests) implement it as a no-op, so the
 * simulation stays exactly the same with or without sound.
 */
#ifndef AD_SOUND_H
#define AD_SOUND_H

typedef enum {
    SND_HIT, SND_CRIT, SND_KILL, SND_ELITE_DIE, SND_BOSS_DIE, SND_HURT, SND_DEATH, SND_LEVEL,
    SND_POTION, SND_DROP, SND_DROP_RARE, SND_DROP_LEGEND, SND_DROP_UNIQUE, SND_GOLD,
    SND_CAST_PHYS, SND_CAST_FIRE, SND_CAST_COLD, SND_CAST_LIGHTNING, SND_CAST_POISON, SND_CAST_SHADOW,
    SND_STAIRS, SND_GOBLIN, SND_SHRINE, SND_CHEST, SND_AMBUSH, SND_ACHIEVE, SND_BOUNTY,
    SND_UI_MOVE, SND_UI_OK, SND_UI_BACK, SND_UI_ERROR, SND_PAGE,
    SND_COUNT
} SoundId;

#define SOUND_VOLUMES 4              /* 0 off, 1 low, 2 medium, 3 high */

#endif
