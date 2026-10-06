/*
 * world_sig.c - the three signature builds in combat (see sig.h).
 *
 * Every power here throws hits marked 'sig', which never trigger another
 * signature power, so chains stay finite. Per-tick and per-second limits
 * keep the screen readable and the calculator fast while the numbers stay
 * huge: signature hits use the build's own multipliers on top of a large
 * % of weapon damage.
 */
#include "world_int.h"
#include "skills.h"
#include "aspects.h"
#include "story.h"
#include "items.h"
#include "../core/sound.h"
#include "../gfx/gfx.h"
#include <string.h>

#define HEAL_WINDOW   TICK_HZ   /* healing is capped per second */
#define HEAL_CAP      0.15      /* at most 15% life a second from bolts / novas */
#define LUNGE_RANGE   80        /* Shred jumps at foes this far away */
#define METEOR_FALL   14        /* ticks from the warning circle to the impact */
#define C_STORM  RGB565(140, 220, 255)
#define C_BONEW  RGB565(240, 232, 210)
#define C_FIREY  RGB565(255, 140, 40)

static const BuildRT *bt(const World *w) { return &w->st.b; }
static int mx(const Monster *m) { return FX_TO_INT(m->x); }
static int my(const Monster *m) { return FX_TO_INT(m->y); }

/* A signature hit: 'pct' % weapon damage carrying skill i's multipliers. */
static Hit sig_hit(const World *w, int i, double pct, int element)
{
    Hit h = skill_hit(w, i, 1.0);
    h.base = w->st.weapon * pct / 100.0;
    h.element = (uint8_t)element;
    h.sig = 1;
    h.status_dur = 0;
    return h;
}

static void sig_area(World *w, Profile *p, int x, int y, int r, const Hit *h)
{
    int i;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && dist_px(FX_FROM_INT(x), FX_FROM_INT(y), w->mon[i].x, w->mon[i].y) <= r)
            deal_damage(w, p, i, h);
}

void sig_shake(World *w, int ticks, int px)
{
    if (w->sig.shake_t <= 0)
        w->sig.shake_px = 0;
    w->sig.shake_t = (int16_t)MAX(w->sig.shake_t, ticks);
    w->sig.shake_px = (int16_t)MAX(w->sig.shake_px, px);
}

static void sig_flash(World *w, uint16_t c, int ticks)
{
    w->sig.flash_t = (int16_t)ticks;
    w->sig.flash_color = c;
}

/* Heal a fraction of life, within the per-second cap. */
static void sig_heal(World *w, double frac)
{
    double room = HEAL_CAP - w->sig.healed;
    if (room <= 0 || w->h.dead_t > 0)
        return;
    frac = MIN(frac, room);
    w->sig.healed += frac;
    w->h.hp = MIN(w->st.max_hp, w->h.hp + w->st.max_hp * frac);
}

/* ------------------------------------------------------- storm werewolf */

static int nearest_other(const World *w, int x, int y, int r, uint64_t skip)
{
    int i, best = -1, bd = r + 1;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && !((skip >> i) & 1u)) {
            int d = dist_px(FX_FROM_INT(x), FX_FROM_INT(y), w->mon[i].x, w->mon[i].y);
            if (d < bd) {
                bd = d;
                best = i;
            }
        }
    return best;
}

/* A bolt from the sky on the target, chaining on through the pack. */
static void storm_call(World *w, Profile *p, int first, bool crit)
{
    Hit h = sig_hit(w, SIG_SKILL_SHRED, bt(w)->sig_power, EL_LIGHT);
    uint64_t hit = 0;
    int k, cur = first, px = mx(&w->mon[first]), py = my(&w->mon[first]);
    int links = 2 + (int)bt(w)->facet[SF_CHAIN];       /* the bolt forks: two foes, more with the aspect */
    for (k = 0; k < links && cur >= 0; k++) {
        Monster *m = &w->mon[cur];
        hit |= (uint64_t)1 << cur;
        effect(w, FX_SKYBOLT, mx(m), my(m), 0, 0, 10, 12, C_STORM);
        if (k > 0)
            effect(w, FX_BOLT, px, py, mx(m), my(m), 0, 8, C_STORM);
        px = mx(m);
        py = my(m);
        deal_damage(w, p, cur, &h);
        sig_heal(w, 0.02);
        if (bt(w)->facet[SF_LUNGE] > 0)
            hero_add_barrier(w, bt(w)->facet[SF_LUNGE] / 100.0);
        cur = nearest_other(w, px, py, 70, hit);
    }
    w->h.haste_t = 2 * TICK_HZ;                     /* the frenzy of the storm */
    if (crit) {
        hero_buff(w, BUFF_UNSTOP, 0, TICK_HZ);
        sig_shake(w, 4, 2);
    }
    world_sound(w, SND_CAST_LIGHTNING);
}

/* Shred out of reach: leap at the target, hitting everything on the way. */
bool sig_lunge(World *w, int i, const Monster *tgt)
{
    fx tx, ty;
    if (bt(w)->sig != SIG_STORMWOLF || i != SIG_SKILL_SHRED || !tgt || w->h.dash_t > 0)
        return false;
    if (dist_px(w->h.x, w->h.y, tgt->x, tgt->y) > LUNGE_RANGE
        || !body_line_clear(w, w->h.x, w->h.y, tgt->x, tgt->y, HERO_HALF))
        return false;
    tx = tgt->x;
    ty = tgt->y;
    w->h.dash_t = 6;
    w->h.dash_skill = i;
    w->h.dash_hit = 0;
    step_toward(w->h.x, w->h.y, tx, ty, FX_FROM_INT(dist_px(w->h.x, w->h.y, tx, ty) + 4) / 6,
                &w->h.dash_vx, &w->h.dash_vy);
    w->h.face = tx > w->h.x ? 1 : -1;
    effect(w, FX_DASH, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y), FX_TO_INT(tx), FX_TO_INT(ty), 12, 10, C_STORM);
    return true;
}

bool sig_reach(const World *w, int i, const Monster *tgt)
{
    return bt(w)->sig == SIG_STORMWOLF && i == SIG_SKILL_SHRED && tgt
        && dist_px(w->h.x, w->h.y, tgt->x, tgt->y) <= LUNGE_RANGE;
}

/* ---------------------------------------------------------- bone spear */

static void bone_nova(World *w, Profile *p, int x, int y, double pct)
{
    Hit h = sig_hit(w, SIG_SKILL_BONESPEAR, pct, EL_PHYS);
    effect(w, FX_SHARDS, x, y, 0, 0, 32, 14, C_BONEW);
    sig_area(w, p, x, y, 32, &h);
    sig_heal(w, 0.01);                              /* the marrow feeds the necromancer */
    if (bt(w)->facet[SF_MARROW] > 0) {
        hero_gain_res(w, bt(w)->facet[SF_MARROW]);
        sig_heal(w, 0.02);
    }
    world_sound(w, SND_CRIT);
}

/* The spear's first hit bursts into a fan of shards flying on. */
void sig_spear_burst(World *w, Proj *pj)
{
    int k, n = 3 + (int)bt(w)->facet[SF_SHARDS];
    if (bt(w)->sig != SIG_BONESPEAR || pj->hit.sig || (pj->sig & (SIGP_BURST | SIGP_SHARD))
        || pj->hit.skill != SIG_SKILL_BONESPEAR)
        return;
    pj->sig |= SIGP_BURST;
    for (k = 0; k < n; k++) {
        Proj *s = spawn_proj(w);
        int a = (k - (n - 1) / 2) * 4;              /* fan around the flight direction */
        if (!s)
            return;
        s->kind = PJ_SKILL;
        s->x = s->px = pj->x;
        s->y = s->py = pj->y;
        s->vx = (fx)(((int64_t)pj->vx * (64 - ABS(a)) + (int64_t)pj->vy * a * 2) / 64);
        s->vy = (fx)(((int64_t)pj->vy * (64 - ABS(a)) - (int64_t)pj->vx * a * 2) / 64);
        s->life = 24;
        s->hit = sig_hit(w, SIG_SKILL_BONESPEAR, bt(w)->sig_power * 0.35, EL_PHYS);
        s->pierce = bt(w)->facet[SF_SHARDS] > 0;
        s->vfx = pj->vfx;
        s->sig = SIGP_SHARD;
    }
}

/* Every sixth spear is a giant: four times the damage, a nova where it stops. */
void sig_spear_cast(World *w, Proj *pj)
{
    if (bt(w)->sig != SIG_BONESPEAR || pj->hit.skill != SIG_SKILL_BONESPEAR || pj->hit.sig)
        return;
    if (++w->sig.spears % 6)
        return;
    pj->sig |= SIGP_GIANT;
    pj->hit.base *= 4.0;
    pj->explode = 1;
    sig_shake(w, 6, 2);
}

void sig_spear_end(World *w, Profile *p, const Proj *pj)
{
    if (!(pj->sig & SIGP_GIANT))
        return;
    bone_nova(w, p, FX_TO_INT(pj->x), FX_TO_INT(pj->y), bt(w)->sig_power * 2.0);
    sig_shake(w, 10, 3);
    sig_flash(w, C_BONEW, 3);
}

/* -------------------------------------------------------------- inferno */

static void call_meteor(World *w, int x, int y, int delay)
{
    int i;
    for (i = 0; i < MAX_METEOR; i++)
        if (!w->sig.met[i].alive) {
            SigMeteor *m = &w->sig.met[i];
            m->alive = 1;
            m->t = (int16_t)(METEOR_FALL + delay);
            m->x = (int16_t)x;
            m->y = (int16_t)y;
            effect(w, FX_WARN, x, y, x, y - 40, 28, METEOR_FALL + delay, C_FIREY);
            return;
        }
}

/* Fireball explosion: a second, wider blast, and maybe a meteor shower. */
void sig_fireball(World *w, Profile *p, const Proj *pj)
{
    int x = FX_TO_INT(pj->x), y = FX_TO_INT(pj->y), k, n;
    Hit h;
    if (bt(w)->sig != SIG_INFERNO || pj->hit.sig || pj->hit.skill != SIG_SKILL_FIREBALL)
        return;
    h = sig_hit(w, SIG_SKILL_FIREBALL, bt(w)->sig_power * 0.5, EL_FIRE);
    h.status = ST_BURN;
    h.status_dur = 3 * TICK_HZ;
    effect(w, FX_FIRERING, x, y, 0, 0, pj->radius * 3 / 2 + 6, 14, C_FIREY);
    sig_area(w, p, x, y, pj->radius * 3 / 2 + 6, &h);
    if (rng_range(&w->rng, 0, 99) >= 35)
        return;
    n = MAX(1, (int)bt(w)->facet[SF_SHOWER]);
    for (k = 0; k < n; k++)
        call_meteor(w, x + (k ? rng_range(&w->rng, -30, 30) : 0), y + (k ? rng_range(&w->rng, -24, 24) : 0), k * 5);
}

static void meteor_land(World *w, Profile *p, const SigMeteor *m)
{
    Hit h = sig_hit(w, SIG_SKILL_FIREBALL, bt(w)->sig_power, EL_FIRE);
    Hit g = h;
    h.status = ST_BURN;
    h.status_dur = 3 * TICK_HZ;
    effect(w, FX_METEOR, m->x, m->y, 0, 0, 30, 16, C_FIREY);
    sig_area(w, p, m->x, m->y, 30, &h);
    g.base *= 0.15;
    g.status = ST_BURN;
    world_spawn_ground(w, m->x, m->y, 22, 90, &g, false, VX_BURN);
    sig_shake(w, 8, 3);
    sig_flash(w, RGB565(255, 120, 40), 2);
    world_sound(w, SND_BOSS_DIE);
}

/* ------------------------------------------------------------- drops */

/* The build-defining unique of a class, 0 if it has none. */
int sig_unique_for(int cls)
{
    int i;
    for (i = 1; i <= unique_count; i++)
        if (unique_def(i)->kind == MOD_SIGNATURE && unique_def(i)->cls == cls)
            return i;
    return 0;
}

static Item *wearing(Profile *p, int uid)
{
    int s;
    for (s = 0; s < SLOT_COUNT; s++)
        if (p->equip[s].used && p->equip[s].rarity == RAR_UNIQUE && p->equip[s].power == uid)
            return &p->equip[s];
    return NULL;
}

/* Worn, the unique grows with the hero: each guardian raises it to the
 * floor's item level (rolls, tempers and masterwork stay). */
static bool empower(World *w, Profile *p, Item *it)
{
    if (it->ilvl >= w->floor)
        return false;
    item_rescale(it, w->floor);
    world_refresh_stats(w, p);
    world_message(w, "BUILD-DEFINING UNIQUE EMPOWERED", RGB565(255, 160, 40));
    return true;
}

/* Guardians carry the build-defining unique of the hero's class: act
 * bosses often, the endless Torment guardians now and then. */
void sig_boss_drop(World *w, Profile *p, const Monster *m)
{
    int uid = sig_unique_for(p->cls), chance = story_is_act_boss(w->floor) ? 30 : 8;
    Item *worn = uid ? wearing(p, uid) : NULL;
    if (!m->boss || uid == 0 || (worn && empower(w, p, worn)) || worn || rng_range(&w->rng, 0, 99) >= chance)
        return;
    world_drop_unique(w, p, m->x, m->y, uid);
    world_banner(w, "BUILD-DEFINING UNIQUE!", RGB565(255, 160, 40));
    sig_shake(w, 10, 3);
}

/* -------------------------------------------------------------- hooks */

/* After a skill hit landed (not a DoT, not itself a signature hit). */
void sig_on_hit(World *w, Profile *p, int mi, const Hit *h, bool crit)
{
    const Monster *m = &w->mon[mi];
    if (h->sig || h->dot)
        return;
    if (bt(w)->sig == SIG_STORMWOLF && h->skill == SIG_SKILL_SHRED && w->sig.last_bolt != w->tick + 1) {
        w->sig.last_bolt = w->tick + 1;
        storm_call(w, p, mi, crit);
    } else if (bt(w)->sig == SIG_BONESPEAR && h->skill == SIG_SKILL_BONESPEAR && crit
               && (w->sig.last_nova == 0 || w->tick + 1 - w->sig.last_nova >= 3)) {
        w->sig.last_nova = w->tick + 1;
        bone_nova(w, p, mx(m), my(m), bt(w)->sig_power);
    }
}

/* Phoenix: burning foes explode when they die (a few a tick at most). */
void sig_on_kill(World *w, Profile *p, const Monster *m)
{
    Hit h;
    if (bt(w)->sig != SIG_INFERNO || bt(w)->facet[SF_PHOENIX] <= 0 || m->dot_t[0] <= 0)
        return;
    if (w->sig.last_boom != w->tick + 1) {
        w->sig.last_boom = w->tick + 1;
        w->sig.booms = 0;
    }
    if (++w->sig.booms > 3)
        return;
    h = sig_hit(w, SIG_SKILL_FIREBALL, bt(w)->facet[SF_PHOENIX], EL_FIRE);
    effect(w, FX_FIRERING, mx(m), my(m), 0, 0, 26, 12, C_FIREY);
    sig_area(w, p, mx(m), my(m), 26, &h);
    sig_heal(w, 0.01);
}

void sig_tick(World *w, Profile *p)
{
    int i;
    if (w->sig.shake_t > 0 && --w->sig.shake_t == 0)
        w->sig.shake_px = 0;
    if (w->sig.flash_t > 0)
        w->sig.flash_t--;
    if (--w->sig.heal_t <= 0) {
        w->sig.heal_t = HEAL_WINDOW;
        w->sig.healed = 0;
    }
    for (i = 0; i < MAX_METEOR; i++) {
        SigMeteor *m = &w->sig.met[i];
        if (m->alive && --m->t == 6)
            effect(w, FX_FALL, m->x, m->y, 0, 0, 30, 6, C_FIREY);
        if (m->alive && m->t <= 0) {
            m->alive = 0;
            meteor_land(w, p, m);
        }
    }
}
