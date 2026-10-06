#include "world_int.h"
#include "balance.h"
#include "progress.h"
#include "skills.h"
#include "build.h"
#include "events.h"
#include "../gfx/gfx.h"
#include <string.h>
#include <stdio.h>

const MonDef mon_defs[MT_COUNT] = {
    /*               name        hp   dmg  speed      rng element    min atk */
    [MT_SKELETON] = { "SKELETON", 1.0, 1.0, FX(0.70), 0, EL_PHYS,   1, 30 },
    [MT_BAT]      = { "BAT",      0.5, 0.6, FX(1.30), 0, EL_PHYS,   1, 24 },
    [MT_GHOUL]    = { "GHOUL",    1.6, 0.9, FX(0.50), 0, EL_POISON, 4, 36 },
    [MT_SPIDER]   = { "SPIDER",   0.8, 1.1, FX(1.05), 0, EL_POISON, 6, 26 },
    [MT_IMP]      = { "IMP",      0.7, 0.8, FX(0.80), 1, EL_FIRE,   8, 45 },
    [MT_CULTIST]  = { "CULTIST",  1.0, 1.2, FX(0.70), 1, EL_SHADOW, 12, 50 },
    [MT_GOLEM]    = { "GOLEM",    3.0, 1.6, FX(0.40), 0, EL_PHYS,   15, 48 },
};

void spawn_monster(World *w, int type, int cx, int cy, bool elite, bool boss)
{
    Monster *m;
    double s = monster_scale(w->floor);
    if (w->nmon >= MAX_MON || !world_walkable(w, cx, cy))
        return;
    m = &w->mon[w->nmon++];
    memset(m, 0, sizeof *m);
    m->alive = 1;
    m->type = (uint8_t)type;
    m->elite = elite;
    m->boss = boss;
    m->face = -1;
    m->x = cell_center(cx);
    m->y = cell_center(cy);
    /* Bosses ignore their base type's toughness so every act boss is a
     * similar wall (a golem boss would otherwise have 75x normal life). */
    m->max_hp = 22.0 * s * (boss ? 25.0 : mon_defs[type].hp * (elite ? 3.0 : 1.0));
    m->hp = m->max_hp;
    m->dmg = 6.0 * s * (boss ? 2.2 : mon_defs[type].dmg * (elite ? 1.6 : 1.0));
    if (elite)
        champion_roll(w, m);
    m->anim = (int16_t)rng_range(&w->rng, 0, 63);
    m->atk_cd = (int16_t)rng_range(&w->rng, 0, mon_defs[type].atk_ticks);
    m->slam_cd = 90;
}

/* Signature colour of the current build (staff orb, buff glow). */
static uint16_t build_accent(const Profile *p)
{
    const ClassDef *c = &class_defs[p->cls % CLASS_COUNT];
    int el = c->preset[p->preset % PRESETS].element;
    return el == EL_PHYS ? c->res_color : element_color((Element)el);
}

/* How close the hero walks before attacking: the reach of its basic and
 * core skills (melee builds close in, casters keep their distance). */
static int engage_range(const World *w, const Profile *p)
{
    int i, best = 0;
    for (i = 0; i < CLASS_SKILLS; i++) {
        const SkillRT *s = &w->st.b.skill[i];
        int r;
        if (!s->usable || s->cat > CAT_CORE)
            continue;
        switch (s->behavior) {
        case SB_PROJ:
        case SB_CHAIN: r = MAX(s->range * 4 / 5, 22); break;
        case SB_DASH:  r = s->range; break;
        case SB_STRIKE:
        case SB_GROUND: r = s->range > 0 ? s->range * 4 / 5 : s->radius - 4; break;
        default:       r = MAX(s->radius - 4, 16); break;
        }
        best = best ? MIN(best, r) : r;
    }
    return best ? best : class_defs[p->cls % CLASS_COUNT].attack_range;
}

void world_refresh_stats(World *w, const Profile *p)
{
    stats_compute(&w->st, p);
    w->accent = build_accent(p);
    w->h.engage = engage_range(w, p);
    w->dmg_numbers = p->dmg_numbers;
    if (w->h.hp > w->st.max_hp)
        w->h.hp = w->st.max_hp;
    if (w->h.res > world_max_res(w))
        w->h.res = world_max_res(w);
}

double world_max_res(const World *w)
{
    return w->st.max_res;
}

void world_init_floor(World *w, const Profile *p, int floor)
{
    uint32_t seed = w->rng.s ? w->rng.s : p->seed;
    int minute_ticks = w->minute_ticks, minute_kills = w->minute_kills, msg_t = w->msg_t;
    char msg[sizeof w->msg];
    uint16_t msg_color = w->msg_color;
    BarkState bark = w->bark;
    uint64_t snd = w->snd;            /* e.g. the stairs sound of the floor just left */
    uint8_t shrine = w->shrine;
    int shrine_t = w->shrine_t;
    memcpy(msg, w->msg, sizeof msg);
    memset(w, 0, sizeof *w);
    rng_seed(&w->rng, seed ^ (uint32_t)floor * 2654435761u);
    w->floor = MAX(floor, 1);
    w->cls = p->cls;
    w->theme = ((w->floor - 1) / 10) % 5;
    w->boss_floor = is_boss_floor(w->floor);
    w->ft_target_cell = -1;
    dungeon_build(w, p);
    world_refresh_stats(w, p);
    w->h.hp = w->st.max_hp;
    w->h.res = class_defs[p->cls % CLASS_COUNT].res_kind == RES_MANA || class_defs[p->cls % CLASS_COUNT].res_kind
               == RES_ENERGY ? w->st.max_res : w->st.max_res * 0.3;
    w->h.potions = w->st.potion_max;
    w->h.face = 1;
    bfs_field(w, w->fh, px_to_cell(w->h.x), px_to_cell(w->h.y));
    world_snapshot_positions(w);
    /* Keep rate tracking and the last message across floors. */
    w->minute_ticks = minute_ticks;
    w->minute_kills = minute_kills;
    memcpy(w->msg, msg, sizeof msg);
    w->msg_color = msg_color;
    w->msg_t = msg_t;
    w->bark = bark;
    w->snd = snd;
    w->shrine = shrine;
    w->shrine_t = shrine_t;
    w->ctl.target = w->ctl.drop = -1;
    w->ctl.cast = w->cmd.cast = -1;
    events_init(w, p);
}

void world_message(World *w, const char *text, uint16_t color)
{
    snprintf(w->msg, sizeof w->msg, "%s", text);
    w->msg_color = color;
    w->msg_t = 4 * TICK_HZ;
}

void world_banner(World *w, const char *text, uint16_t color)
{
    snprintf(w->banner, sizeof w->banner, "%s", text);
    w->banner_color = color;
    w->banner_t = 3 * TICK_HZ;
}

void floater_kind(World *w, int x, int y, const char *text, uint16_t color, FloatKind k)
{
    int i, oldest = 0;
    for (i = 0; i < MAX_FLOAT; i++) {
        if (!w->fl[i].alive) {
            oldest = i;
            break;
        }
        if (w->fl[i].t > w->fl[oldest].t)
            oldest = i;
    }
    w->fl[oldest].alive = 1;
    w->fl[oldest].kind = (uint8_t)k;
    w->fl[oldest].x = (int16_t)x;
    w->fl[oldest].y = (int16_t)y;
    w->fl[oldest].t = 0;
    w->fl[oldest].color = color;
    snprintf(w->fl[oldest].text, sizeof w->fl[oldest].text, "%s", text);
}

void floater(World *w, int x, int y, const char *text, uint16_t color)
{
    floater_kind(w, x, y, text, color, FL_TEXT);
}

Effect *effect(World *w, FxKind k, int x, int y, int x2, int y2, int r, int dur, uint16_t color)
{
    int i;
    for (i = 0; i < MAX_FX; i++)
        if (!w->fx[i].alive) {
            Effect *e = &w->fx[i];
            e->alive = 1;
            e->kind = (uint8_t)k;
            e->x = (int16_t)x; e->y = (int16_t)y;
            e->x2 = (int16_t)x2; e->y2 = (int16_t)y2;
            e->r = (int16_t)r;
            e->t = 0;
            e->dur = (int16_t)dur;
            e->color = color;
            e->vfx = VX_NONE;
            return e;
        }
    return NULL;
}

int world_alive_monsters(const World *w)
{
    int i, n = 0;
    for (i = 0; i < w->nmon; i++)
        n += w->mon[i].alive;
    return n;
}

static void age_visuals(World *w)
{
    int i;
    for (i = 0; i < MAX_FLOAT; i++)
        if (w->fl[i].alive && ++w->fl[i].t > (w->fl[i].kind == FL_BIG ? 36 : 26))
            w->fl[i].alive = 0;
    for (i = 0; i < MAX_FX; i++)
        if (w->fx[i].alive && ++w->fx[i].t > w->fx[i].dur)
            w->fx[i].alive = 0;
    for (i = 0; i < MAX_DROP; i++)
        if (w->dr[i].alive)
            w->dr[i].t++;
    if (w->msg_t > 0)
        w->msg_t--;
    if (w->banner_t > 0)
        w->banner_t--;
}

/* Reveal the map around the hero (fog of war). */
static void reveal(World *w)
{
    int cx = px_to_cell(w->h.x), cy = px_to_cell(w->h.y), x, y;
    for (y = cy - 6; y <= cy + 6; y++)
        for (x = cx - 8; x <= cx + 8; x++)
            if (x >= 0 && y >= 0 && x < MAP_W && y < MAP_H)
                w->seen[y][x] = 1;
}

static void track_rate(World *w, Profile *p)
{
    p->play_seconds += 1.0 / TICK_HZ;
    if (++w->minute_ticks >= 60 * TICK_HZ) {
        /* Smoothed kills-per-minute drives offline progress. */
        p->kpm = p->kpm * 0.7 + w->minute_kills * 0.3;
        w->minute_ticks = 0;
        w->minute_kills = 0;
    }
    if (w->tick % TICK_HZ == 0)
        prog_tick_second(p);
}

void world_snapshot_positions(World *w)
{
    int i;
    w->h.px = w->h.x;
    w->h.py = w->h.y;
    for (i = 0; i < w->nmon; i++) {
        w->mon[i].px = w->mon[i].x;
        w->mon[i].py = w->mon[i].y;
    }
    for (i = 0; i < MAX_PROJ; i++) {
        w->pj[i].px = w->pj[i].x;
        w->pj[i].py = w->pj[i].y;
    }
    for (i = 0; i < MAX_ALLY; i++) {
        w->al[i].px = w->al[i].x;
        w->al[i].py = w->al[i].y;
    }
}

void world_tick(World *w, Profile *p)
{
    world_snapshot_positions(w);
    w->tick++;
    w->ev_died = w->ev_floor_done = w->ev_stuck = false;
    w->snd = 0;
    if (w->tick % 10 == 1)
        bfs_field(w, w->fh, px_to_cell(w->h.x), px_to_cell(w->h.y));
    hero_update(w, p);
    allies_update(w, p);
    monsters_update(w, p);
    projectiles_update(w, p);
    grounds_update(w, p);
    orbs_update(w);
    events_tick(w, p);
    bark_tick(&w->bark, (uint32_t)w->tick);
    age_visuals(w);
    if (w->tick % 4 == 0)
        reveal(w);
    track_rate(w, p);
}
