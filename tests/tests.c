/*
 * tests.c - host-side tests for Ashen Depths (no dependencies).
 *
 * Covers the Diablo IV style systems: loot rules (rarities, implicits,
 * greater affixes, uniques, mythics), crafting (tempering, masterwork,
 * enchanting, imprinting, gems), the skill tree and build presets, the
 * paragon board, the damage pipeline, saves (v5 and converted v1-v4),
 * dungeon connectivity, the five languages and hours of idle play per class.
 */
#include "../src/core/platform.h"
#include "../src/core/bignum.h"
#include "../src/game/aspects.h"
#include "../src/game/balance.h"
#include "../src/game/items.h"
#include "../src/game/paragon.h"
#include "../src/game/progress.h"
#include "../src/game/save.h"
#include "../src/game/session.h"
#include "../src/game/skills.h"
#include "../src/game/build.h"
#include "../src/game/story.h"
#include "../src/game/game.h"
#include "../src/game/stats.h"
#include "../src/game/world_int.h"
#include "../src/gfx/sprites.h"
#include "../src/data/icons.h"
#include "../src/gfx/font.h"
#include "../src/i18n/i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int g_fail, g_checks;
#define CHECK(c) do { g_checks++; if (!(c)) { g_fail++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

bool plat_init(void) { return true; }
void plat_shutdown(void) {}
uint32_t plat_read_buttons(void) { return 0; }
void plat_present(const uint16_t *fb) { (void)fb; }
uint32_t plat_time_us(void) { return 0; }
void plat_wait(void) {}
bool plat_quit_requested(void) { return false; }
const char *plat_save_path(void) { return NULL; }
const char *plat_name(void) { return "TEST"; }
const char *plat_clock_desc(void) { return "TEST"; }
const char *const *plat_control_lines(void) { return NULL; }

static Profile P, Q;
static Session S;

static int count_flag(const Item *it, int flag)
{
    int i, n = 0;
    for (i = 0; i < it->naff; i++)
        n += (it->aff[i].flags & flag) != 0;
    return n;
}

static int count_plain(const Item *it)
{
    return it->naff - count_flag(it, AFX_IMPLICIT) - count_flag(it, AFX_TEMPERED);
}

/* ---------------------------------------------------------------- format */

static void test_format(void)
{
    char b[24];
    fmt_num(b, sizeof b, 0);         CHECK(!strcmp(b, "0"));
    fmt_num(b, sizeof b, 9999);      CHECK(!strcmp(b, "9999"));
    fmt_num(b, sizeof b, 12345);     CHECK(!strcmp(b, "12.3K"));
    fmt_num(b, sizeof b, 4.56e6);    CHECK(!strcmp(b, "4.56M"));
    fmt_num(b, sizeof b, 1e40);      CHECK(b[0] == '1' && strchr(b, 'E'));
    fmt_num(b, sizeof b, NAN);       CHECK(!strcmp(b, "0"));
}

/* ---------------------------------------------------------------- items */

static void check_item_shape(const Item *it, int ilvl)
{
    static const int plain[RAR_COUNT] = { 0, 1, 2, 3, 4, 4 };
    int i;
    char name[48], line[96];
    CHECK(it->used && it->slot < SLOT_COUNT && it->slot != SLOT_RING2);
    CHECK(count_plain(it) == plain[it->rarity]);
    CHECK(count_flag(it, AFX_IMPLICIT) <= 1);
    CHECK(it->temper_left == TEMPER_CHARGES && it->mw == 0 && it->enchant == 0xFF);
    if (it->rarity == RAR_LEGEND) CHECK(aspect_def(it->power) != NULL);
    if (it->rarity >= RAR_UNIQUE) CHECK(unique_def(it->power) != NULL);
    if (it->rarity == RAR_MYTHIC) CHECK(it->ancestral && count_flag(it, AFX_GREATER) == 4);
    if (it->ancestral && it->rarity != RAR_MYTHIC)
        CHECK(count_flag(it, AFX_GREATER) >= 1 && count_flag(it, AFX_GREATER) <= 3);
    if (!it->ancestral) CHECK(count_flag(it, AFX_GREATER) == 0);
    if (ilvl <= 30) CHECK(!it->ancestral);
    for (i = 0; i < it->naff; i++) {
        CHECK(it->aff[i].type < AF_COUNT && it->aff[i].value > 0);
        if (it->rarity < RAR_UNIQUE && !(it->aff[i].flags & AFX_IMPLICIT))
            CHECK(affix_allowed((Slot)it->slot, (AffixType)it->aff[i].type));
        affix_text(line, sizeof line, it, i, 0);
        CHECK(line[0] != 0);
    }
    item_name(name, sizeof name, it);
    CHECK(name[0] != 0 && strlen(name) < sizeof name - 1);
    CHECK(item_power(it) >= 100);
}

static void test_items(void)
{
    Rng r;
    int i, counts[RAR_COUNT] = { 0 }, anc = 0, imp_weapons = 0;
    rng_seed(&r, 42);
    for (i = 0; i < 30000; i++) {
        Item it;
        int ilvl = 1 + i % 90;
        item_roll(&it, &r, ilvl, 0, RAR_COMMON, i % CLASS_COUNT);
        check_item_shape(&it, ilvl);
        counts[it.rarity]++;
        anc += it.ancestral;
        if (it.slot == SLOT_WEAPON)
            imp_weapons += count_flag(&it, AFX_IMPLICIT) == 1 && item_weapon_kind(&it) >= 0;
    }
    CHECK(counts[RAR_COMMON] > counts[RAR_MAGIC] && counts[RAR_MAGIC] > counts[RAR_RARE]);
    CHECK(counts[RAR_RARE] > counts[RAR_LEGEND] && counts[RAR_LEGEND] > counts[RAR_UNIQUE]);
    CHECK(counts[RAR_UNIQUE] > 0 && anc > 0 && imp_weapons > 0);
    printf("  rarity mix: C%d M%d R%d L%d U%d MY%d, ancestral %d\n", counts[0], counts[1], counts[2], counts[3],
           counts[4], counts[5], anc);
}

static void test_uniques(void)
{
    Rng r;
    int u;
    rng_seed(&r, 5);
    for (u = 1; u <= unique_count; u++) {
        Item it;
        Mod mods[24];
        item_make_unique(&it, &r, 60, u, false, unique_def(u)->cls == ANY_CLASS ? 0 : unique_def(u)->cls);
        check_item_shape(&it, 60);
        CHECK(it.power == u && it.slot == unique_def(u)->slot);
        CHECK(item_mods(&it, unique_def(u)->cls == ANY_CLASS ? 0 : unique_def(u)->cls, mods, 24) >= 5);
    }
    for (u = 1; u <= aspect_count; u++) {
        char buf[96];
        aspect_text(buf, sizeof buf, aspect_def(u), 500);
        CHECK(strchr(buf, '#') == NULL && strlen(buf) > 5);
    }
}

/* ------------------------------------------------------------- crafting */

static void test_temper_and_masterwork(void)
{
    Rng r;
    Item it;
    int k, crits = 0, i;
    double before;
    rng_seed(&r, 9);
    item_roll_slot(&it, &r, 40, 0, RAR_RARE, SLOT_WEAPON, CLASS_BARBARIAN);
    CHECK(item_temper(&it, &r, TR_WEAPONRY) && count_flag(&it, AFX_TEMPERED) == 1);
    CHECK(item_temper(&it, &r, TR_WEAPONRY) && count_flag(&it, AFX_TEMPERED) == 1);   /* same recipe: reroll */
    CHECK(item_temper(&it, &r, TR_FINESSE) && count_flag(&it, AFX_TEMPERED) == 2);
    CHECK(!item_temper(&it, &r, TR_ELEMENTS));               /* both temper slots are taken */
    CHECK(it.temper_left == TEMPER_CHARGES - 3);
    CHECK(!temper_allowed(&it, TR_ENDURANCE));               /* defensive recipes don't fit weapons */
    before = item_affix_value(&it, 1);
    for (k = 0; k < MW_MAX; k++)
        CHECK(item_masterwork(&it, &r));
    CHECK(!item_masterwork(&it, &r) && it.mw == MW_MAX);
    for (i = 0; i < it.naff; i++)
        crits += it.aff[i].mwcrit;
    CHECK(crits == 3);                                     /* ranks 4, 8 and 12 */
    CHECK(item_affix_value(&it, 1) >= before * 1.6 - 1e-9);
    CHECK(item_mw_mult(&it) > 1.59 && item_mw_mult(&it) < 1.61);
}

static void test_enchant_imprint_gems(void)
{
    Rng r;
    Affix opt[2];
    int i, target = -1;
    rng_seed(&r, 3);
    prog_new(&P, 1, CLASS_SORCERER);
    item_roll_slot(&P.equip[SLOT_HELM], &r, 30, 0, RAR_RARE, SLOT_HELM, P.cls);
    for (i = 0; i < P.equip[SLOT_HELM].naff; i++)
        if (!(P.equip[SLOT_HELM].aff[i].flags & AFX_IMPLICIT)) { target = i; break; }
    P.gold = 1e9; P.iron = 100; P.souls = 100;
    CHECK(prog_enchant(&P, SLOT_HELM, target, opt, &r) == CRAFT_OK);
    CHECK(opt[0].type != opt[1].type);
    item_enchant_apply(&P.equip[SLOT_HELM], target, &opt[1]);
    CHECK(P.equip[SLOT_HELM].enchant == target && P.equip[SLOT_HELM].aff[target].type == opt[1].type);
    CHECK(prog_enchant(&P, SLOT_HELM, target == 0 ? 1 : 0, opt, &r) == CRAFT_INVALID);  /* one affix per item */
    /* Imprint: an aspect from the codex turns a rare into a legendary. */
    codex_learn(&P, 9, 700);                     /* aspect of the warden: defensive, fits helms */
    CHECK(prog_imprint(&P, SLOT_HELM, 9) == CRAFT_OK);
    CHECK(P.equip[SLOT_HELM].rarity == RAR_LEGEND && P.equip[SLOT_HELM].power == 9);
    CHECK(P.equip[SLOT_HELM].power_roll == 700);
    CHECK(prog_imprint(&P, SLOT_HELM, 25) == CRAFT_INVALID);          /* unknown aspect */
    /* Gems: socket, unsocket, combine. */
    P.gems[GEM_RUBY][0] = 3;
    CHECK(prog_add_socket(&P, SLOT_HELM) == CRAFT_OK || P.equip[SLOT_HELM].sockets == 1);
    CHECK(prog_socket_gem(&P, SLOT_HELM, 0, gem_id(GEM_RUBY, 0)) && P.gems[GEM_RUBY][0] == 2);
    CHECK(prog_unsocket(&P, SLOT_HELM, 0) && P.gems[GEM_RUBY][0] == 3);
    CHECK(prog_craft_gem(&P, GEM_RUBY, 0) == CRAFT_OK && P.gems[GEM_RUBY][0] == 0 && P.gems[GEM_RUBY][1] == 1);
    P.gold = 0;
    CHECK(prog_masterwork(&P, SLOT_HELM, &r) == CRAFT_NO_GOLD);
}

/* -------------------------------------------------------------- skills */

static void test_skill_tree(void)
{
    int cls, k;
    for (cls = 0; cls < CLASS_COUNT; cls++) {
        prog_new(&P, 7, cls);
        CHECK(P.skill_rank[class_defs[cls].preset[0].bar[0]] == 1);
        CHECK(!skill_can_rank(&P, class_defs[cls].preset[0].bar[1]));      /* no points */
        P.skill_points = 1;
        CHECK(!skill_rank_up(&P, 9));                                     /* ultimate gated */
        P.skill_points = 100;
        CHECK(!key_passive_set(&P, 1));                                   /* key passive gated */
        skill_refund_all(&P);
        CHECK(P.skill_points == 101 && skill_points_spent(&P) == 0);
        for (k = 0; k < PRESETS; k++) {
            skill_refund_all(&P);
            P.skill_points = 59;
            build_apply_preset(&P, k);
            CHECK(P.skill_points == 0 && skill_points_spent(&P) == 59);
            CHECK(P.key_passive == class_defs[cls].preset[k].key);
            CHECK(P.bar[5] != NO_SKILL && skill_on_bar(&P, class_defs[cls].preset[k].bar[1]));
        }
    }
    prog_new(&P, 7, CLASS_ROGUE);
    P.skill_points = 50;
    build_apply_preset(&P, 0);
    CHECK(key_passive_set(&P, 2) && P.key_passive == 2);                  /* switching keys is free */
    CHECK(skill_set_upgrade(&P, 2, 1) && skill_set_upgrade(&P, 2, 2) && P.skill_upg[2] == 2);
}

static void test_build_mods(void)
{
    BuildRT b;
    Stats st;
    build_clear(&b);
    build_add_mod(&b, MOD_X_ALL, 0, 20);
    build_add_mod(&b, MOD_X_ALL, 0, 50);
    CHECK(fabs(b.x_all - 1.8) < 1e-9);                                   /* [x] multiplies */
    build_add_mod(&b, MOD_ADD_DMG, 0, 20);
    build_add_mod(&b, MOD_ADD_DMG, 0, 50);
    CHECK(fabs(b.add[ADD_ALL] - 70) < 1e-9);                             /* + adds */
    build_add_mod(&b, MOD_DR, 0, 50);
    build_add_mod(&b, MOD_DR, 0, 50);
    CHECK(fabs(b.dr - 75) < 1e-9);                                       /* DR stacks multiplicatively */
    prog_new(&P, 3, CLASS_NECRO);
    P.skill_points = 59;
    build_apply_preset(&P, 1);                                           /* summoner */
    stats_compute(&st, &P);
    CHECK(st.b.key == KP_COMMANDER && st.b.x_minion >= 1.6 - 1e-9);
    CHECK(st.b.skill[7].count >= 6);                                     /* 4 + enhancement + commander */
    CHECK(stats_dps(&st, &P) > 0 && st.max_hp > 0);
}

/* ------------------------------------------------------------- paragon */

static void test_paragon(void)
{
    int b;
    prog_new(&P, 3, CLASS_DRUID);
    CHECK(paragon_points_for(49, 0) == 0 && paragon_points_for(60, 0) == 44 && paragon_points_for(60, 10) == 54);
    CHECK(paragon_node(P.cls, 0, BOARD_N / 2, BOARD_N - 1)->type == PN_START);
    CHECK(paragon_node(P.cls, 0, BOARD_N / 2, BOARD_N / 2)->type == PN_GLYPH);
    CHECK(paragon_node(P.cls, 2, BOARD_N / 2, 4)->type == PN_LEGEND);
    P.level = 60;
    CHECK(!paragon_buy(&P, 0, BOARD_N / 2, BOARD_N / 2));                 /* not connected */
    CHECK(paragon_buy(&P, 0, BOARD_N / 2, BOARD_N - 1));                 /* the start node */
    CHECK(!paragon_board_open(&P, 1));
    P.paragon_level = 400;
    paragon_auto(&P);
    CHECK(prog_paragon_available(&P) == 0 || paragon_spent(&P) > 300);
    for (b = 0; b < PARAGON_BOARDS; b++)
        CHECK(b > 1 || (paragon_owned(&P, b, BOARD_N / 2, BOARD_N / 2) && P.glyph[b] != 0));
    CHECK(paragon_board_open(&P, 1) && glyph_stat_in_radius(&P, 0) >= 25);
    paragon_refund(&P);
    CHECK(paragon_spent(&P) == 0 && P.glyph[0] == 0);
}

/* --------------------------------------------------------------- damage */

static void test_damage_pipeline(void)
{
    static World w;
    Monster *m;
    Hit h = { 0 };
    prog_new(&P, 8, CLASS_SORCERER);
    world_init_floor(&w, &P, 5);
    m = &w.mon[0];
    m->alive = 1;
    m->hp = m->max_hp = 1e12;
    w.st.crit = 0;
    w.st.op_chance = 0;
    w.st.vuln_dmg = 0.5;
    h.base = 100;
    h.element = EL_FIRE;
    h.skill = NO_SKILL;
    deal_damage(&w, &P, 0, &h);
    CHECK(!w.last_big.is_vuln && !w.last_big.is_crit && w.last_big.vuln == 1.0);
    h.flags = RF_VULN;
    deal_damage(&w, &P, 0, &h);                 /* this hit makes it vulnerable... */
    h.flags = 0;
    w.last_big.total = 0;
    deal_damage(&w, &P, 0, &h);                 /* ...so this one gets x(1.2 + 0.5) */
    CHECK(m->vuln > 0 && w.last_big.is_vuln && fabs(w.last_big.vuln - 1.7) < 1e-9);
    w.st.crit = 1.0;
    w.st.crit_dmg = 1.0;
    w.last_big.total = 0;
    deal_damage(&w, &P, 0, &h);
    CHECK(w.last_big.is_crit && fabs(w.last_big.crit - 2.5 * w.st.b.x_crit) < 1e-9);
    h.dot = true;
    w.last_big.total = 0;
    deal_damage(&w, &P, 0, &h);
    CHECK(w.last_big.total == 0);               /* DoTs never crit and are not logged */
    CHECK(w.hs.hits >= 4 && w.hs.crits >= 1 && w.hs.vulns >= 2);
}

/* ------------------------------------------------------------- progress */

static void test_progression(void)
{
    prog_new(&P, 1, CLASS_BARBARIAN);
    CHECK(P.level == 1 && P.skill_points == 0);
    prog_add_xp(&P, 1e12);
    CHECK(P.level == LEVEL_CAP && P.paragon_level > 0 && P.skill_points == 0);   /* auto spent */
    CHECK(skill_points_spent(&P) == LEVEL_CAP);
    CHECK(prog_paragon_available(&P) == 0 || paragon_spent(&P) >= 300);         /* auto paragon */
    CHECK(paragon_xp(10) > paragon_xp(0));
    P.gold = 1e30;
    CHECK(prog_switch_preset(&P, 2) && P.preset == 2 && skill_points_spent(&P) == LEVEL_CAP);
}

static void test_loot_and_salvage(void)
{
    Rng r;
    Item it;
    double gold;
    int i;
    prog_new(&P, 2, CLASS_ROGUE);
    rng_seed(&r, 4);
    item_roll_slot(&it, &r, 30, 0, RAR_LEGEND, SLOT_GLOVES, P.cls);
    CHECK(prog_learn_aspect(&P, &it) && codex_known(&P, it.power));
    item_roll_slot(&it, &r, 30, 0, RAR_RARE, SLOT_RING1, P.cls);
    CHECK(prog_handle_loot(&P, &it, &gold) == LOOT_EQUIPPED);
    item_roll_slot(&it, &r, 30, 0, RAR_RARE, SLOT_RING1, P.cls);
    prog_handle_loot(&P, &it, &gold);
    CHECK(P.equip[SLOT_RING1].used && P.equip[SLOT_RING2].used);          /* both ring slots fill */
    for (i = 0; i < BAG_SIZE; i++)
        item_roll_slot(&P.bag[i], &r, 30, 0, RAR_LEGEND, SLOT_PANTS, P.cls);
    CHECK(prog_salvage_all_unlocked(&P, &gold) == BAG_SIZE && P.souls >= BAG_SIZE && P.iron > 0 && gold > 0);
}

static void test_offline(void)
{
    OfflineReport rep;
    prog_new(&P, 5, CLASS_DRUID);
    P.save_time = 1000;
    P.kpm = 30;
    CHECK(prog_offline(&P, 1000 + 3600, &rep) && rep.kills > 0 && P.gold > 0);
    CHECK(!prog_offline(&P, 500, &rep));                       /* clock went backwards */
}

static void test_rebirth(void)
{
    prog_new(&P, 6, CLASS_SPIRITBORN);
    P.best_floor = P.floor = 30;
    codex_learn(&P, 3, 900);
    snprintf(P.look.name, sizeof P.look.name, "TESTER");
    CHECK(prog_rebirth(&P, CLASS_NECRO) > 0);
    CHECK(P.cls == CLASS_NECRO && P.rebirths == 1 && codex_known(&P, 3) && !strcmp(P.look.name, "TESTER"));
}

/* ----------------------------------------------------------------- saves */

static void test_save(void)
{
    static uint8_t buf[SAVE_MAX_BYTES], buf2[SAVE_MAX_BYTES];
    Rng r;
    size_t n, i;
    int k;
    prog_new(&P, 11, CLASS_SORCERER);
    rng_seed(&r, 12);
    for (k = 0; k < BAG_SIZE; k++)
        item_roll(&P.bag[k], &r, 60, 10, RAR_RARE, P.cls);
    item_temper(&P.bag[0], &r, TR_PROFITEER);
    item_masterwork(&P.bag[0], &r);
    prog_add_xp(&P, 1e12);
    codex_learn(&P, 2, 400);
    P.gems[GEM_SKULL][3] = 7;
    P.gold = 1.5e30;
    P.look.hair = 4;
    P.lang = LANG_KO;
    n = save_serialize(&P, buf, sizeof buf);
    CHECK(n > 1000 && n < SAVE_MAX_BYTES);
    CHECK(save_deserialize(&Q, buf, n) == SAVE_OK);
    CHECK(save_serialize(&Q, buf2, sizeof buf2) == n && !memcmp(buf, buf2, n));   /* lossless */
    CHECK(Q.gold == P.gold && Q.look.hair == 4 && Q.gems[GEM_SKULL][3] == 7 && Q.codex[2] == 401);
    CHECK(Q.lang == LANG_KO);
    CHECK(Q.paragon_level == P.paragon_level && !memcmp(Q.para, P.para, sizeof P.para));
    for (i = 0; i < n; i += 13) { /* any flipped byte is caught */
        buf[i] ^= 0x41;
        CHECK(save_deserialize(&Q, buf, n) == SAVE_CORRUPT);
        buf[i] ^= 0x41;
    }
    CHECK(save_deserialize(&Q, buf, n - 1) == SAVE_CORRUPT);
    CHECK(save_write(&P, "build/test.sav") == SAVE_OK);
    CHECK(save_load(&Q, "build/test.sav") == SAVE_OK && Q.gold == P.gold);
    remove("build/test.sav");
    CHECK(save_load(&Q, "build/does_not_exist.sav") == SAVE_MISSING);
}

/* Real saves of the earlier versions (v1 and two v3 player saves). */
static void test_migration(void)
{
    CHECK(save_load(&Q, "tests/fixtures/v1_save.sav") == SAVE_OK);
    CHECK(Q.cls == CLASS_BARBARIAN && Q.level >= 1 && Q.equip[SLOT_WEAPON].used);
    CHECK(skill_points_spent(&Q) + Q.skill_points == Q.level);
    CHECK(save_load(&Q, "tests/fixtures/v3_save.sav") == SAVE_OK);
    CHECK(Q.cls == CLASS_SORCERER && Q.level >= 10 && Q.floor >= 6);
    CHECK(Q.skill_points == 0 && Q.bar[0] != NO_SKILL);                  /* rebuilt by the planner */
    CHECK(Q.look.name[0] != 0 && Q.equip[SLOT_WEAPON].used);
    printf("  v3 save: %s level %d floor %d gold %.0f\n", class_defs[Q.cls].name, Q.level, Q.floor, Q.gold);
    CHECK(save_load(&Q, "tests/fixtures/v3_save_b.sav") == SAVE_OK);
    CHECK(Q.lang < LANG_COUNT);
}

static void test_story(void)
{
    char name[32];
    CHECK(story_act(1) == 0 && story_act(10) == 0 && story_act(11) == 1 && story_act(50) == 4 && story_act(51) == -1);
    CHECK(story_is_act_start(21) && !story_is_act_start(22) && story_is_act_boss(40) && !story_is_act_boss(60));
    story_boss_name(name, sizeof name, 10);
    CHECK(!strcmp(name, "MORDRAIN THE BONE WARDEN"));
}

/* ------------------------------------------------------------- languages */

static uint32_t utf8_cp(const char **s)
{
    const uint8_t *p = (const uint8_t *)*s;
    uint32_t c = *p++;
    int extra = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
    if (extra)
        c &= 0x3F >> extra;
    while (extra-- > 0 && (*p & 0xC0) == 0x80)
        c = (c << 6) | (*p++ & 0x3F);
    *s = (const char *)p;
    return c;
}

/* The printf conversions of a string, e.g. "d%f" for "LV %d %% %.0f".
 * Same rule as gen_i18n.py: a '%' in plain text ("+15% DAMAGE") is not one. */
static void spec_signature(const char *s, char *out, size_t cap)
{
    size_t n = 0;
    for (; *s && n + 2 < cap; s++) {
        const char *q = s + 1;
        if (*s != '%')
            continue;
        while (*q && strchr("0123456789.", *q))
            q++;
        if (*q && strchr("%dsfuxc", *q) && (*q != '%' || q == s + 1)) {
            out[n++] = *q;
            s = q;
        }
    }
    out[n] = 0;
}

/* Every translation keeps its printf conversions and every character it
 * uses has a glyph in that language's font. */
static void check_translations(int lang)
{
    int i, missing = 0, bad_fmt = 0;
    lang_set(lang);
    for (i = 0; i < i18n_count; i++) {
        const char *tr = i18n_strings[i].tr[lang - 1], *p = tr;
        char a[32], b[32];
        if (!tr)
            continue;
        spec_signature(i18n_strings[i].en, a, sizeof a);
        spec_signature(tr, b, sizeof b);
        bad_fmt += strcmp(a, b) != 0;
        while (*p) {
            uint32_t cp = utf8_cp(&p);
            missing += cp >= 0x80 && !cjk_glyph(cp);
        }
    }
    CHECK(bad_fmt == 0 && missing == 0);
}

static void check_story_fits(int lang)
{
    int act, i, wide = 0;
    lang_set(lang);
    for (act = 0; act < ACT_COUNT; act++) {
        for (i = 0; i < STORY_LINES && act_defs[act].intro[i]; i++)
            wide += font_text_width(act_defs[act].intro[i], 1) > SCREEN_W - 8;
        for (i = 0; i < STORY_LINES && act_defs[act].victory[i]; i++)
            wide += font_text_width(act_defs[act].victory[i], 1) > SCREEN_W - 8;
        wide += font_text_width(act_defs[act].title, 2) > SCREEN_W - 8;
    }
    for (i = 0; i < STORY_LINES && epilogue[i]; i++)
        wide += font_text_width(epilogue[i], 1) > SCREEN_W - 8;
    CHECK(wide == 0);
}

static void test_languages(void)
{
    char buf[64];
    int lang;
    lang_set(LANG_EN);
    CHECK(!strcmp(T("SAVED"), "SAVED") && !strcmp(T("NO SUCH STRING"), "NO SUCH STRING"));
    lang_set(LANG_ZHS);
    CHECK(!strcmp(T("SAVED"), "\xe5\xb7\xb2\xe4\xbf\x9d\xe5\xad\x98"));          /* 已保存 */
    CHECK(!strcmp(T("NO SUCH STRING"), "NO SUCH STRING"));                      /* falls back to English */
    tjoin(buf, sizeof buf, "ANCESTRAL", "LEGENDARY");
    CHECK(!strchr(buf, ' ') && strlen(buf) > 6);                                /* compact, no space */
    lang_set(LANG_KO);
    tjoin(buf, sizeof buf, "ANCESTRAL", "LEGENDARY");
    CHECK(strchr(buf, ' ') != NULL);
    lang_set(LANG_EN);
    tjoin(buf, sizeof buf, "ANCESTRAL", "LEGENDARY");
    CHECK(!strcmp(buf, "ANCESTRAL LEGENDARY"));
    lang_set(99);
    CHECK(lang_get() < LANG_COUNT);                                             /* out-of-range rejected */
    for (lang = LANG_ZHS; lang < LANG_COUNT; lang++) {
        check_translations(lang);
        check_story_fits(lang);
        CHECK(font_text_width(lang_name(lang), 1) > 0);
    }
    lang_set(LANG_EN);
}

static void test_slot_paths(void)
{
    static Game g;
    char path[300];
    memset(&g, 0, sizeof g);
    g.save_base = "/documents/ndless/AshenDepths.sav.tns";
    game_slot_path(&g, 2, path, sizeof path);
    CHECK(!strcmp(path, "/documents/ndless/AshenDepths2.sav.tns"));
    g.save_base = NULL;
    game_slot_path(&g, 1, path, sizeof path);
    CHECK(path[0] == 0);
}

/* ------------------------------------------------------------ graphics */

static void test_sprites(void)
{
    HeroLook a, b;
    const Sprite *s1, *s2;
    int i;
    prog_new(&P, 1, CLASS_DRUID);
    hero_look_from(&a, &P);
    s1 = spr_hero_look(&a, 0);
    CHECK(s1->w == HERO_W && s1->h == HERO_H);
    b = a;
    b.helm = 4;                                 /* a great helm changes the picture */
    b.helm_mat = RAR_LEGEND + 1;
    s2 = spr_hero_look(&b, 0);
    CHECK(memcmp(s1->px, s2->px, HERO_W * HERO_H * 2) != 0);
    CHECK(spr_hero_look(&a, 0) == s1);          /* cached */
    for (i = 0; i < IC_COUNT; i++)
        CHECK(spr_skill_icon(i, RGB565(200, 100, 50))->w == 16);
}

/* Every monster and the stairs must be reachable from the hero's spawn. */
static void test_dungeon_connectivity(void)
{
    static World w;
    int f, i, bad = 0;
    prog_new(&P, 21, CLASS_BARBARIAN);
    for (f = 1; f <= 200; f++) {
        world_init_floor(&w, &P, f);
        bfs_field(&w, w.ft, px_to_cell(w.h.x), px_to_cell(w.h.y));
        if (w.ft[w.stairs_y][w.stairs_x] > 5000)
            bad++;
        for (i = 0; i < w.nmon; i++)
            if (w.ft[px_to_cell(w.mon[i].y)][px_to_cell(w.mon[i].x)] > 5000)
                bad++;
        CHECK(w.quota > 0 && w.quota <= w.nmon);
    }
    CHECK(bad == 0);
}

/* Every class keeps progressing for hours on autopilot. */
static void test_idle_simulation(void)
{
    int cls;
    for (cls = 0; cls < CLASS_COUNT; cls++) {
        long t;
        int last_best = 0;
        prog_new(&P, 1234, cls);
        memset(&S, 0, sizeof S);
        session_start(&S, &P);
        for (t = 1; t <= 2L * 3600 * TICK_HZ; t++) {
            session_tick(&S, &P);
            if (S.story_pending >= 0)
                session_story_shown(&S, &P);
            if (t % (3600L * TICK_HZ) == 0) {
                printf("  %-11s hour %ld: floor %d (best %d) level %d+%d deaths %d stuck %d\n", class_defs[cls].name,
                       t / (3600L * TICK_HZ), P.floor, P.best_floor, P.level, P.paragon_level, S.deaths,
                       S.stuck_resets);
                CHECK(P.best_floor > last_best);
                last_best = P.best_floor;
            }
        }
        CHECK(P.best_floor >= 45);
        CHECK(S.stuck_resets <= 6);
        CHECK(P.gold == P.gold && S.w.h.hp == S.w.h.hp); /* no NaN */
        CHECK(story_seen(P.story_seen, 0) && story_seen(P.story_seen, 5));
    }
}

int main(void)
{
    CHECK(sprites_init()); /* validates every art grid */
    printf("format\n");        test_format();
    printf("items\n");         test_items();
    printf("uniques\n");       test_uniques();
    printf("temper/mw\n");     test_temper_and_masterwork();
    printf("enchant/gems\n");  test_enchant_imprint_gems();
    printf("skill tree\n");    test_skill_tree();
    printf("build mods\n");    test_build_mods();
    printf("paragon\n");       test_paragon();
    printf("damage\n");        test_damage_pipeline();
    printf("progression\n");   test_progression();
    printf("loot\n");          test_loot_and_salvage();
    printf("offline\n");       test_offline();
    printf("rebirth\n");       test_rebirth();
    printf("save\n");          test_save();
    printf("migration\n");     test_migration();
    printf("story\n");         test_story();
    printf("languages\n");     test_languages();
    printf("slots\n");         test_slot_paths();
    printf("sprites\n");       test_sprites();
    printf("dungeon\n");       test_dungeon_connectivity();
    printf("idle sim\n");      test_idle_simulation();
    printf("\n%d checks, %d failures\n", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
