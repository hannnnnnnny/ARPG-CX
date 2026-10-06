#include "goals.h"
#include "balance.h"
#include "progress.h"
#include "story.h"
#include "world.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>

const AchDef ach_defs[ACH_COUNT] = {
    { "INTO THE DARK", AK_FLOOR, 10 },          { "DEEPER STILL", AK_FLOOR, 25 },
    { "TORMENTED", AK_FLOOR, 51 },              { "THE GLASS DEPTHS", AK_FLOOR, 75 },
    { "HEART OF CINDER", AK_FLOOR, 100 },       { "BEYOND ALL MAPS", AK_FLOOR, 150 },
    { "SEASONED", AK_LEVEL, 20 },               { "MASTER OF ARMS", AK_LEVEL, LEVEL_CAP },
    { "PARAGON", AK_PARAGON, 50 },              { "LEGEND OF CINDERMERE", AK_PARAGON, 200 },
    { "SLAYER", AK_KILLS, 1000 },               { "BUTCHER", AK_KILLS, 10000 },
    { "SCOURGE", AK_KILLS, 100000 },            { "EXTINCTION", AK_KILLS, 1000000 },
    { "CHAMPION HUNTER", AK_ELITES, 25 },       { "BANE OF CHAMPIONS", AK_ELITES, 250 },
    { "GOLD RUSH", AK_GOBLINS, 1 },             { "GOBLIN BANE", AK_GOBLINS, 20 },
    { "PILGRIM", AK_SHRINES, 10 },              { "DEVOTEE", AK_SHRINES, 100 },
    { "TRAP SPRINGER", AK_EVENTS, 10 },         { "UNSHAKEN", AK_EVENTS, 100 },
    { "BOUNTY HUNTER", AK_BOUNTIES, 5 },        { "TOWN HERO", AK_BOUNTIES, 25 },
    { "LIVING LEGEND", AK_BOUNTIES, 100 },      { "ARCHIVIST", AK_LORE, 4 },
    { "KEEPER OF PAGES", AK_LORE, 16 },         { "ASPECT SEEKER", AK_CODEX, 10 },
    { "MASTER OF ASPECTS", AK_CODEX, 30 },      { "ANCESTRAL BLOOD", AK_ANCESTRAL, 1 },
    { "HEIRLOOMS", AK_ANCESTRAL, 25 },          { "MYTHIC", AK_MYTHIC, 1 },
    { "THE ASH SETTLES", AK_STORY, 1 },         { "THE LAST KEEPER", AK_STORY, 2 },
    { "REBORN", AK_REBIRTH, 1 },                { "ETERNAL", AK_REBIRTH, 5 },
    { "DEVOTED", AK_HOURS, 10 },
    /* the long road: goals spread over hundreds of hours */
    { "ABYSS WALKER", AK_FLOOR, 200 },          { "THE DEEP DARK", AK_FLOOR, 250 },
    { "DOORKEEPER", AK_FLOOR, 300 },            { "BEYOND THE DOOR", AK_FLOOR, 350 },
    { "NO WAY BACK", AK_FLOOR, 400 },           { "BOTTOMLESS", AK_FLOOR, 500 },
    { "PARAGON OF ASH", AK_PARAGON, 500 },      { "ETERNAL PARAGON", AK_PARAGON, 1000 },
    { "APOCALYPSE", AK_KILLS, 3000000 },        { "THE END OF ALL THINGS", AK_KILLS, 10000000 },
    { "CHAMPION SLAYER", AK_ELITES, 1000 },     { "GOBLIN HUNTER", AK_GOBLINS, 100 },
    { "TREASURE HOARDER", AK_GOBLINS, 500 },    { "LEGEND OF THE BOARD", AK_BOUNTIES, 2000 },
    { "LOREKEEPER", AK_LORE, 28 },              { "THE WHOLE STORY", AK_LORE, LORE_COUNT },
    { "EVERY ASPECT", AK_CODEX, 60 },           { "ANCESTRAL ARSENAL", AK_ANCESTRAL, 100 },
    { "MYTHIC HOARD", AK_MYTHIC, 5 },           { "FULL CIRCLE", AK_STORY, 3 },
    { "PHOENIX", AK_REBIRTH, 10 },              { "UNDYING", AK_REBIRTH, 25 },
    { "ENDLESS CYCLE", AK_REBIRTH, 50 },        { "DEDICATED", AK_HOURS, 50 },
    { "A HUNDRED HOURS", AK_HOURS, 100 },       { "OBSESSED", AK_HOURS, 200 },
    { "THREE HUNDRED HOURS", AK_HOURS, 300 },
};

/* ------------------------------------------------------------- bounties */

static int pick_monster(const Profile *p, Rng *r)
{
    int t, tries = 0;
    do {
        t = rng_range(r, 0, MT_COUNT - 1);
    } while (mon_defs[t].min_floor > MAX(p->floor, 1) && ++tries < 50);
    return mon_defs[t].min_floor > MAX(p->floor, 1) ? MT_SKELETON : t;
}

static bool kind_taken(const Profile *p, int kind)
{
    int i;
    for (i = 0; i < BOUNTY_SLOTS; i++)
        if (p->bounty[i].kind == kind)
            return true;
    return false;
}

/* Sized so each bounty takes a few minutes of play. */
static void roll_bounty(const Profile *p, Bounty *b, Rng *r)
{
    int kind;
    do {
        kind = rng_range(r, BT_KILL_TYPE, BT_COUNT - 1);
    } while (kind_taken(p, kind));
    b->kind = (uint8_t)kind;
    b->arg = 0;
    b->have = 0;
    switch (kind) {
    case BT_KILL_TYPE: b->arg = (uint8_t)pick_monster(p, r); b->need = (uint16_t)rng_range(r, 120, 240); break;
    case BT_ELITES:    b->need = (uint16_t)rng_range(r, 15, 30); break;
    case BT_FLOORS:    b->need = (uint16_t)rng_range(r, 10, 20); break;
    case BT_GOBLINS:   b->need = (uint16_t)rng_range(r, 1, 2); break;
    case BT_SHRINES:   b->need = (uint16_t)rng_range(r, 4, 6); break;
    case BT_EVENTS:    b->need = (uint16_t)rng_range(r, 4, 6); break;
    case BT_LEGENDARY: b->need = (uint16_t)rng_range(r, 4, 8); break;
    default:           b->need = 1; break;               /* BT_BOSS */
    }
}

void goals_refill(Profile *p, Rng *r)
{
    int i;
    for (i = 0; i < BOUNTY_SLOTS; i++)
        if (p->bounty[i].kind == BT_NONE)
            roll_bounty(p, &p->bounty[i], r);
}

static void count_event(Profile *p, GoalEvent e)
{
    switch (e) {
    case GE_ELITE:     p->n_elites++; break;
    case GE_GOBLIN:    p->n_goblins++; break;
    case GE_SHRINE:    p->n_shrines++; break;
    case GE_EVENT:     p->n_events++; break;
    case GE_ANCESTRAL: p->n_ancestral++; break;
    case GE_MYTHIC:    p->n_mythic++; break;
    default:           break;
    }
}

static bool bounty_matches(const Bounty *b, GoalEvent e, int arg)
{
    switch (b->kind) {
    case BT_KILL_TYPE: return e == GE_KILL && arg == b->arg;
    case BT_ELITES:    return e == GE_ELITE;
    case BT_FLOORS:    return e == GE_FLOOR;
    case BT_GOBLINS:   return e == GE_GOBLIN;
    case BT_SHRINES:   return e == GE_SHRINE;
    case BT_EVENTS:    return e == GE_EVENT;
    case BT_LEGENDARY: return e == GE_LEGENDARY;
    case BT_BOSS:      return e == GE_BOSS;
    default:           return false;
    }
}

int goals_note(Profile *p, GoalEvent e, int arg)
{
    int i, done = 0;
    count_event(p, e);
    for (i = 0; i < BOUNTY_SLOTS; i++) {
        Bounty *b = &p->bounty[i];
        if (b->have >= b->need || !bounty_matches(b, e, arg))
            continue;
        if (++b->have >= b->need)
            done |= 1 << i;
    }
    return done;
}

double goals_complete(Profile *p, int slot, Rng *r)
{
    int f = MAX(p->best_floor, 1);
    double gold = kill_gold(f) * 400.0;
    if (slot < 0 || slot >= BOUNTY_SLOTS || p->bounty[slot].kind == BT_NONE)
        return 0;
    prog_add_gold(p, gold);
    p->iron += 5 + f / 3;
    p->souls += 2 + f / 15;
    p->n_bounties++;
    p->bounty[slot].kind = BT_NONE;
    roll_bounty(p, &p->bounty[slot], r);
    return gold;
}

void bounty_text(char *out, size_t cap, const Bounty *b)
{
    static const char *const text[BT_COUNT] = {
        "", "HUNT: %s", "SLAY CHAMPIONS", "CLEAR FLOORS", "SLAY A TREASURE GOBLIN", "KNEEL AT SHRINES",
        "SURVIVE FLOOR EVENTS", "FIND LEGENDARY ITEMS", "DEFEAT A GUARDIAN",
    };
    if (b->kind == BT_KILL_TYPE)
        snprintf(out, cap, T(text[BT_KILL_TYPE]), T(mon_defs[b->arg % MT_COUNT].name));
    else
        snprintf(out, cap, "%s", T(text[b->kind % BT_COUNT]));
}

bool bounty_sane(const Bounty *b)
{
    if (b->kind == BT_NONE)
        return true;
    return b->kind < BT_COUNT && b->need > 0 && b->have <= b->need && (b->kind != BT_KILL_TYPE || b->arg < MT_COUNT);
}

/* --------------------------------------------------------- achievements */

static uint32_t codex_size(const Profile *p)
{
    uint32_t i, n = 0;
    for (i = 0; i < ASPECT_MAX; i++)
        n += p->codex[i] != 0;
    return n;
}

uint32_t ach_progress(const Profile *p, int id)
{
    switch (ach_defs[id].kind) {
    case AK_FLOOR:     return (uint32_t)MAX(p->best_floor_ever, p->best_floor);
    case AK_LEVEL:     return (uint32_t)p->level;
    case AK_PARAGON:   return (uint32_t)p->paragon_level;
    case AK_KILLS:     return p->total_kills > 4e9 ? 4000000000u : (uint32_t)p->total_kills;
    case AK_ELITES:    return p->n_elites;
    case AK_GOBLINS:   return p->n_goblins;
    case AK_SHRINES:   return p->n_shrines;
    case AK_EVENTS:    return p->n_events;
    case AK_BOUNTIES:  return p->n_bounties;
    case AK_LORE:      return (uint32_t)story_lore_found(p);
    case AK_CODEX:     return codex_size(p);
    case AK_ANCESTRAL: return p->n_ancestral;
    case AK_MYTHIC:    return p->n_mythic;
    case AK_STORY:     return (uint32_t)story_event_seen(p, STORY_EPILOGUE) + story_event_seen(p, STORY_FINALE)
                              + story_event_seen(p, STORY_VICTORY + ACT_COUNT - 1);
    case AK_REBIRTH:   return (uint32_t)p->rebirths;
    default:           return (uint32_t)(p->play_seconds / 3600.0);   /* AK_HOURS */
    }
}

int goals_check_achievements(Profile *p)
{
    int i, first = -1;
    for (i = 0; i < ACH_COUNT; i++)
        if (!ach_earned(p, i) && ach_progress(p, i) >= ach_defs[i].need) {
            p->ach |= (uint64_t)1 << i;
            if (first < 0)
                first = i;
        }
    return first;
}

int ach_count(const Profile *p)
{
    int i, n = 0;
    for (i = 0; i < ACH_COUNT; i++)
        n += ach_earned(p, i);
    return n;
}

void ach_text(char *out, size_t cap, int id)
{
    static const char *const many[AK_COUNT] = {
        "REACH FLOOR %s", "REACH LEVEL %s", "REACH PARAGON %s", "SLAY %s MONSTERS", "SLAY %s CHAMPIONS",
        "SLAY %s TREASURE GOBLINS", "USE %s SHRINES", "SURVIVE %s FLOOR EVENTS", "COMPLETE %s BOUNTIES",
        "FIND %s LOST PAGES", "LEARN %s ASPECTS", "FIND %s ANCESTRAL ITEMS", "FIND A MYTHIC UNIQUE", "",
        "REBIRTH %s TIMES", "PLAY FOR %s HOURS",
    };
    static const char *const one[AK_COUNT] = {
        [AK_GOBLINS] = "SLAY A TREASURE GOBLIN", [AK_ANCESTRAL] = "FIND AN ANCESTRAL ITEM",
        [AK_MYTHIC] = "FIND A MYTHIC UNIQUE", [AK_STORY] = "FINISH ACT V", [AK_REBIRTH] = "REBIRTH ONCE",
    };
    const AchDef *d = &ach_defs[id];
    char n[16];
    if (d->kind == AK_STORY && d->need >= 2) {
        snprintf(out, cap, "%s", T(d->need == 2 ? "FINISH ACT X" : "FINISH ACT XV"));
        return;
    }
    if (d->need == 1 && one[d->kind]) {
        snprintf(out, cap, "%s", T(one[d->kind]));
        return;
    }
    fmt_num(n, sizeof n, (double)d->need);
    snprintf(out, cap, T(many[d->kind]), n);
}
