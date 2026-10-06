#include "save.h"
#include "save_io.h"
#include "save_legacy.h"
#include "build.h"
#include "skills.h"
#include "goals.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

/* v1-v3: the original three-class game (converted by save_legacy.c).
 * v4: six classes, Diablo IV style items, skill tree, paragon, codex,
 * crafting materials, gems, elixirs and the hero's appearance.
 * v5: display language.
 * v6: acts VI-X (32-bit story bits), bounties, achievements, lost pages
 * and the event counters.
 * v7: glyph progress, 64 lost pages (chapters XI-XV, Torment tiers). */
#define SAVE_VERSION 7

uint32_t save_crc32(const uint8_t *data, size_t n)
{
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;
    int b;
    for (i = 0; i < n; i++) {
        crc ^= data[i];
        for (b = 0; b < 8; b++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

/* ---------------------------------------------------------------- items */

static void write_item(Writer *w, const Item *it)
{
    int i;
    w8(w, it->used);
    if (!it->used)
        return;
    w8(w, it->slot); w8(w, it->rarity); w8(w, it->naff); w8(w, it->base); w8(w, it->name);
    w8(w, it->locked); w8(w, it->ancestral); w8(w, it->power); w16(w, it->power_roll);
    w8(w, it->mw); w8(w, it->temper_left); w8(w, it->enchant); w8(w, it->sockets);
    w8(w, it->gem[0]); w8(w, it->gem[1]); w16(w, it->ilvl); wf(w, it->main);
    for (i = 0; i < it->naff; i++) {
        const Affix *a = &it->aff[i];
        w8(w, a->type); w8(w, a->flags); w8(w, a->arg); w8(w, a->mwcrit); wf(w, a->value);
    }
}

static bool item_sane(const Item *it)
{
    int i;
    if (it->slot >= SLOT_COUNT || it->rarity >= RAR_COUNT || it->naff > MAX_AFFIX || it->mw > MW_MAX
        || it->sockets > 2 || it->power_roll > 1000 || it->temper_left > TEMPER_CHARGES)
        return false;
    for (i = 0; i < it->naff; i++)
        if (it->aff[i].type >= AF_COUNT || it->aff[i].arg >= CLASS_SKILLS || it->aff[i].mwcrit > 3)
            return false;
    for (i = 0; i < it->sockets; i++)
        if (it->gem[i] != GEM_NONE && it->gem[i] >= GEM_KINDS * GEM_TIERS)
            return false;
    return true;
}

static void read_item(Reader *r, Item *it)
{
    int i;
    memset(it, 0, sizeof *it);
    it->used = r8(r);
    if (!it->used)
        return;
    it->slot = r8(r); it->rarity = r8(r); it->naff = r8(r); it->base = r8(r); it->name = r8(r);
    it->locked = r8(r); it->ancestral = r8(r); it->power = r8(r); it->power_roll = r16(r);
    it->mw = r8(r); it->temper_left = r8(r); it->enchant = r8(r); it->sockets = r8(r);
    it->gem[0] = r8(r); it->gem[1] = r8(r); it->ilvl = r16(r); it->main = rf(r);
    if (it->naff > MAX_AFFIX) {
        r->ok = false;
        return;
    }
    for (i = 0; i < it->naff && r->ok; i++) {
        Affix *a = &it->aff[i];
        a->type = r8(r); a->flags = r8(r); a->arg = r8(r); a->mwcrit = r8(r); a->value = rf(r);
    }
    if (!item_sane(it))
        r->ok = false;
}

/* ------------------------------------------------------------- profile */

static void write_look(Writer *w, const Look *l)
{
    int i;
    w8(w, l->skin); w8(w, l->hair); w8(w, l->hair_color); w8(w, l->face); w8(w, l->eyes); w8(w, l->cloth);
    w8(w, l->show_helm);
    for (i = 0; i < NAME_LEN; i++) w8(w, (uint8_t)l->name[i]);
}

static void read_look(Reader *r, Look *l)
{
    int i;
    l->skin = r8(r); l->hair = r8(r); l->hair_color = r8(r); l->face = r8(r); l->eyes = r8(r); l->cloth = r8(r);
    l->show_helm = r8(r);
    for (i = 0; i < NAME_LEN; i++) {
        char c = (char)r8(r);
        l->name[i] = (c >= ' ' && c <= 'Z') || c == 0 ? c : ' ';
    }
    l->name[NAME_LEN] = '\0';
}

static void write_tree(Writer *w, const Profile *p)
{
    int i, j;
    for (i = 0; i < CLASS_SKILLS; i++) { w8(w, p->skill_rank[i]); w8(w, p->skill_enh[i]); w8(w, p->skill_upg[i]); }
    for (i = 0; i < CLASS_PASSIVES; i++) w8(w, p->passive[i]);
    w8(w, p->key_passive);
    for (i = 0; i < BAR_SLOTS; i++) w8(w, p->bar[i]);
    w8(w, p->preset);
    w32(w, (uint32_t)p->skill_points);
    for (i = 0; i < PARAGON_BOARDS; i++)
        for (j = 0; j < BOARD_BYTES; j++)
            w8(w, p->para[i][j]);
    for (i = 0; i < PARAGON_BOARDS; i++) w8(w, p->glyph[i]);
    for (i = 0; i < GLYPH_COUNT; i++) w8(w, p->glyph_lvl[i]);
}

static void read_tree(Reader *r, Profile *p)
{
    int i, j;
    for (i = 0; i < CLASS_SKILLS; i++) { p->skill_rank[i] = r8(r); p->skill_enh[i] = r8(r); p->skill_upg[i] = r8(r); }
    for (i = 0; i < CLASS_PASSIVES; i++) p->passive[i] = r8(r);
    p->key_passive = r8(r);
    for (i = 0; i < BAR_SLOTS; i++) p->bar[i] = r8(r);
    p->preset = r8(r);
    p->skill_points = (int)r32(r);
    for (i = 0; i < PARAGON_BOARDS; i++)
        for (j = 0; j < BOARD_BYTES; j++)
            p->para[i][j] = r8(r);
    for (i = 0; i < PARAGON_BOARDS; i++) p->glyph[i] = r8(r);
    for (i = 0; i < GLYPH_COUNT; i++) p->glyph_lvl[i] = r8(r);
}

static void write_crafting(Writer *w, const Profile *p)
{
    int i, j;
    for (i = 0; i < ASPECT_MAX; i++) w16(w, p->codex[i]);
    wf(w, p->iron); wf(w, p->souls);
    for (i = 0; i < GEM_KINDS; i++)
        for (j = 0; j < GEM_TIERS; j++)
            w16(w, p->gems[i][j]);
    w8(w, p->potion_lvl); w8(w, p->elixir); w32(w, p->elixir_secs);
}

static void read_crafting(Reader *r, Profile *p)
{
    int i, j;
    for (i = 0; i < ASPECT_MAX; i++) p->codex[i] = r16(r);
    p->iron = rf(r); p->souls = rf(r);
    for (i = 0; i < GEM_KINDS; i++)
        for (j = 0; j < GEM_TIERS; j++)
            p->gems[i][j] = r16(r);
    p->potion_lvl = r8(r); p->elixir = r8(r); p->elixir_secs = r32(r);
}

static void write_goals(Writer *w, const Profile *p)
{
    int i;
    for (i = 0; i < BOUNTY_SLOTS; i++) {
        w8(w, p->bounty[i].kind); w8(w, p->bounty[i].arg); w16(w, p->bounty[i].need); w16(w, p->bounty[i].have);
    }
    w32(w, (uint32_t)p->ach); w32(w, (uint32_t)(p->ach >> 32));
    w32(w, (uint32_t)p->lore); w32(w, (uint32_t)(p->lore >> 32));
    w32(w, p->n_goblins); w32(w, p->n_shrines); w32(w, p->n_events); w32(w, p->n_elites);
    w32(w, p->n_bounties); w32(w, p->n_ancestral); w32(w, p->n_mythic);
}

static void read_goals(Reader *r, Profile *p)
{
    int i;
    uint32_t lo;
    for (i = 0; i < BOUNTY_SLOTS; i++) {
        p->bounty[i].kind = r8(r); p->bounty[i].arg = r8(r); p->bounty[i].need = r16(r); p->bounty[i].have = r16(r);
    }
    lo = r32(r);
    p->ach = lo | ((uint64_t)r32(r) << 32);
    lo = r32(r);
    p->lore = lo | (r->version >= 7 ? (uint64_t)r32(r) << 32 : 0);
    p->n_goblins = r32(r); p->n_shrines = r32(r); p->n_events = r32(r); p->n_elites = r32(r);
    p->n_bounties = r32(r); p->n_ancestral = r32(r); p->n_mythic = r32(r);
}

static void write_body(Writer *w, const Profile *p)
{
    int i;
    w8(w, p->cls);
    write_look(w, &p->look);
    w32(w, (uint32_t)p->level); wf(w, p->xp); w32(w, (uint32_t)p->paragon_level); wf(w, p->gold);
    w32(w, (uint32_t)p->floor); w32(w, (uint32_t)p->best_floor);
    for (i = 0; i < SLOT_COUNT; i++) write_item(w, &p->equip[i]);
    for (i = 0; i < BAG_SIZE; i++) write_item(w, &p->bag[i]);
    write_tree(w, p);
    write_crafting(w, p);
    w32(w, p->story_seen); wf(w, p->embers);
    for (i = 0; i < UP_COUNT; i++) w8(w, p->up[i]);
    w32(w, (uint32_t)p->rebirths); w32(w, (uint32_t)p->best_floor_ever);
    w8(w, p->auto_equip); w8(w, p->salvage_upto); w8(w, p->mode); w8(w, p->auto_skills);
    w8(w, p->auto_paragon); w8(w, p->auto_craft); w8(w, p->dmg_numbers); w8(w, p->story_pause);
    w8(w, p->show_fps); w8(w, p->low_power); w8(w, p->lang);
    for (i = 0; i < GLYPH_COUNT; i++) w16(w, p->glyph_xp[i]);
    w32(w, p->save_time); wf(w, p->kpm); wf(w, p->total_kills); wf(w, p->play_seconds); w32(w, p->seed);
    write_goals(w, p);
}

static void read_body(Reader *r, Profile *p)
{
    int i;
    p->cls = r8(r);
    read_look(r, &p->look);
    p->level = (int)r32(r); p->xp = rf(r); p->paragon_level = (int)r32(r); p->gold = rf(r);
    p->floor = (int)r32(r); p->best_floor = (int)r32(r);
    for (i = 0; i < SLOT_COUNT; i++) read_item(r, &p->equip[i]);
    for (i = 0; i < BAG_SIZE; i++) read_item(r, &p->bag[i]);
    read_tree(r, p);
    read_crafting(r, p);
    p->story_seen = r->version >= 6 ? r32(r) : r16(r);
    p->embers = rf(r);
    for (i = 0; i < UP_COUNT; i++) p->up[i] = r8(r);
    p->rebirths = (int)r32(r); p->best_floor_ever = (int)r32(r);
    p->auto_equip = r8(r); p->salvage_upto = r8(r); p->mode = r8(r); p->auto_skills = r8(r);
    p->auto_paragon = r8(r); p->auto_craft = r8(r); p->dmg_numbers = r8(r); p->story_pause = r8(r);
    p->show_fps = r8(r); p->low_power = r8(r);
    if (r->version < 7 && p->level >= LEVEL_CAP)
        p->xp = 0;                   /* paragon progress is counted in kills since v7 */
    p->lang = r->version >= 5 ? r8(r) : (uint8_t)lang_get();
    for (i = 0; i < GLYPH_COUNT; i++)
        p->glyph_xp[i] = r->version >= 7 ? r16(r) : 0;
    p->save_time = r32(r); p->kpm = rf(r); p->total_kills = rf(r); p->play_seconds = rf(r); p->seed = r32(r);
    if (r->version >= 6)
        read_goals(r, p);           /* older saves start with empty bounty slots */
}

static bool tree_sane(const Profile *p)
{
    int i;
    for (i = 0; i < CLASS_SKILLS; i++)
        if (p->skill_rank[i] > SKILL_MAX_RANK || p->skill_enh[i] > 1 || p->skill_upg[i] > 2)
            return false;
    for (i = 0; i < CLASS_PASSIVES; i++)
        if (p->passive[i] > PASSIVE_MAX_RANK)
            return false;
    for (i = 0; i < BAR_SLOTS; i++)
        if (p->bar[i] != NO_SKILL && p->bar[i] >= CLASS_SKILLS)
            return false;
    for (i = 0; i < GLYPH_COUNT; i++)
        if (p->glyph_lvl[i] > GLYPH_MAX_LEVEL)
            return false;
    return p->key_passive <= CLASS_KEYS && p->preset < PRESETS && p->skill_points >= 0;
}

static bool profile_sane(const Profile *p)
{
    int i;
    if (p->cls >= CLASS_COUNT || p->level < 1 || p->level > LEVEL_CAP || p->floor < 1 || p->floor > 1000000)
        return false;
    if (p->best_floor < p->floor || p->gold < 0 || p->xp < 0 || p->embers < 0 || p->paragon_level < 0)
        return false;
    if (p->lang >= LANG_COUNT)
        return false;
    for (i = 0; i < BOUNTY_SLOTS; i++)
        if (!bounty_sane(&p->bounty[i]))
            return false;
    if (p->mode > MODE_FARM || p->dmg_numbers >= DMGNUM_COUNT || p->elixir >= ELIX_COUNT || p->iron < 0
        || p->souls < 0)
        return false;
    for (i = 0; i < SLOT_COUNT; i++)
        if (p->equip[i].used && !((p->equip[i].slot == i) || ((i == SLOT_RING1 || i == SLOT_RING2)
                                                              && p->equip[i].slot == SLOT_RING1)))
            return false;
    return tree_sane(p);
}

size_t save_serialize(const Profile *p, uint8_t *buf, size_t cap)
{
    Writer w;
    uint32_t len;
    if (cap < 16)
        return 0;
    w.p = buf; w.n = 12; w.cap = cap; w.ok = true;   /* header filled in below */
    memcpy(buf, "ADSV", 4);
    write_body(&w, p);
    if (!w.ok || w.n + 4 > cap)
        return 0;
    len = (uint32_t)(w.n - 12);
    buf[4] = SAVE_VERSION; buf[5] = buf[6] = buf[7] = 0;
    buf[8] = (uint8_t)len; buf[9] = (uint8_t)(len >> 8);
    buf[10] = (uint8_t)(len >> 16); buf[11] = (uint8_t)(len >> 24);
    w32(&w, save_crc32(buf, w.n));
    return w.ok ? w.n : 0;
}

SaveStatus save_deserialize(Profile *p, const uint8_t *buf, size_t len)
{
    static Profile tmp;   /* large: keep it off the calculator's small stack */
    Reader r;
    uint32_t body, crc;
    if (len < 16 || memcmp(buf, "ADSV", 4) != 0 || buf[4] < 1 || buf[4] > SAVE_VERSION)
        return SAVE_CORRUPT;
    body = (uint32_t)buf[8] | ((uint32_t)buf[9] << 8) | ((uint32_t)buf[10] << 16) | ((uint32_t)buf[11] << 24);
    if ((size_t)body + 16 != len)
        return SAVE_CORRUPT;
    crc = (uint32_t)buf[len - 4] | ((uint32_t)buf[len - 3] << 8)
        | ((uint32_t)buf[len - 2] << 16) | ((uint32_t)buf[len - 1] << 24);
    if (crc != save_crc32(buf, len - 4))
        return SAVE_CORRUPT;
    memset(&tmp, 0, sizeof tmp);
    if (buf[4] < 4) {                     /* v1-v3: the original game */
        SaveStatus st = save_legacy_read(&tmp, buf, len, buf[4]);
        if (st == SAVE_OK)
            *p = tmp;
        return st;
    }
    r.p = buf; r.n = 12; r.len = len - 4; r.ok = true; r.version = buf[4];
    read_body(&r, &tmp);
    if (!r.ok || r.n != r.len || !profile_sane(&tmp))
        return SAVE_CORRUPT;
    *p = tmp;
    return SAVE_OK;
}

SaveStatus save_load(Profile *p, const char *path)
{
    static uint8_t buf[SAVE_MAX_BYTES + 1];
    size_t n;
    FILE *f;
    if (!path || !(f = fopen(path, "rb")))
        return SAVE_MISSING;
    n = fread(buf, 1, sizeof buf, f);
    fclose(f);
    return save_deserialize(p, buf, n);
}

static bool write_file(const char *path, const uint8_t *buf, size_t n)
{
    FILE *f = fopen(path, "wb");
    bool ok;
    if (!f)
        return false;
    ok = fwrite(buf, 1, n, f) == n;
    ok = (fclose(f) == 0) && ok;
    return ok;
}

SaveStatus save_write(const Profile *p, const char *path)
{
    static uint8_t buf[SAVE_MAX_BYTES];
    char tmp[300];
    size_t n = save_serialize(p, buf, sizeof buf);
    if (!path || n == 0 || strlen(path) + 5 > sizeof tmp)
        return SAVE_IO_ERROR;
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    if (!write_file(tmp, buf, n))
        return SAVE_IO_ERROR;
    remove(path);
    if (rename(tmp, path) != 0) {
        remove(tmp);
        return write_file(path, buf, n) ? SAVE_OK : SAVE_IO_ERROR;
    }
    return SAVE_OK;
}
