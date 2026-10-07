/*
 * world_myth.c - mythic powers in combat (see mythic.h).
 *
 * Mythic hits carry the core skill's multipliers on top of a large % of
 * weapon damage and are marked (Hit.sig = MYTH_HIT) so they never set off
 * another power: chains stay finite. Explosions share a per-tick budget so
 * a pack dying at once stays readable and cheap on the calculator.
 */
#include "mythic.h"
#include "world_int.h"
#include "aspects.h"
#include "balance.h"
#include "story.h"
#include "skills.h"
#include "../core/sound.h"
#include "../gfx/gfx.h"

#define MYTH_HIT       2                /* Hit.sig value of a mythic hit */
#define BURSTS_PER_TICK 4
#define C_STAR   RGB565(255, 236, 150)
#define C_VOID   RGB565(170, 90, 255)
#define C_TIME   RGB565(150, 210, 255)
#define C_BLOOD  RGB565(230, 40, 50)
#define C_DRAGON RGB565(255, 120, 30)
#define C_THUNDR RGB565(170, 230, 255)

static double pw(const World *w, int power) { return w->st.b.myth[power]; }
static int mx(const Monster *m) { return FX_TO_INT(m->x); }
static int my(const Monster *m) { return FX_TO_INT(m->y); }
static int hx(const World *w) { return FX_TO_INT(w->h.x); }
static int hy(const World *w) { return FX_TO_INT(w->h.y); }

static Hit myth_hit(const World *w, double pct, int element)
{
    Hit h = skill_hit(w, w->core_skill, 1.0);
    h.base = w->st.weapon * pct / 100.0;
    h.element = (uint8_t)element;
    h.sig = MYTH_HIT;
    h.status = ST_NONE;
    h.status_dur = 0;
    return h;
}

static void myth_area(World *w, Profile *p, int x, int y, int r, const Hit *h)
{
    int i;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && dist_px(FX_FROM_INT(x), FX_FROM_INT(y), w->mon[i].x, w->mon[i].y) <= r)
            deal_damage(w, p, i, h);
}

/* At most BURSTS_PER_TICK explosions a tick. */
static bool burst_budget(World *w)
{
    if (w->myth.burst_tick != w->tick + 1) {
        w->myth.burst_tick = w->tick + 1;
        w->myth.bursts = 0;
    }
    return ++w->myth.bursts <= BURSTS_PER_TICK;
}

/* The k-th living monster within r of (x, y), skipping 'skip'; -1 if none. */
static int pick_near(const World *w, int x, int y, int r, int k, int skip)
{
    int i;
    for (i = 0; i < w->nmon; i++) {
        const Monster *m = &w->mon[i];
        if (i != skip && m->alive && dist_px(FX_FROM_INT(x), FX_FROM_INT(y), m->x, m->y) <= r && k-- == 0)
            return i;
    }
    return -1;
}

static int count_in(const World *w, int x, int y, int r)
{
    int i, n = 0;
    for (i = 0; i < w->nmon; i++)
        n += w->mon[i].alive && dist_px(FX_FROM_INT(x), FX_FROM_INT(y), w->mon[i].x, w->mon[i].y) <= r;
    return n;
}

/* Pull a monster 'step' px toward (x, y), stopping 'stop' px short. */
static void pull(World *w, Monster *m, fx x, fx y, int step, int stop)
{
    fx dx, dy;
    if (m->boss || m->special == MS_BUTCHER || dist_px(m->x, m->y, x, y) <= stop)
        return;
    step_toward(m->x, m->y, x, y, FX_FROM_INT(step), &dx, &dy);
    move_body(w, &m->x, &m->y, dx, dy, MON_HALF);
}

/* ------------------------------------------------------------ periodic */

static void starfall(World *w, Profile *p)
{
    int k, n = count_in(w, hx(w), hy(w), 140);
    Hit h = myth_hit(w, pw(w, MY_STARFALL), EL_LIGHT);
    if (--w->myth.star_t > 0 || n == 0)
        return;
    w->myth.star_t = 36;
    for (k = 0; k < 3; k++) {
        int i = pick_near(w, hx(w), hy(w), 140, rng_range(&w->rng, 0, n - 1), -1);
        if (i < 0)
            continue;
        effect(w, FX_STAR, mx(&w->mon[i]), my(&w->mon[i]), 0, 0, 18, 10, C_STAR);
        myth_area(w, p, mx(&w->mon[i]), my(&w->mon[i]), 18, &h);
    }
    sig_shake(w, 3, 1);
    world_sound(w, SND_CAST_LIGHTNING);
}

static void void_tick(World *w, Profile *p)
{
    int i, n, c;
    if (w->myth.void_pull > 0) {
        fx vx = FX_FROM_INT(w->myth.vx), vy = FX_FROM_INT(w->myth.vy);
        for (i = 0; i < w->nmon; i++)
            if (w->mon[i].alive && dist_px(w->mon[i].x, w->mon[i].y, vx, vy) <= 110)
                pull(w, &w->mon[i], vx, vy, 2, 6);
        if (--w->myth.void_pull == 0) {
            Hit h = myth_hit(w, pw(w, MY_VOID), EL_SHADOW);
            effect(w, FX_BOOM, w->myth.vx, w->myth.vy, 0, 0, 46, 16, C_VOID);
            myth_area(w, p, w->myth.vx, w->myth.vy, 46, &h);
            sig_shake(w, 10, 3);
            world_sound(w, SND_BOSS_DIE);
        }
        return;
    }
    if (--w->myth.void_t > 0 || (c = densest_pack(w, 160, 40, &n)) < 0 || n < 2)
        return;
    w->myth.void_t = 150;
    w->myth.void_pull = 36;
    w->myth.vx = (int16_t)mx(&w->mon[c]);
    w->myth.vy = (int16_t)my(&w->mon[c]);
    effect(w, FX_VORTEX, w->myth.vx, w->myth.vy, 0, 0, 110, 36, C_VOID);
    world_sound(w, SND_CAST_SHADOW);
}

static void timestop(World *w)
{
    int i;
    if (w->myth.stop_t > 0) {
        w->myth.stop_t--;
        return;
    }
    if (--w->myth.stop_cd > 0 || count_in(w, hx(w), hy(w), 120) < 3)
        return;
    w->myth.stop_cd = 15 * TICK_HZ;
    w->myth.stop_t = 3 * TICK_HZ;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && dist_px(w->mon[i].x, w->mon[i].y, w->h.x, w->h.y) <= 220)
            w->mon[i].freeze = (int16_t)MAX(w->mon[i].freeze, 3 * TICK_HZ);
    effect(w, FX_NOVA, hx(w), hy(w), 0, 0, 120, 14, C_TIME);
    world_banner(w, "TIME STANDS STILL", C_TIME);
    world_sound(w, SND_CAST_COLD);
}

static void dragonfire(World *w, Profile *p)
{
    Hit h;
    if (--w->myth.dragon_t > 0 || count_in(w, hx(w), hy(w), 46) == 0)
        return;
    w->myth.dragon_t = TICK_HZ;
    h = myth_hit(w, pw(w, MY_DRAGON), EL_FIRE);
    h.status = ST_BURN;
    h.status_dur = 2 * TICK_HZ;
    effect(w, FX_FIRERING, hx(w), hy(w), 0, 0, 46, 14, C_DRAGON);
    myth_area(w, p, hx(w), hy(w), 46, &h);
    world_sound(w, SND_CAST_FIRE);
}

/* Four blades at 90 degree steps on a circle of 26 px around the hero. */
static void blades(World *w, Profile *p)
{
    static const int8_t ring[8][2] = { { 26, 0 }, { 18, 18 }, { 0, 26 }, { -18, 18 },
                                       { -26, 0 }, { -18, -18 }, { 0, -26 }, { 18, -18 } };
    int k, phase;
    Hit h;
    w->myth.blade_a = (uint8_t)(w->myth.blade_a + 1);
    if (--w->myth.blade_cd > 0)
        return;
    w->myth.blade_cd = 6;
    h = myth_hit(w, pw(w, MY_BLADES), EL_PHYS);
    phase = (w->myth.blade_a >> 2) & 7;
    for (k = 0; k < 4; k++) {
        const int8_t *o = ring[(phase + k * 2) & 7];
        myth_area(w, p, hx(w) + o[0], hy(w) + o[1], 11, &h);
    }
}

static void magma(World *w)
{
    Hit h;
    if (!w->h.moving || --w->myth.magma_t > 0)
        return;
    w->myth.magma_t = 10;
    h = myth_hit(w, pw(w, MY_MAGMA) / 2.0, EL_FIRE);  /* two pulses a second */
    world_spawn_ground(w, hx(w), hy(w) + 3, 14, 60, &h, false, VX_BURN);
}

static void devour(World *w)
{
    int i, r = (int)pw(w, MY_DEVOUR), n = 0;
    if (--w->myth.devour_t > 0 || count_in(w, hx(w), hy(w), r) < 2)
        return;
    w->myth.devour_t = 3 * TICK_HZ;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && dist_px(w->mon[i].x, w->mon[i].y, w->h.x, w->h.y) <= r) {
            int k;
            for (k = 0; k < 12; k++)
                pull(w, &w->mon[i], w->h.x, w->h.y, 8, 14);
            n++;
        }
    effect(w, FX_VORTEX, hx(w), hy(w), 0, 0, r, 14, C_BLOOD);
    w->h.hp = MIN(w->st.max_hp, w->h.hp + w->st.max_hp * 0.05);
    if (n)
        world_sound(w, SND_CAST_SHADOW);
}

/* Voidstride Boots: an out-of-reach target is reached in a blink. */
static void blink(World *w, Profile *p)
{
    const Monster *m;
    Hit h;
    int sx = hx(w), sy = hy(w);
    if (--w->myth.blink_cd > 0 || w->h.target_kind != TGT_MON || w->h.target_idx < 0 || w->h.dash_t > 0)
        return;
    m = &w->mon[w->h.target_idx % MAX_MON];
    if (!m->alive || dist_px(m->x, m->y, w->h.x, w->h.y) < 60 || dist_px(m->x, m->y, w->h.x, w->h.y) > 240
        || !body_line_clear(w, w->h.x, w->h.y, m->x, m->y, HERO_HALF))
        return;
    w->myth.blink_cd = 20;
    step_toward(m->x, m->y, w->h.x, w->h.y, FX(12), &w->h.x, &w->h.y);   /* lands 12 px short, on the clear line */
    w->h.x += m->x;
    w->h.y += m->y;
    w->h.px = w->h.x;
    w->h.py = w->h.y;
    h = myth_hit(w, pw(w, MY_BLINK), EL_SHADOW);
    effect(w, FX_DASH, sx, sy, hx(w), hy(w), 12, 10, C_VOID);
    effect(w, FX_NOVA, hx(w), hy(w), 0, 0, 30, 12, C_VOID);
    myth_area(w, p, hx(w), hy(w), 30, &h);
    world_sound(w, SND_CAST_SHADOW);
}

void myth_tick(World *w, Profile *p)
{
    const BuildRT *b = &w->st.b;
    if (w->myth.undying_cd > 0) w->myth.undying_cd--;
    if (w->myth.eye_cd > 0) w->myth.eye_cd--;
    if (w->myth.thunder_cd > 0) w->myth.thunder_cd--;
    if (w->myth.streak_t > 0 && --w->myth.streak_t == 0)
        w->myth.streak = 0;
    if (w->h.dead_t > 0)
        return;
    if (b->myth[MY_STARFALL] > 0) starfall(w, p);
    if (b->myth[MY_VOID] > 0)     void_tick(w, p);
    if (b->myth[MY_TIMESTOP] > 0) timestop(w);
    if (b->myth[MY_DRAGON] > 0)   dragonfire(w, p);
    if (b->myth[MY_BLADES] > 0)   blades(w, p);
    if (b->myth[MY_MAGMA] > 0)    magma(w);
    if (b->myth[MY_DEVOUR] > 0)   devour(w);
    if (b->myth[MY_BLINK] > 0)    blink(w, p);
}

/* ---------------------------------------------------------------- hits */

static void shatter(World *w, Profile *p, const Monster *m)
{
    static const int8_t dir[6][2] = { { 4, 0 }, { 2, 3 }, { -2, 3 }, { -4, 0 }, { -2, -3 }, { 2, -3 } };
    int k;
    if (!burst_budget(w))
        return;
    for (k = 0; k < 6; k++) {
        Proj *s = spawn_proj(w);
        if (!s)
            return;
        s->kind = PJ_SKILL;
        s->x = s->px = m->x;
        s->y = s->py = m->y;
        s->vx = FX_FROM_INT(dir[k][0]);
        s->vy = FX_FROM_INT(dir[k][1]);
        s->life = 16;
        s->pierce = 1;
        s->hit = myth_hit(w, pw(w, MY_SHATTER), EL_LIGHT);
        s->sig = SIGP_SHARD;
    }
    effect(w, FX_SPARK, mx(m), my(m), 0, 0, 14, 8, C_STAR);
    (void)p;
}

/* A crescent from the hero through the target: everything near the line. */
static void crescent(World *w, Profile *p, const Monster *m)
{
    int i, x0 = hx(w), y0 = hy(w), dx = mx(m) - x0, dy = my(m) - y0, len = MAX(1, dist_px(w->h.x, w->h.y, m->x, m->y));
    int x1 = x0 + dx * 170 / len, y1 = y0 + dy * 170 / len;
    Hit h = myth_hit(w, pw(w, MY_CRESCENT), EL_PHYS);
    for (i = 0; i < w->nmon; i++) {
        const Monster *t = &w->mon[i];
        int px = mx(t) - x0, py = my(t) - y0, along = (px * dx + py * dy) / len, across = (px * dy - py * dx) / len;
        if (t->alive && along >= -8 && along <= 170 && ABS(across) <= 18)
            deal_damage(w, p, i, &h);
    }
    effect(w, FX_CRESCENT, x0, y0, x1, y1, 18, 12, RGB565(255, 250, 230));
    sig_shake(w, 5, 2);
    world_sound(w, SND_CAST_PHYS);
}

static void reap(World *w, Profile *p, Monster *m)
{
    Hit h = myth_hit(w, 300, EL_SHADOW);
    effect(w, FX_REAP, mx(m), my(m), 0, 0, 16, 12, C_VOID);
    floater_kind(w, mx(m), my(m) - 18, "REAPED", C_VOID, FL_BIG);
    m->hp = 0;
    kill_rewards(w, p, m);
    if (burst_budget(w))
        myth_area(w, p, mx(m), my(m), 24, &h);
}

static void eye_beams(World *w, Profile *p, int mi)
{
    int k, n = 0;
    Hit h = myth_hit(w, pw(w, MY_EYES), EL_FIRE);
    w->myth.eye_cd = 4;
    for (k = 0; k < 3; k++) {
        int i = pick_near(w, mx(&w->mon[mi]), my(&w->mon[mi]), 120, k, mi);
        if (i < 0)
            break;
        effect(w, FX_BOLT, hx(w), hy(w) - 8, mx(&w->mon[i]), my(&w->mon[i]), 0, 8, RGB565(255, 80, 60));
        deal_damage(w, p, i, &h);
        n++;
    }
    if (n)
        world_sound(w, SND_CAST_FIRE);
}

static void thunder(World *w, Profile *p, int first)
{
    Hit h = myth_hit(w, pw(w, MY_THUNDER), EL_LIGHT);
    uint64_t done = 0;
    int k, cur = first, px = hx(w), py = hy(w) - 8;
    w->myth.thunder_cd = 3;
    for (k = 0; k < 6 && cur >= 0; k++) {
        Monster *m = &w->mon[cur];
        int i, best = -1, bd = 91;
        done |= (uint64_t)1 << cur;
        effect(w, FX_BOLT, px, py, mx(m), my(m), 0, 8, C_THUNDR);
        px = mx(m);
        py = my(m);
        deal_damage(w, p, cur, &h);
        for (i = 0; i < w->nmon; i++)
            if (w->mon[i].alive && !((done >> i) & 1u) && dist_px(m->x, m->y, w->mon[i].x, w->mon[i].y) < bd) {
                bd = dist_px(m->x, m->y, w->mon[i].x, w->mon[i].y);
                best = i;
            }
        cur = best;
    }
    world_sound(w, SND_CAST_LIGHTNING);
}

/* After a direct hit landed (never a DoT or a power's own hit). */
void myth_on_hit(World *w, Profile *p, int mi, const Hit *h, bool crit)
{
    const BuildRT *b = &w->st.b;
    Monster *m = &w->mon[mi];
    if (h->sig || h->dot)
        return;
    if (b->myth[MY_SHATTER] > 0 && crit)
        shatter(w, p, m);
    if (b->myth[MY_THUNDER] > 0 && crit && w->myth.thunder_cd == 0 && m->alive)
        thunder(w, p, mi);
    if (b->myth[MY_EYES] > 0 && w->myth.eye_cd == 0 && rng_range(&w->rng, 0, 99) < 25)
        eye_beams(w, p, mi);
    if (b->myth[MY_CRESCENT] > 0 && ++w->myth.hits % 5 == 0)
        crescent(w, p, m);
    if (b->myth[MY_REAPER] > 0 && m->alive && !m->boss && m->special != MS_BUTCHER
        && m->hp < m->max_hp * b->myth[MY_REAPER] / 100.0)
        reap(w, p, m);
}

void myth_on_kill(World *w, Profile *p, const Monster *m)
{
    const BuildRT *b = &w->st.b;
    if (b->myth[MY_SLAUGHTER] > 0) {
        w->myth.streak = (int16_t)MIN(w->myth.streak + 4, (int)b->myth[MY_SLAUGHTER]);
        w->myth.streak_t = 4 * TICK_HZ;
    }
    if (b->myth[MY_RUIN] > 0 && burst_budget(w)) {
        Hit h = myth_hit(w, b->myth[MY_RUIN], EL_FIRE);
        effect(w, FX_BOOM, mx(m), my(m), 0, 0, 30, 12, C_BLOOD);
        myth_area(w, p, mx(m), my(m), 30, &h);
    }
}

double myth_damage_mult(const World *w, const Monster *m)
{
    double x = 1.0 + w->myth.streak / 100.0;
    if (w->myth.stop_t > 0 && m->freeze > 0)
        x *= 1.0 + w->st.b.myth[MY_TIMESTOP] / 100.0;
    return x;
}

bool myth_refuse_death(World *w)
{
    if (w->st.b.myth[MY_UNDYING] <= 0 || w->myth.undying_cd > 0)
        return false;
    w->myth.undying_cd = 30 * TICK_HZ;
    w->h.hp = w->st.max_hp;
    hero_add_barrier(w, 0.5);
    hero_buff(w, BUFF_UNSTOP, 0, 2 * TICK_HZ);
    effect(w, FX_LEVEL, hx(w), hy(w), 0, 0, 30, 24, RGB565(255, 230, 140));
    world_banner(w, "DEATH REFUSED", RGB565(255, 230, 140));
    sig_shake(w, 8, 3);
    world_sound(w, SND_LEVEL);
    return true;
}

int myth_dive(World *w, const Profile *p)
{
    int k;
    if (w->st.b.myth[MY_DIVER] <= 0 || p->mode != MODE_PUSH || rng_range(&w->rng, 0, 99) >= (int)w->st.b.myth[MY_DIVER])
        return 0;
    for (k = 0; k <= 2; k++)
        if (is_boss_floor(p->floor + k))
            return 0;                               /* never past a guardian */
    return 2;
}

/* ---------------------------------------------------------------- loot */

int mythic_unique(int power)
{
    int i;
    for (i = 1; i <= unique_count; i++)
        if (unique_def(i)->mythic && unique_def(i)->kind == MOD_MYTHIC && unique_def(i)->arg == power)
            return i;
    return 0;
}

void mythic_try_drop(World *w, Profile *p, fx x, fx y, int odds)
{
    int uid;
    if ((int)(rng_next(&w->rng) % 100000u) >= odds)
        return;
    uid = mythic_unique(rng_range(&w->rng, MY_NONE + 1, MY_COUNT - 1));
    if (uid)
        world_drop_unique(w, p, x, y, uid);
}
