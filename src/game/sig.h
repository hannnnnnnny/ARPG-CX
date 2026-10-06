/*
 * sig.h - signature builds: the three "meta" builds of the season, each
 * switched on by a build-defining unique and pushed further by two
 * aspects (facets). Unlike ordinary powers they have code-side behaviour,
 * their own effects and exaggerated numbers:
 *
 *   STORM WEREWOLF  (druid, Stormhowl Pelt)   Shred turns to lightning;
 *       every hit calls a bolt from the sky that heals and shields the
 *       hero, and Shred lunges at foes up to 80 px away.
 *   BONE SPEAR      (necromancer, Spine of the First Keeper)  spears
 *       burst into a fan of bone shards, crits erupt in a bone nova and
 *       every sixth spear is a giant one.
 *   INFERNO         (sorcerer, Heart of the Inferno)  fireballs explode
 *       twice and call meteors; burning foes explode when they die.
 */
#ifndef AD_SIG_H
#define AD_SIG_H

typedef enum { SIG_NONE, SIG_STORMWOLF, SIG_BONESPEAR, SIG_INFERNO, SIG_COUNT } Signature;

/* Aspect facets: each strengthens one signature. */
typedef enum {
    SF_CHAIN,     /* storm wolf: sky bolts chain to more foes */
    SF_LUNGE,     /* storm wolf: % life as barrier per bolt, longer lunges */
    SF_SHARDS,    /* bone spear: more shards, shards pierce */
    SF_MARROW,    /* bone spear: novas restore essence and life */
    SF_SHOWER,    /* inferno: meteors fall in showers */
    SF_PHOENIX,   /* inferno: burning deaths explode (% weapon damage) */
    SF_COUNT
} SigFacet;

/* The class skill each signature reshapes (skills.c indices). */
#define SIG_SKILL_SHRED     4    /* druid */
#define SIG_SKILL_BONESPEAR 2    /* necromancer */
#define SIG_SKILL_FIREBALL  2    /* sorcerer */

#endif
