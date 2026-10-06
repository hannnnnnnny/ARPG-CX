#include "progress.h"
#include "goals.h"
#include "aspects.h"
#include "balance.h"
#include "build.h"
#include "paragon.h"
#include "skills.h"
#include "stats.h"
#include "../i18n/i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------- new hero */

void prog_default_look(Look *l, int cls, uint32_t seed)
{
    static const char *const a[8] = { "KA", "VA", "MOR", "SE", "TH", "AL", "RI", "DRA" };
    static const char *const b[8] = { "REN", "LIA", "DUN", "VYN", "ORA", "THOS", "IKA", "MAR" };
    Rng r;
    rng_seed(&r, seed ^ 0x10C4u);
    memset(l, 0, sizeof *l);
    l->skin = (uint8_t)rng_range(&r, 0, 5);
    l->hair = (uint8_t)rng_range(&r, 0, 5);
    l->hair_color = (uint8_t)rng_range(&r, 0, 7);
    l->face = (uint8_t)(cls == CLASS_BARBARIAN ? 1 : rng_range(&r, 0, 4));
    l->eyes = (uint8_t)rng_range(&r, 0, 3);
    l->cloth = (uint8_t)rng_range(&r, 0, 7);
    l->show_helm = 1;
    snprintf(l->name, sizeof l->name, "%s%s", a[rng_range(&r, 0, 7)], b[rng_range(&r, 0, 7)]);
}

void prog_new(Profile *p, uint32_t seed, int cls)
{
    Rng r;
    memset(p, 0, sizeof *p);
    p->cls = (uint8_t)CLAMP(cls, 0, CLASS_COUNT - 1);
    p->level = 1;
    p->floor = p->best_floor = 1;
    p->auto_equip = 1;
    p->salvage_upto = RAR_RARE;           /* legendaries and better are kept */
    p->mode = MODE_PUSH;
    p->auto_skills = 1;
    p->auto_paragon = 1;
    p->auto_craft = 1;
    p->kpm = 2.0;
    p->lang = (uint8_t)lang_get();
    p->seed = seed ? seed : 0xA5E1u;
    memset(p->bar, NO_SKILL, sizeof p->bar);
    prog_default_look(&p->look, p->cls, p->seed);
    p->skill_points = 1;                 /* the first basic skill */
    build_auto_spend(p);
    rng_seed(&r, p->seed);
    /* A starting kit so the first floor is a fight, not a slaughter. */
    item_roll_slot(&p->equip[SLOT_WEAPON], &r, 1, 0, RAR_COMMON, SLOT_WEAPON, p->cls);
    item_roll_slot(&p->equip[SLOT_CHEST], &r, 1, 0, RAR_COMMON, SLOT_CHEST, p->cls);
    goals_refill(p, &r);
}

/* ------------------------------------------------------------ experience */

int prog_paragon_available(const Profile *p)
{
    return paragon_points_for(p->level, p->paragon_level) - paragon_spent(p);
}

double prog_xp_needed(const Profile *p)
{
    return p->level < LEVEL_CAP ? xp_to_next(p->level) : paragon_xp(p->paragon_level);
}

int prog_add_xp(Profile *p, double xp)
{
    int levels = 0;
    p->xp += xp;
    /* Cap iterations so a corrupt or huge value can never freeze the game. */
    while (p->xp >= prog_xp_needed(p) && levels < 1000) {
        p->xp -= prog_xp_needed(p);
        if (p->level < LEVEL_CAP) {
            p->level++;
            p->skill_points++;
        } else {
            p->paragon_level++;
        }
        levels++;
    }
    if (levels && p->auto_skills)
        build_auto_spend(p);
    if (levels && p->auto_paragon)
        paragon_auto(p);
    return levels;
}

void prog_add_gold(Profile *p, double gold)
{
    if (gold > 0)
        p->gold += gold;
}

/* ------------------------------------------------------------------ loot */

static int free_bag_slot(const Profile *p)
{
    int i;
    for (i = 0; i < BAG_SIZE; i++)
        if (!p->bag[i].used)
            return i;
    return -1;
}

/* Worst unlocked bag item by rarity, then item level. */
static int worst_bag_item(const Profile *p)
{
    int i, worst = -1;
    for (i = 0; i < BAG_SIZE; i++) {
        const Item *a = &p->bag[i];
        if (!a->used || a->locked)
            continue;
        if (worst < 0 || a->rarity < p->bag[worst].rarity
            || (a->rarity == p->bag[worst].rarity && a->ilvl < p->bag[worst].ilvl))
            worst = i;
    }
    return worst;
}

static void return_gem(Profile *p, int gem)
{
    if (gem != GEM_NONE && gem / GEM_TIERS < GEM_KINDS)
        p->gems[gem / GEM_TIERS][gem % GEM_TIERS]++;
}

static void salvage_item(Profile *p, const Item *it, double *gold)
{
    *gold += salvage_gold(it);
    p->gold += salvage_gold(it);
    p->iron += salvage_iron(it);
    p->souls += salvage_souls(it);
    prog_learn_aspect(p, it);
    if (it->sockets >= 1)               /* socketed gems go back to the pouch */
        return_gem(p, it->gem[0]);
    if (it->sockets >= 2)
        return_gem(p, it->gem[1]);
}

static void equip_into(Profile *p, const Item *it, int slot)
{
    Item old = p->equip[slot];
    p->equip[slot] = *it;
    p->equip[slot].locked = 0;
    if (old.used) {
        int s = free_bag_slot(p);
        double g = 0;
        if (s >= 0)
            p->bag[s] = old;
        else
            salvage_item(p, &old, &g);
    }
}

bool prog_learn_aspect(Profile *p, const Item *it)
{
    int before;
    if (!it->used || it->rarity != RAR_LEGEND || it->power == 0 || it->power >= ASPECT_MAX)
        return false;
    before = p->codex[it->power];
    codex_learn(p, it->power, it->power_roll);
    return p->codex[it->power] != before;
}

LootResult prog_handle_loot(Profile *p, const Item *it, double *gold_gained)
{
    int slot, worst, target = item_target_slot(p, it);
    *gold_gained = 0;
    prog_learn_aspect(p, it);
    if (p->auto_equip && (!p->equip[target].used || item_upgrade_ratio(p, it) > 0.005)) {
        equip_into(p, it, target);
        return LOOT_EQUIPPED;
    }
    if (p->salvage_upto != 255 && it->rarity <= p->salvage_upto) {
        salvage_item(p, it, gold_gained);
        return LOOT_SALVAGED;
    }
    slot = free_bag_slot(p);
    if (slot >= 0) {
        p->bag[slot] = *it;
        p->bag[slot].locked = 0;
        return LOOT_BAGGED;
    }
    /* Bag full: keep the better of the new item and the worst bag item. */
    worst = worst_bag_item(p);
    if (worst >= 0 && (p->bag[worst].rarity < it->rarity
                       || (p->bag[worst].rarity == it->rarity && p->bag[worst].ilvl < it->ilvl))) {
        salvage_item(p, &p->bag[worst], gold_gained);
        p->bag[worst] = *it;
        p->bag[worst].locked = 0;
    } else {
        salvage_item(p, it, gold_gained);
    }
    return LOOT_BAG_FULL_SALVAGED;
}

bool prog_equip_from_bag(Profile *p, int idx)
{
    Item it;
    if (idx < 0 || idx >= BAG_SIZE || !p->bag[idx].used)
        return false;
    it = p->bag[idx];
    memset(&p->bag[idx], 0, sizeof p->bag[idx]);
    equip_into(p, &it, item_target_slot(p, &it));
    return true;
}

bool prog_salvage_bag(Profile *p, int idx, double *gold_gained)
{
    *gold_gained = 0;
    if (idx < 0 || idx >= BAG_SIZE || !p->bag[idx].used || p->bag[idx].locked)
        return false;
    salvage_item(p, &p->bag[idx], gold_gained);
    memset(&p->bag[idx], 0, sizeof p->bag[idx]);
    return true;
}

int prog_salvage_all_unlocked(Profile *p, double *gold_gained)
{
    int i, n = 0;
    double g, total = 0;
    for (i = 0; i < BAG_SIZE; i++)
        if (prog_salvage_bag(p, i, &g)) {
            total += g;
            n++;
        }
    *gold_gained = total;
    return n;
}

/* ------------------------------------------------------------ crafting */

static CraftResult pay(Profile *p, double gold, double iron, double souls)
{
    if (p->gold < gold)
        return CRAFT_NO_GOLD;
    if (p->iron < iron || p->souls < souls)
        return CRAFT_NO_MATS;
    p->gold -= gold;
    p->iron -= iron;
    p->souls -= souls;
    return CRAFT_OK;
}

static Item *slot_item(Profile *p, Slot s)
{
    return ((unsigned)s < SLOT_COUNT && p->equip[s].used) ? &p->equip[s] : NULL;
}

CraftResult prog_masterwork(Profile *p, Slot s, Rng *r)
{
    Item *it = slot_item(p, s);
    CraftResult res;
    if (!it || it->mw >= MW_MAX)
        return CRAFT_INVALID;
    res = pay(p, masterwork_gold(it), masterwork_iron(it), masterwork_souls(it));
    if (res == CRAFT_OK)
        item_masterwork(it, r);
    return res;
}

CraftResult prog_temper(Profile *p, Slot s, TemperRecipe t, Rng *r)
{
    Item *it = slot_item(p, s);
    Item probe;
    CraftResult res;
    if (!it || it->temper_left == 0 || !temper_allowed(it, t))
        return CRAFT_INVALID;
    probe = *it;
    if (!item_temper(&probe, r, t))       /* check it can work before paying */
        return CRAFT_INVALID;
    res = pay(p, temper_gold(it), 1, 0);
    if (res == CRAFT_OK)
        *it = probe;
    return res;
}

CraftResult prog_enchant(Profile *p, Slot s, int affix, Affix out[2], Rng *r)
{
    Item *it = slot_item(p, s);
    if (!it || !item_enchant_options(it, affix, r, out, p->cls))
        return CRAFT_INVALID;
    return pay(p, enchant_gold(it), 0, it->rarity >= RAR_LEGEND ? 1 : 0);
}

CraftResult prog_imprint(Profile *p, Slot s, int aspect)
{
    Item *it = slot_item(p, s);
    CraftResult res;
    if (!it || !codex_known(p, aspect) || !item_can_imprint(it, aspect, p->cls))
        return CRAFT_INVALID;
    res = pay(p, imprint_gold(it), 0, 2);
    if (res == CRAFT_OK)
        item_imprint(it, aspect, codex_roll(p, aspect));
    return res;
}

CraftResult prog_add_socket(Profile *p, Slot s)
{
    Item *it = slot_item(p, s);
    CraftResult res;
    if (!it || it->sockets >= (s == SLOT_WEAPON ? 2 : 1))
        return CRAFT_INVALID;
    res = pay(p, socket_gold(it), 5, 1);
    if (res == CRAFT_OK) {
        it->gem[it->sockets] = GEM_NONE;
        it->sockets++;
    }
    return res;
}

bool prog_unsocket(Profile *p, Slot s, int socket)
{
    Item *it = slot_item(p, s);
    if (!it || socket < 0 || socket >= it->sockets || it->gem[socket] == GEM_NONE)
        return false;
    return_gem(p, it->gem[socket]);
    it->gem[socket] = GEM_NONE;
    return true;
}

bool prog_socket_gem(Profile *p, Slot s, int socket, int gem)
{
    Item *it = slot_item(p, s);
    int kind = gem / GEM_TIERS, tier = gem % GEM_TIERS;
    if (!it || socket < 0 || socket >= it->sockets || gem < 0 || kind >= GEM_KINDS || p->gems[kind][tier] == 0)
        return false;
    prog_unsocket(p, s, socket);
    p->gems[kind][tier]--;
    it->gem[socket] = (uint8_t)gem;
    return true;
}

CraftResult prog_craft_gem(Profile *p, int kind, int tier)
{
    CraftResult res;
    if (kind < 0 || kind >= GEM_KINDS || tier < 0 || tier >= GEM_TIERS - 1 || p->gems[kind][tier] < 3)
        return CRAFT_INVALID;
    res = pay(p, gem_craft_gold(tier), 0, 0);
    if (res == CRAFT_OK) {
        p->gems[kind][tier] -= 3;
        p->gems[kind][tier + 1]++;
    }
    return res;
}

CraftResult prog_gamble(Profile *p, Slot s, Rng *r, Item *out)
{
    CraftResult res = pay(p, gamble_gold(p->best_floor), 0, 0);
    if (res == CRAFT_OK)   /* the gambler's stock is luckier than the floor */
        item_roll_slot(out, r, p->best_floor, p->up[UP_FORTUNE] + 8, RAR_MAGIC, s, p->cls);
    return res;
}

CraftResult prog_buy_elixir(Profile *p, ElixirKind k)
{
    CraftResult res;
    if (k <= ELIX_NONE || k >= ELIX_COUNT)
        return CRAFT_INVALID;
    res = pay(p, elixir_gold(p->best_floor), 0, 0);
    if (res == CRAFT_OK) {
        p->elixir = (uint8_t)k;
        p->elixir_secs = 30 * 60;
    }
    return res;
}

CraftResult prog_upgrade_potion(Profile *p)
{
    CraftResult res;
    if (p->potion_lvl >= POTION_MAX_LVL)
        return CRAFT_INVALID;
    res = pay(p, potion_upgrade_gold(p->potion_lvl), 0, 0);
    if (res == CRAFT_OK)
        p->potion_lvl++;
    return res;
}

/* ------------------------------------------------------- idle crafting */

/* Keep a reserve so a respec or a gamble is always possible. */
static bool can_spend(const Profile *p, double cost)
{
    return p->gold - cost >= 2.0 * respec_gold(p->level, p->best_floor);
}

static int auto_masterwork(Profile *p, Rng *r)
{
    int s, best = -1;
    double bc = 0;
    for (s = 0; s < SLOT_COUNT; s++) {
        const Item *it = &p->equip[s];
        double c;
        if (!it->used || it->mw >= MW_MAX || it->rarity < RAR_RARE)
            continue;
        c = masterwork_gold(it);
        if (best < 0 || c < bc) {
            best = s;
            bc = c;
        }
    }
    return best >= 0 && can_spend(p, bc) && prog_masterwork(p, (Slot)best, r) == CRAFT_OK;
}

static TemperRecipe preferred_recipe(const Profile *p, const Item *it, int nth)
{
    const BuildPreset *pr = &class_defs[p->cls % CLASS_COUNT].preset[p->preset % PRESETS];
    if (temper_allowed(it, TR_WEAPONRY))
        return nth == 0 ? TR_WEAPONRY : pr->element != EL_PHYS ? TR_ELEMENTS : TR_FINESSE;
    if (it->slot == SLOT_BOOTS)
        return nth == 0 ? TR_PROFITEER : TR_ENDURANCE;
    return nth == 0 ? TR_ENDURANCE : TR_SUSTAIN;
}

static int tempered_count(const Item *it)
{
    int i, n = 0;
    for (i = 0; i < it->naff; i++)
        n += (it->aff[i].flags & AFX_TEMPERED) != 0;
    return n;
}

static int auto_temper(Profile *p, Rng *r)
{
    int s;
    for (s = 0; s < SLOT_COUNT; s++) {
        const Item *it = &p->equip[s];
        int n;
        if (!it->used || it->rarity < RAR_RARE || it->temper_left == 0 || (n = tempered_count(it)) >= MAX_TEMPER)
            continue;
        if (can_spend(p, temper_gold(it)) && prog_temper(p, (Slot)s, preferred_recipe(p, it, n), r) == CRAFT_OK)
            return 1;
    }
    return 0;
}

static bool aspect_equipped(const Profile *p, int aspect)
{
    int s;
    for (s = 0; s < SLOT_COUNT; s++)
        if (p->equip[s].used && p->equip[s].rarity == RAR_LEGEND && p->equip[s].power == aspect)
            return true;
    return false;
}

static bool aspect_wanted(const BuildPreset *pr, int aspect)
{
    int i;
    for (i = 0; i < 4; i++)
        if (pr->aspects[i] == aspect)
            return true;
    return false;
}

static int auto_imprint(Profile *p)
{
    const BuildPreset *pr = &class_defs[p->cls % CLASS_COUNT].preset[p->preset % PRESETS];
    int i, s;
    for (i = 0; i < 4; i++) {
        int a = pr->aspects[i];
        if (!codex_known(p, a) || aspect_equipped(p, a))
            continue;
        for (s = 0; s < SLOT_COUNT; s++) {
            const Item *it = &p->equip[s];
            bool spare = it->rarity == RAR_RARE || (it->rarity == RAR_LEGEND && !aspect_wanted(pr, it->power));
            if (spare && item_can_imprint(it, a, p->cls) && can_spend(p, imprint_gold(it))
                && prog_imprint(p, (Slot)s, a) == CRAFT_OK)
                return 1;
        }
    }
    return 0;
}

static int best_gem(const Profile *p, int kind)
{
    int t;
    for (t = GEM_TIERS - 1; t >= 0; t--)
        if (p->gems[kind][t] > 0)
            return gem_id(kind, t);
    return -1;
}

static int auto_gems(Profile *p)
{
    int s, k, t, done = 0;
    for (k = 0; k < GEM_KINDS; k++)          /* 3 of a kind -> the next tier */
        for (t = 0; t < GEM_TIERS - 1; t++)
            while (p->gems[k][t] >= 3 && can_spend(p, gem_craft_gold(t)) && prog_craft_gem(p, k, t) == CRAFT_OK)
                done++;
    for (s = 0; s < SLOT_COUNT; s++) {
        Item *it = &p->equip[s];
        int i, kind = s == SLOT_WEAPON ? GEM_EMERALD : (s == SLOT_AMULET || s >= SLOT_RING1 || s == SLOT_OFFHAND)
                    ? GEM_DIAMOND : GEM_RUBY;
        int g = best_gem(p, kind);
        for (i = 0; i < it->sockets && g >= 0; i++)
            if (it->gem[i] == GEM_NONE || it->gem[i] % GEM_TIERS < g % GEM_TIERS) {
                done += prog_socket_gem(p, (Slot)s, i, g);
                g = best_gem(p, kind);
            }
    }
    return done;
}

int prog_auto_craft(Profile *p, Rng *r)
{
    int n = 0, k;
    if (!p->auto_craft)
        return 0;
    n += auto_gems(p);
    n += auto_imprint(p);
    for (k = 0; k < 4; k++)                 /* a few blacksmith jobs per visit */
        n += auto_temper(p, r) + auto_masterwork(p, r);
    if (p->elixir_secs == 0 && p->best_floor >= 10 && can_spend(p, 3.0 * elixir_gold(p->best_floor)))
        n += prog_buy_elixir(p, ELIX_PRECISION) == CRAFT_OK;
    if (can_spend(p, 4.0 * potion_upgrade_gold(p->potion_lvl)))
        n += prog_upgrade_potion(p) == CRAFT_OK;
    return n;
}

/* -------------------------------------------------------------- respec */

double prog_respec_cost(const Profile *p)
{
    return respec_gold(p->level, p->best_floor);
}

static bool pay_respec(Profile *p)
{
    double cost = prog_respec_cost(p);
    if (p->gold < cost)
        return false;
    p->gold -= cost;
    return true;
}

bool prog_respec_skills(Profile *p)
{
    if (!pay_respec(p))
        return false;
    skill_refund_all(p);
    if (p->auto_skills)
        build_auto_spend(p);
    return true;
}

bool prog_switch_preset(Profile *p, int preset)
{
    if (!pay_respec(p))
        return false;
    build_apply_preset(p, preset);
    if (p->auto_paragon) {          /* glyph choice follows the build */
        memset(p->glyph, 0, sizeof p->glyph);
        paragon_auto(p);
    }
    return true;
}

bool prog_respec_paragon(Profile *p)
{
    if (!pay_respec(p))
        return false;
    paragon_refund(p);
    if (p->auto_paragon)
        paragon_auto(p);
    return true;
}

/* -------------------------------------------------------------- floors */

void prog_floor_cleared(Profile *p, bool boss)
{
    int b;
    if (p->mode == MODE_PUSH)
        p->floor++;
    p->best_floor = MAX(p->best_floor, p->floor);
    p->best_floor_ever = MAX(p->best_floor_ever, p->best_floor);
    /* Torment guardians empower the socketed glyphs (the pit, idle style). */
    if (boss && p->floor > TORMENT_FLOOR)
        for (b = 0; b < PARAGON_BOARDS; b++)
            if (p->glyph[b] && p->glyph_lvl[(p->glyph[b] - 1) % GLYPH_COUNT] < GLYPH_MAX_LEVEL)
                p->glyph_lvl[(p->glyph[b] - 1) % GLYPH_COUNT]++;
}

void prog_died(Profile *p)
{
    /* Fall back a floor and farm it: the classic idle "wall" loop. */
    if (p->floor > 1)
        p->floor--;
}

void prog_tick_second(Profile *p)
{
    if (p->elixir_secs > 0 && --p->elixir_secs == 0)
        p->elixir = ELIX_NONE;
}

/* ------------------------------------------------------------- rebirth */

bool prog_can_rebirth(const Profile *p)
{
    return p->best_floor >= REBIRTH_MIN_FLOOR;
}

/* Bounties, renown, lost pages and the event counters belong to the
 * player, not to one life. */
static void keep_goals(Profile *p, const Profile *keep)
{
    memcpy(p->bounty, keep->bounty, sizeof p->bounty);
    p->ach = keep->ach;
    p->lore = keep->lore;
    p->n_goblins = keep->n_goblins;
    p->n_shrines = keep->n_shrines;
    p->n_events = keep->n_events;
    p->n_elites = keep->n_elites;
    p->n_bounties = keep->n_bounties;
    p->n_ancestral = keep->n_ancestral;
    p->n_mythic = keep->n_mythic;
}

double prog_rebirth(Profile *p, int new_cls)
{
    Profile keep = *p;
    double gained = ember_reward(p->best_floor);
    if (!prog_can_rebirth(p))
        return 0;
    prog_new(p, keep.seed * 1664525u + 1013904223u, new_cls);
    /* Meta progress, the codex, appearance and preferences survive. */
    p->look = keep.look;
    memcpy(p->codex, keep.codex, sizeof p->codex);
    memcpy(p->glyph_lvl, keep.glyph_lvl, sizeof p->glyph_lvl);
    p->embers = keep.embers + gained;
    memcpy(p->up, keep.up, sizeof p->up);
    p->rebirths = keep.rebirths + 1;
    p->best_floor_ever = MAX(keep.best_floor_ever, keep.best_floor);
    p->auto_equip = keep.auto_equip;
    p->salvage_upto = keep.salvage_upto;
    p->auto_skills = keep.auto_skills;
    p->auto_paragon = keep.auto_paragon;
    p->auto_craft = keep.auto_craft;
    p->dmg_numbers = keep.dmg_numbers;
    p->story_pause = keep.story_pause;
    p->story_seen = keep.story_seen;   /* the story is told once per profile */
    keep_goals(p, &keep);
    p->show_fps = keep.show_fps;
    p->low_power = keep.low_power;
    p->total_kills = keep.total_kills;
    p->play_seconds = keep.play_seconds;
    p->floor = p->best_floor = 1 + 5 * p->up[UP_START];
    return gained;
}

bool prog_buy_upgrade(Profile *p, UpgradeId id)
{
    double cost = upgrade_cost(p->up[id]);
    if (p->up[id] >= upgrade_max(id) || p->embers < cost)
        return false;
    p->embers -= cost;
    p->up[id]++;
    return true;
}

/* ------------------------------------------------------------- offline */

uint32_t prog_offline_cap_seconds(const Profile *p)
{
    return (uint32_t)(OFFLINE_BASE_HOURS + 2 * p->up[UP_PATIENCE]) * 3600u;
}

static void offline_loot(Profile *p, OfflineReport *rep, int n)
{
    Rng r;
    int i;
    rng_seed(&r, p->seed ^ (uint32_t)p->save_time);
    for (i = 0; i < n; i++) {
        Item it;
        double g;
        item_roll(&it, &r, p->floor, p->up[UP_FORTUNE], RAR_COMMON, p->cls);
        if (prog_handle_loot(p, &it, &g) == LOOT_EQUIPPED || g == 0)
            rep->items_kept++;
        else
            rep->items_salvaged++;
    }
    for (i = 0; i < 20 && prog_auto_craft(p, &r) > 0; i++)
        ;
}

bool prog_offline(Profile *p, uint32_t now, OfflineReport *rep)
{
    Stats st;
    double minutes;
    memset(rep, 0, sizeof *rep);
    /* Clock went backwards or never saved: credit nothing. */
    if (p->save_time == 0 || now <= p->save_time + 60)
        return false;
    rep->raw_seconds = now - p->save_time;
    rep->seconds = MIN(rep->raw_seconds, prog_offline_cap_seconds(p));
    minutes = rep->seconds / 60.0;
    stats_compute(&st, p);
    /* Offline runs at half efficiency of the measured live kill rate. */
    rep->kills = floor(CLAMP(p->kpm, 0.5, 600.0) * minutes * 0.5);
    rep->gold = rep->kills * kill_gold(p->floor) * (1.0 + st.gold_pct / 100.0);
    rep->xp = rep->kills * kill_xp(p->floor) * (1.0 + st.xp_pct / 100.0);
    prog_add_gold(p, rep->gold);
    rep->levels = prog_add_xp(p, rep->xp);
    p->total_kills += rep->kills;
    p->elixir_secs = rep->seconds >= p->elixir_secs ? 0 : p->elixir_secs - rep->seconds;
    offline_loot(p, rep, (int)MIN(rep->kills * 0.06, 40.0));
    p->save_time = now;
    return rep->kills > 0;
}
