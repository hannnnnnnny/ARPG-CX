/*
 * save_legacy.c - reads save files from before the Diablo IV style update
 * (format versions 1-3: three classes, 7 gear slots, talents and runes)
 * and converts them into a current Profile.
 *
 * What carries over: class, level (above the new cap becomes paragon),
 * gold, floors, embers and rebirth upgrades, story progress, options and
 * statistics. Old gear is re-forged into new-style items of the same slot,
 * rarity and item level. Skill and talent points are refunded and the
 * auto planner rebuilds the closest matching build.
 */
#include "save_legacy.h"
#include "save_io.h"
#include "build.h"
#include "items.h"
#include "paragon.h"
#include "progress.h"
#include <string.h>

#define OLD_SLOTS 7
#define OLD_BAG 24
#define OLD_BRANCHES 3
#define OLD_NODES 5

typedef struct { uint8_t used, slot, rarity, ilvl_ok; uint16_t ilvl; } OldItem;

typedef struct {
    uint8_t cls;
    int level, floor, best_floor, rebirths, best_ever;
    double xp, gold, embers, kpm, kills, play;
    OldItem equip[OLD_SLOTS], bag[OLD_BAG];
    uint8_t talent[OLD_BRANCHES][OLD_NODES], focus;
    uint8_t up[UP_COUNT], auto_equip, salvage, mode, show_fps, low_power;
    uint16_t story;
    uint32_t save_time, seed;
} OldProfile;

static void read_old_item(Reader *r, OldItem *it)
{
    int i, naff;
    uint32_t ilvl;
    memset(it, 0, sizeof *it);
    it->used = r8(r);
    if (!it->used)
        return;
    it->slot = r8(r); it->rarity = r8(r); naff = r8(r);
    r8(r); r8(r); r8(r);                        /* base, name, locked */
    if (r->version >= 2) r8(r);                 /* legendary power */
    ilvl = r32(r);                              /* read once: MIN() evaluates twice */
    it->ilvl = (uint16_t)MIN(ilvl, 65535u);
    rf(r);                                      /* main stat */
    if (it->slot >= OLD_SLOTS || it->rarity > 3 || naff > 6)
        r->ok = false;
    for (i = 0; i < naff && r->ok; i++) {
        r8(r);
        if (r->version >= 2) r8(r);
        rf(r);
    }
}

static void read_old_skills(Reader *r, OldProfile *o)
{
    int i, j;
    if (r->version < 2) {
        for (i = 0; i < 7; i++) r8(r);
        r32(r);
        return;
    }
    for (i = 0; i < 6; i++) { r8(r); r8(r); }
    r32(r);
    for (i = 0; i < OLD_BRANCHES; i++)
        for (j = 0; j < OLD_NODES; j++)
            o->talent[i][j] = r8(r);
    r32(r);
}

static void read_old_body(Reader *r, OldProfile *o)
{
    int i;
    o->cls = r->version >= 2 ? r8(r) : 0;
    o->level = (int)r32(r); o->xp = rf(r); o->gold = rf(r);
    o->floor = (int)r32(r); o->best_floor = (int)r32(r);
    for (i = 0; i < OLD_SLOTS; i++) { read_old_item(r, &o->equip[i]); r8(r); }
    for (i = 0; i < OLD_BAG; i++) read_old_item(r, &o->bag[i]);
    read_old_skills(r, o);
    o->embers = rf(r);
    for (i = 0; i < UP_COUNT; i++) o->up[i] = r8(r);
    o->rebirths = (int)r32(r); o->best_ever = (int)r32(r);
    o->auto_equip = r8(r); o->salvage = r8(r); o->mode = r8(r);
    r8(r); o->show_fps = r8(r); o->low_power = r8(r); r8(r);   /* auto skills, auto forge */
    if (r->version >= 2) {
        r8(r);                                                  /* auto talents */
        o->story = r8(r);
        o->story |= (uint16_t)(r8(r) << 8);
    }
    o->focus = r->version >= 3 ? r8(r) : 255;
    o->save_time = r32(r); o->kpm = rf(r); o->kills = rf(r); o->play = rf(r);
    o->seed = r32(r);
}

/* Old talent branch -> the new build preset that plays most like it. */
static int old_preset(const OldProfile *o)
{
    static const uint8_t map[3][OLD_BRANCHES] = { { 2, 0, 1 }, { 0, 1, 2 }, { 0, 1, 2 } };
    int b, best = 0, pts[OLD_BRANCHES] = { 0 }, n;
    for (b = 0; b < OLD_BRANCHES; b++)
        for (n = 0; n < OLD_NODES; n++)
            pts[b] += o->talent[b][n];
    for (b = 1; b < OLD_BRANCHES; b++)
        if (pts[b] > pts[best])
            best = b;
    if (o->focus < OLD_BRANCHES)
        best = o->focus;
    return map[o->cls % 3][best];
}

/* Re-forge an old item: same slot, rarity and item level, new-style rolls. */
static void reforge(Item *out, const OldItem *in, int cls, Rng *r)
{
    static const uint8_t slot_map[OLD_SLOTS] = { SLOT_WEAPON, SLOT_HELM, SLOT_CHEST, SLOT_GLOVES, SLOT_BOOTS,
                                                 SLOT_RING1, SLOT_AMULET };
    int tries;
    memset(out, 0, sizeof *out);
    if (!in->used)
        return;
    for (tries = 0; tries < 60; tries++) {
        item_roll_slot(out, r, MAX(in->ilvl, 1), 0, (Rarity)in->rarity, (Slot)slot_map[in->slot % OLD_SLOTS], cls);
        if (out->rarity == in->rarity)
            break;
    }
}

static void convert(const OldProfile *o, Profile *p)
{
    Rng r;
    int i, cls = o->cls % 3, level = CLAMP(o->level, 1, 100000);
    prog_new(p, o->seed, cls);
    rng_seed(&r, o->seed ^ 0x51ABu);
    p->level = MIN(level, LEVEL_CAP);
    p->paragon_level = MAX(level - LEVEL_CAP, 0);
    p->xp = level < LEVEL_CAP ? o->xp : 0;
    p->skill_points = p->level;                 /* one per level, all refunded */
    memset(p->skill_rank, 0, sizeof p->skill_rank);
    memset(p->bar, NO_SKILL, sizeof p->bar);
    p->gold = o->gold;
    p->floor = MAX(o->floor, 1);
    p->best_floor = MAX(o->best_floor, p->floor);
    for (i = 0; i < OLD_SLOTS; i++)
        if (o->equip[i].used) {
            Item it;
            reforge(&it, &o->equip[i], cls, &r);
            p->equip[it.slot] = it;
        }
    for (i = 0; i < OLD_BAG; i++)
        reforge(&p->bag[i], &o->bag[i], cls, &r);
    p->embers = o->embers;
    memcpy(p->up, o->up, sizeof p->up);
    p->rebirths = o->rebirths;
    p->best_floor_ever = MAX(o->best_ever, p->best_floor);
    p->auto_equip = o->auto_equip;
    p->salvage_upto = o->salvage <= RAR_RARE ? o->salvage : 255;
    p->mode = o->mode <= MODE_FARM ? o->mode : MODE_PUSH;
    p->show_fps = o->show_fps;
    p->low_power = o->low_power;
    p->story_seen = o->story;
    p->save_time = o->save_time;
    p->kpm = o->kpm;
    p->total_kills = o->kills;
    p->play_seconds = o->play;
    p->preset = (uint8_t)old_preset(o);
    build_auto_spend(p);
    paragon_auto(p);
}

SaveStatus save_legacy_read(Profile *p, const uint8_t *buf, size_t len, int version)
{
    Reader r;
    OldProfile o;
    memset(&o, 0, sizeof o);
    r.p = buf; r.n = 12; r.len = len - 4; r.ok = true; r.version = version;
    read_old_body(&r, &o);
    if (!r.ok || r.n != r.len || o.cls > 2 || o.level < 1 || o.floor < 1)
        return SAVE_CORRUPT;
    convert(&o, p);
    return SAVE_OK;
}
