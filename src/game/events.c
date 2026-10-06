#include "events.h"
#include "world_int.h"
#include "balance.h"
#include "progress.h"
#include "story.h"
#include "../core/bignum.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

#define EVENT_CHANCE  55               /* % of ordinary floors with an event */
#define GOBLIN_TICKS  (12 * TICK_HZ)
#define GOBLIN_SPEED  FX(1.15)          /* the hero (1.6) catches up, slowly */
#define REACHABLE     30000
#define C_GOLDEN      RGB565(255, 210, 80)
#define C_EVENT       RGB565(255, 150, 90)

static const char *const shrine_names[SH_COUNT] = {
    "BLESSED SHRINE", "LETHAL SHRINE", "GREED SHRINE", "ENLIGHTENED SHRINE", "FRENZY SHRINE", "PROTECTION SHRINE",
};
static const char *const shrine_desc[SH_COUNT] = {
    "+50% DAMAGE FOR 40 SECONDS", "EVERY HIT IS CRITICAL FOR 40 SECONDS", "TRIPLE GOLD FOR 40 SECONDS",
    "TRIPLE EXPERIENCE FOR 40 SECONDS", "+50% ATTACK SPEED FOR 40 SECONDS", "HALF DAMAGE TAKEN FOR 40 SECONDS",
};
static const char *const champ_names[CH_KINDS] = {
    "FAST", "STURDY", "VAMPIRIC", "VOLATILE", "WARDED", "FRENZIED",
};

static const char *const shrine_tags[SH_COUNT] = {
    "BLESSED", "LETHAL", "GREED", "ENLIGHTENED", "FRENZY", "PROTECTION",
};

const char *shrine_name(int kind) { return shrine_names[kind % SH_COUNT]; }
const char *shrine_tag(int kind) { return shrine_tags[kind % SH_COUNT]; }

/* ------------------------------------------------------------ placement */

static bool find_cell(World *w, int min_path, int *cx, int *cy)
{
    int tries;
    for (tries = 0; tries < 300; tries++) {
        int x = rng_range(&w->rng, 1, MAP_W - 2), y = rng_range(&w->rng, 1, MAP_H - 2);
        int d = w->fh[y][x];
        if (w->cell[y][x] == CELL_FLOOR && d >= min_path && d < REACHABLE
            && (ABS(x - w->stairs_x) > 1 || ABS(y - w->stairs_y) > 1)) {
            *cx = x;
            *cy = y;
            return true;
        }
    }
    return false;
}

static int wave_type(World *w)
{
    int t, tries = 0;
    do {
        t = rng_range(&w->rng, 0, MT_COUNT - 1);
    } while (mon_defs[t].min_floor > w->floor && ++tries < 50);
    return mon_defs[t].min_floor > w->floor ? MT_SKELETON : t;
}

/* A pack that knows where the hero is, around cell (cx, cy). */
static void spawn_wave(World *w, int cx, int cy, int n, int champs)
{
    int k, tries = 0, type = wave_type(w);
    for (k = 0; k < n && tries < 200; tries++) {
        int x = cx + rng_range(&w->rng, -5, 5), y = cy + rng_range(&w->rng, -4, 4), before = w->nmon;
        if (!world_walkable(w, x, y) || (ABS(x - cx) < 2 && ABS(y - cy) < 2) || w->fh[y][x] > 14)
            continue;                             /* only spots a short walk from the hero */
        spawn_monster(w, type, x, y, k < champs, false);
        if (w->nmon == before)
            return;                               /* monster pool is full */
        w->mon[before].wave = 1;
        w->mon[before].aggro = 1;
        w->mon[before].px = w->mon[before].x;
        w->mon[before].py = w->mon[before].y;
        w->ev.wave_left++;
        k++;
    }
}

static void spawn_goblin(World *w)
{
    int cx, cy, before = w->nmon;
    Monster *m;
    if (!find_cell(w, 10, &cx, &cy))
        return;
    spawn_monster(w, MT_IMP, cx, cy, false, false);
    if (w->nmon == before)
        return;
    m = &w->mon[before];
    m->goblin = 1;
    m->max_hp = m->hp = m->max_hp * 5.0;
    m->dmg = 0;
    w->ev.kind = EV_GOBLIN;
}

void events_init(World *w, const Profile *p)
{
    int roll, kind;
    memset(&w->ev, 0, sizeof w->ev);
    if (w->boss_floor || w->floor < 2 || rng_range(&w->rng, 0, 99) >= EVENT_CHANCE)
        return;
    roll = rng_range(&w->rng, 0, 99);
    kind = roll < 20 ? EV_GOBLIN : roll < 50 ? EV_SHRINE : roll < 70 ? EV_AMBUSH : roll < 85 ? EV_CHEST : EV_FALLEN;
    if (kind == EV_FALLEN && story_unread_lore(p, 0) < 0)
        kind = EV_SHRINE;
    if (kind == EV_GOBLIN) {
        spawn_goblin(w);
        return;
    }
    if (kind != EV_AMBUSH && !find_cell(w, 8, &w->ev.cx, &w->ev.cy))
        return;
    w->ev.kind = (uint8_t)kind;
    w->ev.shrine = (uint8_t)rng_range(&w->rng, 0, SH_COUNT - 1);
}

/* ------------------------------------------------------------- rewards */

static void bonus_gold(World *w, Profile *p, double kills, int x, int y)
{
    char n[16], buf[32];
    double g = kill_gold(w->floor) * kills * (1.0 + w->st.gold_pct / 100.0);
    prog_add_gold(p, g);
    fmt_num(n, sizeof n, g);
    snprintf(buf, sizeof buf, "+%s G", n);
    floater_kind(w, x, y, buf, C_GOLDEN, FL_BIG);
}

static void goblin_loot(World *w, Profile *p, const Monster *m)
{
    int kind = rng_range(&w->rng, 0, GEM_KINDS - 1);
    bonus_gold(w, p, 60, FX_TO_INT(m->x), FX_TO_INT(m->y) - 20);
    world_drop_item(w, p, m->x, m->y, RAR_RARE, 6);
    world_drop_item(w, p, m->x, m->y, RAR_RARE, 6);
    p->gems[kind][CLAMP(w->floor / 12, 0, GEM_TIERS - 1)] += 2;
    world_banner(w, "TREASURE GOBLIN SLAIN!", C_GOLDEN);
    bark(&w->bark, BK_GOBLIN_KILL, (uint32_t)w->tick);
    w->ev.state = ES_DONE;
    world_goal(w, p, GE_GOBLIN, 0);
}

static void wave_cleared(World *w, Profile *p)
{
    fx x = w->ev.kind == EV_CHEST ? cell_center(w->ev.cx) : w->h.x;
    fx y = w->ev.kind == EV_CHEST ? cell_center(w->ev.cy) : w->h.y;
    w->ev.state = ES_DONE;
    if (w->ev.kind == EV_CHEST) {
        world_drop_item(w, p, x, y, RAR_LEGEND, 4);
        world_drop_item(w, p, x, y, RAR_RARE, 4);
        bonus_gold(w, p, 30, FX_TO_INT(x), FX_TO_INT(y) - 16);
        world_banner(w, "THE CURSED CHEST OPENS", C_GOLDEN);
    } else {
        world_drop_item(w, p, x, y, RAR_RARE, 4);
        bonus_gold(w, p, 20, FX_TO_INT(x), FX_TO_INT(y) - 16);
        world_message(w, "AMBUSH REPELLED!", C_EVENT);
    }
    world_goal(w, p, GE_EVENT, 0);
}

static void volatile_burst(World *w, Profile *p, const Monster *m)
{
    effect(w, FX_NOVA, FX_TO_INT(m->x), FX_TO_INT(m->y), 0, 0, 30, 14, RGB565(255, 120, 40));
    if (dist_px(m->x, m->y, w->h.x, w->h.y) <= 30)
        hurt_hero(w, p, m->dmg * 1.5, EL_FIRE, -1);
}

void events_on_kill(World *w, Profile *p, Monster *m)
{
    if (m->goblin)
        goblin_loot(w, p, m);
    if (m->champ & CH_VOLATILE)
        volatile_burst(w, p, m);
    if (m->wave && w->ev.wave_left > 0 && --w->ev.wave_left == 0)
        wave_cleared(w, p);
}

/* ------------------------------------------------------------- per tick */

static Monster *find_goblin(World *w)
{
    int i;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && w->mon[i].goblin)
            return &w->mon[i];
    return NULL;
}

static void goblin_tick(World *w)
{
    Monster *m = find_goblin(w);
    if (!m || w->ev.state == ES_DONE)
        return;
    if (w->ev.state == ES_WAITING && m->aggro) {
        w->ev.state = ES_RUNNING;
        w->ev.t = GOBLIN_TICKS;
        world_message(w, "A TREASURE GOBLIN!", C_GOLDEN);
        bark(&w->bark, BK_GOBLIN, (uint32_t)w->tick);
    } else if (w->ev.state == ES_RUNNING && --w->ev.t <= 0) {
        m->alive = 0;                            /* through its portal */
        effect(w, FX_NOVA, FX_TO_INT(m->x), FX_TO_INT(m->y), 0, 0, 18, 16, RGB565(200, 120, 255));
        if (w->kills < w->quota)
            w->quota--;
        w->ev.state = ES_DONE;
        world_message(w, "THE GOBLIN ESCAPED", RGB565(200, 120, 255));
        bark(&w->bark, BK_GOBLIN_GONE, (uint32_t)w->tick);
    }
}

void events_goblin_act(World *w, Monster *m)
{
    fx dx, dy, ox = m->x, oy = m->y;
    if (w->ev.state != ES_RUNNING)
        return;
    step_toward(m->x, m->y, m->x + (m->x - w->h.x), m->y + (m->y - w->h.y), GOBLIN_SPEED, &dx, &dy);
    move_body(w, &m->x, &m->y, dx, dy, MON_HALF);
    if (m->x == ox && m->y == oy)                /* cornered: slide along the wall */
        move_body(w, &m->x, &m->y, dy, -dx, MON_HALF);
    if (dx)
        m->face = dx > 0 ? 1 : -1;
}

static void check_achievements(World *w, Profile *p)
{
    int id = goals_check_achievements(p);
    char buf[96];
    if (id < 0)
        return;
    world_banner(w, ach_defs[id].name, C_GOLDEN);
    snprintf(buf, sizeof buf, T("ACHIEVEMENT EARNED - RENOWN %d"), renown_tier(p));
    world_message(w, buf, C_GOLDEN);
    bark(&w->bark, BK_ACHIEVE, (uint32_t)w->tick);
    world_refresh_stats(w, p);                     /* renown adds damage and life */
}

void events_tick(World *w, Profile *p)
{
    if (w->shrine_t > 0)
        w->shrine_t--;
    if (w->tick % (2 * TICK_HZ) == 0)
        check_achievements(w, p);
    if (w->ev.kind == EV_GOBLIN)
        goblin_tick(w);
    if (w->ev.kind == EV_AMBUSH && w->ev.state == ES_WAITING && w->quota > 1 && w->kills >= w->quota / 2) {
        w->ev.state = ES_RUNNING;
        spawn_wave(w, px_to_cell(w->h.x), px_to_cell(w->h.y), 5, 1);
        world_banner(w, "AMBUSH!", RGB565(255, 90, 70));
        bark(&w->bark, BK_AMBUSH, (uint32_t)w->tick);
        if (w->ev.wave_left == 0)
            w->ev.state = ES_DONE;
    }
}

/* -------------------------------------------------------------- objects */

bool events_object_pending(const World *w)
{
    return (w->ev.kind == EV_SHRINE || w->ev.kind == EV_CHEST || w->ev.kind == EV_FALLEN)
        && w->ev.state == ES_WAITING && w->fh[w->ev.cy][w->ev.cx] < REACHABLE;
}

bool events_wave_active(const World *w)
{
    return w->ev.wave_left > 0;
}

static void touch_fallen(World *w, Profile *p)
{
    int page = story_unread_lore(p, (uint32_t)w->tick * 2654435761u);
    w->ev.state = ES_DONE;
    world_drop_item(w, p, cell_center(w->ev.cx), cell_center(w->ev.cy), RAR_MAGIC, 2);
    if (page < 0)
        return;
    w->ev_lore = page + 1;
    world_message(w, "FOUND A LOST PAGE", RGB565(220, 190, 130));
    bark(&w->bark, BK_LORE, (uint32_t)w->tick);
}

void events_touch(World *w, Profile *p)
{
    w->h.idle_ticks = 0;
    if (w->ev.kind == EV_SHRINE) {
        w->shrine = w->ev.shrine;
        w->shrine_t = SHRINE_TICKS;
        w->ev.state = ES_DONE;
        world_banner(w, shrine_names[w->shrine], RGB565(140, 220, 255));
        world_message(w, shrine_desc[w->shrine], RGB565(140, 220, 255));
        effect(w, FX_LEVEL, FX_TO_INT(w->h.x), FX_TO_INT(w->h.y), 0, 0, 20, 24, RGB565(140, 220, 255));
        bark(&w->bark, BK_SHRINE, (uint32_t)w->tick);
        world_goal(w, p, GE_SHRINE, 0);
    } else if (w->ev.kind == EV_CHEST) {
        w->ev.state = ES_RUNNING;
        spawn_wave(w, w->ev.cx, w->ev.cy, 6, 2);
        world_message(w, "THE CHEST'S GUARDIANS AWAKE!", C_EVENT);
        bark(&w->bark, BK_CHEST, (uint32_t)w->tick);
        if (w->ev.wave_left == 0)
            wave_cleared(w, p);
    } else if (w->ev.kind == EV_FALLEN) {
        touch_fallen(w, p);
    }
}

/* ------------------------------------------------------------ champions */

/* Affixes per champion: 1, then 2 from floor 30, 3 from Torment III and 4
 * from Torment V. */
int champion_affixes(int floor)
{
    return floor >= 251 ? 4 : floor >= 151 ? 3 : floor >= 30 ? 2 : 1;
}

void champion_roll(World *w, Monster *m)
{
    int n = champion_affixes(w->floor);
    while (n > 0) {
        int bit = 1 << rng_range(&w->rng, 0, CH_KINDS - 1);
        if (m->champ & bit)
            continue;
        m->champ |= (uint8_t)bit;
        n--;
    }
    if (m->champ & CH_STURDY)
        m->max_hp = m->hp = m->max_hp * 1.8;
}

void champion_name(char *out, size_t cap, const Monster *m)
{
    char affixes[64] = "";
    int k;
    for (k = 0; k < CH_KINDS; k++)
        if (m->champ & (1 << k)) {
            char tmp[64];
            if (affixes[0])
                tjoin(tmp, sizeof tmp, affixes, champ_names[k]);
            else
                snprintf(tmp, sizeof tmp, "%s", T(champ_names[k]));
            snprintf(affixes, sizeof affixes, "%s", tmp);
        }
    tjoin(out, cap, affixes, mon_defs[m->type % MT_COUNT].name);
}

/* --------------------------------------------------------------- goals */

void world_goal(World *w, Profile *p, GoalEvent e, int arg)
{
    int done = goals_note(p, e, arg), i;
    for (i = 0; i < BOUNTY_SLOTS; i++) {
        char t[64], buf[112];
        if (!(done & (1 << i)))
            continue;
        bounty_text(t, sizeof t, &p->bounty[i]);
        goals_complete(p, i, &w->rng);
        snprintf(buf, sizeof buf, T("BOUNTY COMPLETE: %s"), t);
        world_message(w, buf, C_GOLDEN);
        world_drop_item(w, p, w->h.x, w->h.y, RAR_RARE, 4);
        bark(&w->bark, BK_BOUNTY, (uint32_t)w->tick);
    }
}
