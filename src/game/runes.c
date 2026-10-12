/*
 * runes.c - runewords (see runes.h).
 */
#include "runes.h"
#include "world_int.h"
#include "skills.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"
#include <stdio.h>

int runeword_index(int trigger, int effect)
{
    if (!rune_is_trigger(trigger) || !rune_is_effect(effect))
        return -1;
    return (trigger - RN_RAGE) * RUNE_EFFECTS + (effect - RN_FROST);
}

const char *rune_name(int r)
{
    static const char *const n[RUNE_KINDS] = { "", "RAGE", "BLOOD", "SHADOW", "SWIFT", "FROST", "FIRE", "THUNDER",
                                               "BONE" };
    return n[r % RUNE_KINDS];
}

uint16_t rune_color(int r)
{
    static const uint16_t c[RUNE_KINDS] = {
        0, RGB565(236, 96, 84), RGB565(200, 40, 50), RGB565(180, 120, 255), RGB565(112, 222, 120),
        RGB565(140, 200, 255), RGB565(255, 140, 30), RGB565(255, 230, 90), RGB565(220, 210, 180),
    };
    return c[r % RUNE_KINDS];
}

const char *rune_text(int r)
{
    static const char *const n[RUNE_KINDS] = {
        "", "EVERY 5TH CRITICAL HIT", "ON A KILL", "WHEN YOU ARE HIT", "EVERY 3 SECONDS IN BATTLE",
        "A FROST NOVA THAT FREEZES (120%)", "A FIRE BURST THAT BURNS (200%)", "LIGHTNING STRIKES 3 FOES (150% EACH)",
        "A BONE BARRIER (10% LIFE) AND NOVA (80%)",
    };
    return n[r % RUNE_KINDS];
}

const char *runeword_name(int trigger, int effect)
{
    static const char *const n[RUNEWORDS] = {
        "FROSTRAGE", "WILDFIRE", "STORMFURY", "BONECRACK",          /* rage */
        "COLD HARVEST", "PYRE", "BLOOD THUNDER", "OSSUARY",         /* blood */
        "RIME SHELL", "SPITE", "RETRIBUTION", "MARROW WARD",        /* shadow */
        "WINTER STEP", "EMBER TRAIL", "STATIC", "BONE RHYTHM",      /* swift */
    };
    int i = runeword_index(trigger, effect);
    return i < 0 ? "" : n[i];
}

/* ---------------------------------------------------------------- profile */

bool runeword_found(const Profile *p, int trigger, int effect)
{
    int i = runeword_index(trigger, effect);
    return i >= 0 && (p->rune_found >> i) & 1u;
}

int runewords_found(const Profile *p)
{
    int i, n = 0;
    for (i = 0; i < RUNEWORDS; i++)
        n += (p->rune_found >> i) & 1u;
    return n;
}

bool prog_bind_runeword(Profile *p, int trigger, int effect)
{
    int i = runeword_index(trigger, effect);
    if (i < 0 || p->runes[trigger] == 0 || p->runes[effect] == 0)
        return false;
    p->runes[trigger]--;
    p->runes[effect]--;
    p->rune_trig = (uint8_t)trigger;
    p->rune_eff = (uint8_t)effect;
    p->rune_found |= (uint16_t)(1u << i);
    return true;
}

/* ---------------------------------------------------------------- combat */

static Hit rune_hit(const World *w, double coef, int element, int status, int dur)
{
    Hit h = { 0 };
    h.base = w->st.weapon * coef;
    h.element = (uint8_t)element;
    h.status = (uint8_t)status;
    h.status_dur = (int16_t)dur;
    h.skill = NO_SKILL;
    h.rune = 1;                            /* rune hits never feed a trigger */
    return h;
}

static void nova(World *w, Profile *p, int x, int y, int r, const Hit *h, uint16_t c)
{
    int i;
    effect(w, FX_NOVA, x, y, 0, 0, r, 12, c);
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && dist_px(FX_FROM_INT(x), FX_FROM_INT(y), w->mon[i].x, w->mon[i].y) <= r)
            deal_damage(w, p, i, h);
}

static void thunder(World *w, Profile *p)
{
    Hit h = rune_hit(w, 1.5, EL_LIGHT, ST_NONE, 0);
    uint64_t done = 0;
    int k, i;
    for (k = 0; k < 3; k++) {
        int best = -1, bd = 101;
        for (i = 0; i < w->nmon; i++)
            if (w->mon[i].alive && !((done >> i) & 1u)) {
                int d = dist_px(w->h.x, w->h.y, w->mon[i].x, w->mon[i].y);
                if (d < bd) { bd = d; best = i; }
            }
        if (best < 0)
            return;
        done |= (uint64_t)1 << best;
        effect(w, FX_SKYBOLT, FX_TO_INT(w->mon[best].x), FX_TO_INT(w->mon[best].y), 0, 0, 10, 10, rune_color(RN_THUNDER));
        deal_damage(w, p, best, &h);
    }
}

/* The bound effect, at (x, y) for bursts or around the hero. */
static void fire_effect(World *w, Profile *p, int x, int y)
{
    int hxp = FX_TO_INT(w->h.x), hyp = FX_TO_INT(w->h.y);
    Hit h;
    if (w->rune_cd > 0 || !rune_is_effect(p->rune_eff))
        return;
    w->rune_cd = RUNE_COOLDOWN;
    switch (p->rune_eff) {
    case RN_FROST:
        h = rune_hit(w, 1.2, EL_COLD, ST_FREEZE, TICK_HZ * 3 / 2);
        nova(w, p, hxp, hyp, 40, &h, rune_color(RN_FROST));
        break;
    case RN_FIRE:
        h = rune_hit(w, 2.0, EL_FIRE, ST_BURN, 3 * TICK_HZ);
        nova(w, p, x, y, 30, &h, rune_color(RN_FIRE));
        break;
    case RN_THUNDER:
        thunder(w, p);
        break;
    default:
        hero_add_barrier(w, 0.10);
        h = rune_hit(w, 0.8, EL_PHYS, ST_NONE, 0);
        nova(w, p, hxp, hyp, 28, &h, rune_color(RN_BONE));
        break;
    }
}

void runes_on_crit(World *w, Profile *p, int target)
{
    if (p->rune_trig != RN_RAGE || ++w->rune_crits < RUNE_RAGE_CRITS)
        return;
    w->rune_crits = 0;
    fire_effect(w, p, FX_TO_INT(w->mon[target].x), FX_TO_INT(w->mon[target].y));
}

void runes_on_kill(World *w, Profile *p, const Monster *m)
{
    if (p->rune_trig == RN_BLOOD)
        fire_effect(w, p, FX_TO_INT(m->x), FX_TO_INT(m->y));
}

void runes_on_hurt(World *w, Profile *p)
{
    if (p->rune_trig == RN_SHADOW)
        fire_effect(w, p, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y));
}

void runes_tick(World *w, Profile *p)
{
    if (w->rune_cd > 0)
        w->rune_cd--;
    if (p->rune_trig != RN_SWIFT || ++w->rune_swift < RUNE_SWIFT_TICKS)
        return;
    w->rune_swift = 0;
    if (count_near(w, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y), 60) > 0)
        fire_effect(w, p, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y));
}

void runes_drop(World *w, Profile *p, const Monster *m)
{
    char buf[48];
    int pct = m->special == MS_BUTCHER ? 100 : m->boss ? RUNE_BOSS_PCT : m->elite ? RUNE_ELITE_PCT : 0, r;
    if (pct == 0 || rng_range(&w->rng, 0, 99) >= pct)
        return;
    r = rng_range(&w->rng, RN_RAGE, RN_BONE);
    if (p->runes[r] < 9999)
        p->runes[r]++;
    snprintf(buf, sizeof buf, T("RUNE OF %s"), T(rune_name(r)));
    floater_kind(w, FX_TO_INT(m->x), FX_TO_INT(m->y) - 22, buf, rune_color(r), FL_BIG);
    world_sound(w, SND_DROP_RARE);
}
