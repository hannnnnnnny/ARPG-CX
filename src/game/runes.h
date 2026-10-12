/*
 * runes.h - runewords. Eight runes drop from elites and guardians: four
 * triggers (when) and four effects (what). Bound together at the
 * Runesmith they make one of sixteen runewords, each with its own name,
 * unknown until tried. One runeword is active at a time; binding costs
 * one rune of each kind. Every effect has a 1 s inner cooldown.
 */
#ifndef AD_RUNES_H
#define AD_RUNES_H

#include "world.h"

typedef enum {
    RN_NONE,
    RN_RAGE, RN_BLOOD, RN_SHADOW, RN_SWIFT,      /* triggers */
    RN_FROST, RN_FIRE, RN_THUNDER, RN_BONE,      /* effects */
    RUNE_KINDS
} RuneKind;

#define RUNE_TRIGGERS 4
#define RUNE_EFFECTS 4
#define RUNEWORDS (RUNE_TRIGGERS * RUNE_EFFECTS)
#define RUNE_RAGE_CRITS 5
#define RUNE_SWIFT_TICKS (3 * TICK_HZ)
#define RUNE_COOLDOWN TICK_HZ
#define RUNE_ELITE_PCT 2
#define RUNE_BOSS_PCT 30

static inline bool rune_is_trigger(int r) { return r >= RN_RAGE && r <= RN_SWIFT; }
static inline bool rune_is_effect(int r)  { return r >= RN_FROST && r <= RN_BONE; }
int runeword_index(int trigger, int effect);       /* 0..RUNEWORDS-1, -1 if not a pair */

const char *rune_name(int r);                      /* "FROST" */
uint16_t rune_color(int r);
const char *rune_text(int r);                      /* what the trigger waits for / the effect does */
const char *runeword_name(int trigger, int effect);

/* Binds a runeword: one rune of each is spent. False without them. */
bool prog_bind_runeword(Profile *p, int trigger, int effect);
bool runeword_found(const Profile *p, int trigger, int effect);
int  runewords_found(const Profile *p);

/* Combat hooks (world): crits, kills and blows feed the active trigger. */
void runes_on_crit(World *w, Profile *p, int target);
void runes_on_kill(World *w, Profile *p, const Monster *m);
void runes_on_hurt(World *w, Profile *p);
void runes_tick(World *w, Profile *p);
void runes_drop(World *w, Profile *p, const Monster *m);

#endif
