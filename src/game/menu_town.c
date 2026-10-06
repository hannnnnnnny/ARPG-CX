/*
 * menu_town.c - TOWN page: where the gold goes (Diablo IV services).
 *   Blacksmith: masterwork (12 ranks) and tempering (2 extra affixes)
 *   Occultist:  enchant one affix (pick 1 of 2 or keep) and imprint
 *               aspects from the Codex of Power
 *   Jeweler:    socket gems, add sockets, combine gems
 *   Alchemist:  potion upgrades and elixirs
 *   Gambler:    buy unknown gear by slot
 */
#include "menu_int.h"
#include "aspects.h"
#include "balance.h"
#include "items.h"
#include "stats.h"
#include "../gfx/font.h"
#include "../gfx/sprites.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

static const char *const town_names[TOWN_COUNT] = { "", "BLACKSMITH", "OCCULTIST", "JEWELER", "ALCHEMIST", "GAMBLER" };
static const char *const town_desc[TOWN_COUNT] = { "", "MASTERWORK AND TEMPER YOUR GEAR", "ENCHANT AFFIXES, IMPRINT ASPECTS",
                                                   "GEMS AND SOCKETS", "POTIONS AND ELIXIRS", "BUY UNKNOWN GEAR" };
static const char *const elixir_names[ELIX_COUNT] = { "", "ELIXIR OF FORTITUDE", "ELIXIR OF PRECISION",
                                                      "ELIXIR OF ADVANTAGE", "ELIXIR OF IRON BARBS",
                                                      "ELIXIR OF WISDOM" };
static const char *const elixir_desc[ELIX_COUNT] = { "", "+10% LIFE", "+15% CRIT DAMAGE",
                                                     "+8% ATTACK SPEED", "5% DR, +25% THORNS", "+10% XP" };
static const uint8_t gamble_slots[9] = { SLOT_WEAPON, SLOT_OFFHAND, SLOT_HELM, SLOT_CHEST, SLOT_GLOVES, SLOT_PANTS,
                                         SLOT_BOOTS, SLOT_AMULET, SLOT_RING1 };

static void cost_text(char *out, size_t cap, double gold, double iron, double souls)
{
    char g[16], i[16], s[16];
    fmt_num(g, sizeof g, gold);
    fmt_num(i, sizeof i, iron);
    fmt_num(s, sizeof s, souls);
    if (souls > 0)
        snprintf(out, cap, T("%s GOLD %s IRON %s SOULS"), g, i, s);
    else if (iron > 0)
        snprintf(out, cap, T("%s GOLD %s IRON"), g, i);
    else
        snprintf(out, cap, T("%s GOLD"), g);
}

static Item *cur_item(Game *g)
{
    return &g->p.equip[CLAMP(g->town_sel, 0, SLOT_COUNT - 1)];
}

/* ------------------------------------------------------------ blacksmith */

static int recipe_of(const Item *it, int opt)
{
    int t, ids[TR_COUNT], n = 0;
    for (t = 0; t < TR_COUNT; t++)
        if (temper_allowed(it, (TemperRecipe)t))
            ids[n++] = t;
    return n ? ids[((opt % n) + n) % n] : 0;
}

static void smith_tick(Game *g, Input *in)
{
    Item *it = cur_item(g);
    if (in_ok(in))
        menu_craft_toast(g, prog_masterwork(&g->p, (Slot)g->town_sel, &g->craft_rng),
                         it->mw % 4 == 3 ? "MASTERWORK! A CRITICAL UPGRADE" : "MASTERWORKED +5%");
    else if (in_alt(in))
        menu_craft_toast(g, prog_temper(&g->p, (Slot)g->town_sel, (TemperRecipe)recipe_of(it, g->town_opt),
                                        &g->craft_rng), "TEMPERED: A NEW AFFIX");
}

static void smith_render(Game *g, int x, int y)
{
    Item *it = cur_item(g);
    char buf[128];
    if (!it->used)
        return;
    snprintf(buf, sizeof buf, T("MASTERWORK %d/%d (ENTER)"), it->mw, MW_MAX);
    font_draw(x, y, buf, C_SEL, 1);
    if (it->mw < MW_MAX) {
        cost_text(buf, sizeof buf, masterwork_gold(it), masterwork_iron(it), masterwork_souls(it));
        font_draw(x, y + 10, buf, C_DIM, 1);
    }
    font_draw_wrapped(x, y + 20, "+5% TO ALL AFFIXES PER RANK. RANKS 4, 8, 12 GIVE ONE AFFIX +25%.", 160, C_DIM);
    snprintf(buf, sizeof buf, T("TEMPER: %s (DEL)"), T(temper_name((TemperRecipe)recipe_of(it, g->town_opt))));
    font_draw(x, y + 60, buf, C_TEMP, 1);
    snprintf(buf, sizeof buf, T("CHARGES LEFT %d"), it->temper_left);
    font_draw(x, y + 70, buf, C_DIM, 1);
    font_draw(x, y + 80, "LEFT/RIGHT: RECIPE", C_DIM, 1);
    cost_text(buf, sizeof buf, temper_gold(it), 1, 0);
    font_draw(x, y + 90, buf, C_DIM, 1);
    font_draw_wrapped(x, y + 102, "UP TO 2 TEMPERED AFFIXES. TEMPERING A RECIPE AGAIN REROLLS ITS AFFIX.", 160, C_DIM);
}

/* ------------------------------------------------------------- occultist */

static int enchantable(const Item *it, int opt)
{
    int i, n = 0, count = 0;
    for (i = 0; i < it->naff; i++)
        count += !(it->aff[i].flags & (AFX_IMPLICIT | AFX_TEMPERED));
    if (count == 0)
        return -1;
    opt = ((opt % count) + count) % count;
    for (i = 0; i < it->naff; i++)
        if (!(it->aff[i].flags & (AFX_IMPLICIT | AFX_TEMPERED)) && n++ == opt)
            return i;
    return -1;
}

static int imprintable(const Game *g, const Item *it, int opt)
{
    int ids[ASPECT_MAX], n = 0, a;
    for (a = 1; a <= aspect_count && a < ASPECT_MAX; a++)
        if (codex_known(&g->p, a) && item_can_imprint(it, a, g->p.cls))
            ids[n++] = a;
    return n ? ids[((opt % n) + n) % n] : 0;
}

static void enchant_dialog_tick(Game *g, Input *in)
{
    EnchantDialog *e = &g->ench;
    if (in_up(in))   e->pick = (e->pick + 2) % 3;
    if (in_down(in)) e->pick = (e->pick + 1) % 3;
    if (in_ok(in)) {
        if (e->pick < 2) {
            item_enchant_apply(&g->p.equip[e->slot], e->affix, &e->opt[e->pick]);
            game_profile_changed(g);
            game_toast(g, "ENCHANTED", C_GOOD);
        }
        e->open = false;
    } else if (in_back(in)) {
        e->open = false;
    }
    input_block_held(in);
}

static void occult_tick(Game *g, Input *in)
{
    Item *it = cur_item(g);
    if (in_alt(in)) {
        g->town_mode = !g->town_mode;
        g->town_opt = 0;
        return;
    }
    if (!in_ok(in) || !it->used)
        return;
    if (g->town_mode) {
        int a = imprintable(g, it, g->town_opt);
        menu_craft_toast(g, a ? prog_imprint(&g->p, (Slot)g->town_sel, a) : CRAFT_INVALID, "ASPECT IMPRINTED");
    } else {
        int i = enchantable(it, g->town_opt);
        CraftResult r = i >= 0 ? prog_enchant(&g->p, (Slot)g->town_sel, i, g->ench.opt, &g->craft_rng)
                               : CRAFT_INVALID;
        if (r == CRAFT_OK) {
            g->ench.open = true;
            g->ench.slot = g->town_sel;
            g->ench.affix = i;
            g->ench.pick = 0;
        } else {
            menu_craft_toast(g, r, "");
        }
    }
}

static void occult_render(Game *g, int x, int y)
{
    Item *it = cur_item(g);
    char buf[96];
    int known = 0, a;
    for (a = 1; a <= aspect_count && a < ASPECT_MAX; a++)
        known += codex_known(&g->p, a);
    snprintf(buf, sizeof buf, T("MODE: %s (DEL)"), T(g->town_mode ? "IMPRINT" : "ENCHANT"));
    font_draw(x, y, buf, C_SEL, 1);
    snprintf(buf, sizeof buf, T("CODEX OF POWER: %d/%d ASPECTS"), known, aspect_count);
    font_draw(x, y + 10, buf, C_DIM, 1);
    y += 10;
    if (!it->used)
        return;
    if (g->town_mode) {
        int id = imprintable(g, it, g->town_opt);
        if (!id) {
            font_draw_wrapped(x, y + 12, "NO MATCHING ASPECT IN THE CODEX FOR THIS ITEM (RARE OR LEGENDARY ONLY).",
                              160, C_DIM);
            return;
        }
        font_draw(x, y + 12, aspect_def(id)->name, C_LEG, 1);
        aspect_text(buf, sizeof buf, aspect_def(id), codex_roll(&g->p, id));
        y = font_draw_wrapped(x, y + 22, buf, 160, C_TEXT);
        cost_text(buf, sizeof buf, imprint_gold(it), 0, 2);
        font_draw(x, y + 2, buf, C_DIM, 1);
        font_draw(x, y + 12, "LEFT/RIGHT: ASPECT  ENTER: IMPRINT", C_DIM, 1);
    } else {
        int i = enchantable(it, g->town_opt);
        if (i < 0)
            return;
        affix_text(buf, sizeof buf, it, i, g->p.cls);
        y = font_draw_wrapped(x, y + 12, buf, 160, C_AFFIX);
        cost_text(buf, sizeof buf, enchant_gold(it), 0, it->rarity >= RAR_LEGEND ? 1 : 0);
        font_draw(x, y + 2, buf, C_DIM, 1);
        font_draw_wrapped(x, y + 12, "LEFT/RIGHT: AFFIX  ENTER: REROLL. ONLY ONE AFFIX PER ITEM CAN BE ENCHANTED.",
                          160, C_DIM);
    }
}

static void enchant_dialog(const Game *g)
{
    const Item *it = &g->p.equip[g->ench.slot];
    Item tmp = *it;
    char buf[128];
    int r;
    gfx_fill_rect(16, 66, 288, 92, RGB565(24, 16, 20));
    gfx_rect(16, 66, 288, 92, C_SEL);
    font_draw(24, 72, "CHOOSE THE NEW AFFIX", C_DIM, 1);
    for (r = 0; r < 3; r++) {
        int y = 88 + r * 20;
        if (g->ench.pick == r)
            gfx_fill_rect(20, y - 4, 280, 18, C_HIL);
        if (r < 2)
            tmp.aff[g->ench.affix] = g->ench.opt[r];
        else
            tmp.aff[g->ench.affix] = it->aff[g->ench.affix];
        affix_text(buf, sizeof buf, &tmp, g->ench.affix, g->p.cls);
        font_draw(26, y, r < 2 ? buf : "KEEP THE CURRENT AFFIX", g->ench.pick == r ? C_SEL : C_AFFIX, 1);
    }
}

/* --------------------------------------------------------------- jeweler */

static int best_gem_of(const Profile *p, int kind)
{
    int t;
    for (t = GEM_TIERS - 1; t >= 0; t--)
        if (p->gems[kind][t] > 0)
            return gem_id(kind, t);
    return -1;
}

static void jewel_tick(Game *g, Input *in)
{
    Item *it = cur_item(g);
    int kind = ((g->town_opt % GEM_KINDS) + GEM_KINDS) % GEM_KINDS, i, gem = best_gem_of(&g->p, kind);
    if (in_ok(in) && it->used) {
        int socket = -1;
        for (i = it->sockets - 1; i >= 0; i--)
            if (it->gem[i] == GEM_NONE || socket < 0)
                socket = i;
        if (gem >= 0 && socket >= 0 && prog_socket_gem(&g->p, (Slot)g->town_sel, socket, gem)) {
            game_profile_changed(g);
            game_toast(g, "GEM SOCKETED", C_GOOD);
        } else {
            game_toast(g, it->sockets ? "NO GEM OF THAT KIND" : "NO SOCKET (DEL ADDS ONE)", C_BAD);
        }
    } else if (in_alt(in)) {
        menu_craft_toast(g, prog_add_socket(&g->p, (Slot)g->town_sel), "SOCKET ADDED");
    } else if (in_lock(in)) {
        int t, n = 0;
        for (t = 0; t < GEM_TIERS - 1; t++)
            while (prog_craft_gem(&g->p, kind, t) == CRAFT_OK)
                n++;
        game_toast(g, n ? "GEMS COMBINED" : "NEED 3 GEMS OF A TIER (AND GOLD)", n ? C_GOOD : C_BAD);
    }
}

static void jewel_render(Game *g, int x, int y)
{
    int kind = ((g->town_opt % GEM_KINDS) + GEM_KINDS) % GEM_KINDS, t, k;
    char buf[128];
    snprintf(buf, sizeof buf, T("GEM: %s  (LEFT/RIGHT)"), gem_name(gem_id(kind, 2)));
    font_draw(x, y, buf, gem_color(kind), 1);
    for (k = 0; k < GEM_KINDS; k++) {        /* the pouch: one row per kind, counts per tier */
        gfx_fill_rect(x, y + 14 + k * 10, 6, 6, gem_color(k));
        for (t = 0; t < GEM_TIERS; t++) {
            snprintf(buf, sizeof buf, "%d", g->p.gems[k][t]);
            font_draw(x + 12 + t * 26, y + 13 + k * 10, buf, g->p.gems[k][t] ? C_TEXT : C_DIM, 1);
        }
    }
    gem_effect_text(buf, sizeof buf, gem_id(kind, GEM_TIERS - 1), (Slot)g->town_sel);
    font_draw_wrapped(x, y + 88, buf, 160, C_DIM);
    cost_text(buf, sizeof buf, socket_gold(cur_item(g)), 5, 1);
    font_draw(x, y + 108, "ADD SOCKET (DEL):", C_DIM, 1);
    font_draw(x, y + 118, buf, C_DIM, 1);
    font_draw(x, y + 130, "CTRL: COMBINE 3 -> 1 HIGHER", C_DIM, 1);
}

/* ------------------------------------------------- alchemist and gambler */

static void alchemy_tick(Game *g, Input *in)
{
    if (!in_ok(in))
        return;
    if (g->town_sel == 0)
        menu_craft_toast(g, prog_upgrade_potion(&g->p), "POTIONS UPGRADED");
    else
        menu_craft_toast(g, prog_buy_elixir(&g->p, (ElixirKind)g->town_sel), "ELIXIR ACTIVE FOR 30 MINUTES");
}

static void gamble_tick(Game *g, Input *in)
{
    Item it;
    double gold;
    char name[64], buf[112];
    if (!in_ok(in))
        return;
    if (prog_gamble(&g->p, (Slot)gamble_slots[g->town_sel % 9], &g->craft_rng, &it) != CRAFT_OK) {
        game_toast(g, "NOT ENOUGH GOLD", C_BAD);
        return;
    }
    g->gamble_last = it;
    item_name(name, sizeof name, &it);
    snprintf(buf, sizeof buf, "%s: %s", prog_handle_loot(&g->p, &it, &gold) == LOOT_EQUIPPED ? T("EQUIPPED") : T("GOT"), name);
    game_profile_changed(g);
    game_toast(g, buf, rarity_color((Rarity)it.rarity));
}

static int town_rows(const Game *g)
{
    switch (g->town) {
    case TOWN_ALCHEMY: return ELIX_COUNT;
    case TOWN_GAMBLE:  return 9;
    case TOWN_LIST:    return TOWN_COUNT - 1;
    default:           return SLOT_COUNT;
    }
}

void town_tick(Game *g, Input *in)
{
    if (g->ench.open) {
        enchant_dialog_tick(g, in);
        return;
    }
    if (g->town == TOWN_LIST) {
        menu_move(in, &g->sel[PG_TOWN], town_rows(g));
        if (in_ok(in)) {
            g->town = (TownScreen)(g->sel[PG_TOWN] + 1);
            g->town_sel = g->town_opt = 0;
            input_block_held(in);
        }
        return;
    }
    if (in_back(in)) {
        g->town = TOWN_LIST;
        input_block_held(in);
        return;
    }
    if (menu_move(in, &g->town_sel, town_rows(g)))
        g->town_opt = 0;
    if (in_left(in))  g->town_opt--;
    if (in_right(in)) g->town_opt++;
    switch (g->town) {
    case TOWN_SMITH:   smith_tick(g, in); break;
    case TOWN_OCCULT:  occult_tick(g, in); break;
    case TOWN_JEWEL:   jewel_tick(g, in); break;
    case TOWN_ALCHEMY: alchemy_tick(g, in); break;
    default:           gamble_tick(g, in); break;
    }
}

/* -------------------------------------------------------------- drawing */

static void slot_rows(const Game *g)
{
    int i;
    for (i = 0; i < SLOT_COUNT; i++) {
        const Item *it = &g->p.equip[i];
        int y = 20 + i * 19;
        char name[64];
        if (i == g->town_sel)
            gfx_fill_rect(2, y - 2, 140, 18, C_HIL);
        font_draw(6, y, slot_name((Slot)i), C_DIM, 1);
        if (!it->used)
            continue;
        item_name(name, sizeof name, it);
        font_draw_fit(6, y + 8, name, 134, rarity_color((Rarity)it->rarity));
        if (it->mw) {
            snprintf(name, sizeof name, "+%d", it->mw);
            font_draw(138 - font_text_width(name, 1), y, name, C_TEMP, 1);
        }
    }
}

static void list_render(const Game *g)
{
    int i;
    for (i = 1; i < TOWN_COUNT; i++) {
        int y = 22 + (i - 1) * 38;
        bool on = g->sel[PG_TOWN] == i - 1;
        menu_row_highlight(y, 34, on);
        font_draw(12, y + 2, town_names[i], on ? C_SEL : C_TEXT, 2);
        font_draw(14, y + 20, town_desc[i], C_DIM, 1);
    }
}

static void shop_rows(const Game *g)
{
    int i, n = town_rows(g);
    char buf[128], c[16];
    for (i = 0; i < n; i++) {
        int y = 32 + i * 16;
        menu_row_highlight(y, 15, i == g->town_sel);
        if (g->town == TOWN_GAMBLE) {
            font_draw(8, y, slot_name((Slot)gamble_slots[i]), C_TEXT, 1);
            fmt_num(c, sizeof c, gamble_gold(g->p.best_floor));
        } else if (i == 0) {
            snprintf(buf, sizeof buf, T("HEALING POTION LV %d/%d"), g->p.potion_lvl, POTION_MAX_LVL);
            font_draw(8, y, buf, C_TEXT, 1);
            fmt_num(c, sizeof c, potion_upgrade_gold(g->p.potion_lvl));
        } else {
            font_draw(8, y, elixir_names[i], g->p.elixir == i && g->p.elixir_secs ? C_GOOD : C_TEXT, 1);
            font_draw(164, y, elixir_desc[i], C_DIM, 1);
            fmt_num(c, sizeof c, elixir_gold(g->p.best_floor));
        }
        font_draw(314 - font_text_width(c, 1), y, c, RGB565(250, 212, 80), 1);
    }
}

void town_render(Game *g)
{
    if (g->town == TOWN_LIST) {
        list_render(g);
        menu_footer(g, "ENTER: VISIT   TAB: PAGE   ESC: BATTLE");
        return;
    }
    font_draw(g->town == TOWN_ALCHEMY || g->town == TOWN_GAMBLE ? 8 : 150, 18, town_names[g->town], C_SEL, 1);
    if (g->town == TOWN_ALCHEMY || g->town == TOWN_GAMBLE) {
        shop_rows(g);
        if (g->town == TOWN_GAMBLE && g->gamble_last.used) {
            char name[64], buf[64];
            item_name(name, sizeof name, &g->gamble_last);
            snprintf(buf, sizeof buf, T("LAST: %s%s (%d)"), g->gamble_last.ancestral ? T("ANCESTRAL ") : "", name,
                     item_power(&g->gamble_last));
            font_draw(8, 172, buf, rarity_color((Rarity)g->gamble_last.rarity), 1);
        }
        menu_footer(g, "ENTER: BUY   ESC: TOWN");
        return;
    }
    slot_rows(g);
    gfx_vline(144, 16, 198, C_EDGE);
    switch (g->town) {
    case TOWN_SMITH:  smith_render(g, 150, 30); break;
    case TOWN_OCCULT: occult_render(g, 150, 30); break;
    default:          jewel_render(g, 150, 30); break;
    }
    menu_footer(g, g->town == TOWN_JEWEL ? "ENTER: SOCKET  DEL: ADD SOCKET  ESC: TOWN"
                 : g->town == TOWN_SMITH ? "ENTER: MASTERWORK  DEL: TEMPER  ESC: TOWN" : "ENTER: APPLY  ESC: TOWN");
    if (g->ench.open)
        enchant_dialog(g);
}
