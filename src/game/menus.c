/*
 * menus.c - the menu frame: tabs, footer, page dispatch, and the shared
 * Diablo IV style item tooltip. Pages live in menu_*.c.
 *
 * TAB cycles pages (LEFT/RIGHT too on pages that don't use them), ESC
 * returns to the battle, which keeps running behind the menu.
 */
#include "menu_int.h"
#include "aspects.h"
#include "items.h"
#include "stats.h"
#include "../gfx/font.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

static int g_card_max = FOOTER_Y - 2;   /* tooltip lines below this are skipped */

static void card_text(int x, int y, const char *s, uint16_t c)
{
    if (y + 7 <= g_card_max)
        font_draw(x, y, s, c, 1);
}

static int card_wrap(int x, int y, const char *s, int w, uint16_t c)
{
    return font_draw_wrapped_clip(x, y, s, w, c, g_card_max);
}

static const char *const page_names[PG_COUNT] = { "HERO", "BAG", "SKILLS", "BOARD", "TOWN", "GOALS", "EMBERS", "OPTION" };

bool menu_move(const Input *in, int *sel, int rows)
{
    if (rows <= 0)
        return false;
    if (in_up(in)) {
        *sel = (*sel + rows - 1) % rows;
        return true;
    }
    if (in_down(in)) {
        *sel = (*sel + 1) % rows;
        return true;
    }
    return false;
}

int menu_scroll(int sel, int rows, int visible)
{
    return CLAMP(sel - visible / 2, 0, MAX(0, rows - visible));
}

void menu_row_highlight(int y, int h, bool on)
{
    game_ui_row(y - 2, h, on);
    if (on)
        gfx_fill_rect(2, y - 2, SCREEN_W - 4, h, C_HIL);
}

void menu_footer(const Game *g, const char *keys)
{
    char buf[112], gold[16], iron[16], souls[16];
    fmt_num(gold, sizeof gold, g->p.gold);
    fmt_num(iron, sizeof iron, g->p.iron);
    fmt_num(souls, sizeof souls, g->p.souls);
    gfx_hline(0, FOOTER_Y, SCREEN_W, C_EDGE);
    snprintf(buf, sizeof buf, T("FLOOR %d  GOLD %s  IRON %s  SOULS %s"), g->p.floor, gold, iron, souls);
    font_draw(4, FOOTER_Y + 4, buf, C_DIM, 1);
    font_draw(4, FOOTER_Y + 14, keys, C_TEXT, 1);
}

void menu_craft_toast(Game *g, CraftResult r, const char *ok)
{
    switch (r) {
    case CRAFT_OK:      game_profile_changed(g); game_toast(g, ok, C_GOOD); break;
    case CRAFT_NO_GOLD: game_toast(g, "NOT ENOUGH GOLD", C_BAD); break;
    case CRAFT_NO_MATS: game_toast(g, "NOT ENOUGH MATERIALS (SALVAGE ITEMS)", C_BAD); break;
    default:            game_toast(g, "NOT POSSIBLE ON THIS ITEM", C_BAD); break;
    }
}

/* ------------------------------------------------------------ item card */

static int card_header(const Game *g, const Item *it, int x, int y, int w, const Item *cmp)
{
    char buf[96], name[64], rar[64];
    uint16_t rc = rarity_color((Rarity)it->rarity);
    tjoin(rar, sizeof rar, it->ancestral && it->rarity != RAR_MYTHIC ? "ANCESTRAL" : "", rarity_name((Rarity)it->rarity));
    tjoin(buf, sizeof buf, rar, slot_name((Slot)it->slot));
    card_text(x, y, buf, rc);
    item_name(name, sizeof name, it);
    y = card_wrap(x, y + 10, name, w, rc);
    snprintf(buf, sizeof buf, T("%d ITEM POWER"), item_power(it));
    card_text(x, y, buf, C_TEXT);
    if (it->mw) {
        snprintf(buf, sizeof buf, T("MW %d/%d"), it->mw, MW_MAX);
        card_text(x + w - font_text_width(buf, 1), y, buf, C_TEMP);
    }
    if (cmp && cmp != it) {
        double r = item_upgrade_ratio(&g->p, it) * 100.0;
        snprintf(buf, sizeof buf, T("%s%d%% POWER"), r >= 0 ? "+" : "", (int)r);
        card_text(x, y + 9, buf, r > 0.5 ? C_GOOD : r < -0.5 ? C_BAD : C_DIM);
        y += 9;
    }
    return y + 10;
}

static int card_affixes(const Game *g, const Item *it, int x, int y, int w)
{
    char buf[128], line[144];
    int i;
    item_main_text(buf, sizeof buf, it);
    if (buf[0]) {
        card_text(x, y, buf, C_TEXT);
        y += 10;
    }
    for (i = 0; i < it->naff; i++) {
        const Affix *a = &it->aff[i];
        uint16_t c = a->flags & AFX_IMPLICIT ? C_DIM : a->flags & AFX_GREATER ? C_GREAT
                   : a->flags & AFX_TEMPERED ? C_TEMP : C_AFFIX;
        affix_text(buf, sizeof buf, it, i, g->p.cls);
        snprintf(line, sizeof line, "%s%s%s%s", a->flags & AFX_GREATER ? "* " : a->flags & AFX_TEMPERED ? "T " : "",
                 buf, a->mwcrit ? (a->mwcrit > 1 ? " ++" : " +") : "", a->flags & AFX_ENCHANT ? " (E)" : "");
        y = card_wrap(x, y, line, w, c) - 1;
        if (a->flags & AFX_IMPLICIT) {
            gfx_hline(x, y, w, RGB565(60, 50, 50));
            y += 2;
        }
    }
    return y;
}

static int card_power(const Game *g, const Item *it, int x, int y, int w)
{
    char buf[96];
    if (it->rarity == RAR_LEGEND && aspect_def(it->power)) {
        const AspectDef *a = aspect_def(it->power);
        aspect_text(buf, sizeof buf, a, it->power_roll);
        y = card_wrap(x, y, buf, w, aspect_usable(a, g->p.cls) ? C_LEG : C_DIM);
    } else if (it->rarity >= RAR_UNIQUE && unique_def(it->power)) {
        unique_text(buf, sizeof buf, unique_def(it->power), it->power_roll);
        y = card_wrap(x, y, buf, w, rarity_color((Rarity)it->rarity));
    }
    return y;
}

static int card_sockets(const Item *it, int x, int y)
{
    char buf[128];
    int i;
    for (i = 0; i < it->sockets && i < 2; i++) {
        if (it->gem[i] == GEM_NONE) {
            card_text(x, y, "O EMPTY SOCKET", C_DIM);
        } else {
            char eff[96];
            gem_effect_text(eff, sizeof eff, it->gem[i], (Slot)it->slot);
            snprintf(buf, sizeof buf, "O %s", eff);
            card_text(x, y, buf, gem_color(it->gem[i] / GEM_TIERS));
        }
        y += 9;
    }
    return y;
}

int menu_item_card(const Game *g, const Item *it, int x, int y, int w, const Item *compare)
{
    if (!it->used)
        return y;
    y = card_header(g, it, x, y, w, compare);
    y = card_affixes(g, it, x, y, w);
    y = card_power(g, it, x, y + 1, w);
    y = card_sockets(it, x, y + 1);
    if (it->rarity >= RAR_RARE) {
        char buf[96];
        snprintf(buf, sizeof buf, T("TEMPERS LEFT %d%s"), it->temper_left, it->locked ? T("  LOCKED") : "");
        card_text(x, y + 1, buf, C_DIM);
        y += 10;
    }
    return y;
}

/* --------------------------------------------------------------- frame */

bool menu_page_uses_lr(const Game *g)
{
    switch (g->page) {
    case PG_HERO:
    case PG_PARAGON: return true;
    case PG_TOWN:    return g->town != TOWN_LIST;
    default:         return false;
    }
}

static bool menu_busy(const Game *g)
{
    return g->upgrade_pick >= 0 || g->ench.open || (g->page == PG_TOWN && g->town != TOWN_LIST);
}

void menu_tick(Game *g, Input *in, uint32_t now)
{
    if (g->state != GS_MENU || screens_confirm_tick(g, in, now))
        return;
    if (!menu_busy(g)) {
        if (in_back(in)) {
            g->state = GS_BATTLE;
            input_block_held(in);
            return;
        }
        if (in_tab(in) || (!menu_page_uses_lr(g) && in_right(in)))
            g->page = (MenuPage)((g->page + 1) % PG_COUNT);
        else if (!menu_page_uses_lr(g) && in_left(in))
            g->page = (MenuPage)((g->page + PG_COUNT - 1) % PG_COUNT);
    } else if (in_tab(in)) {
        g->page = (MenuPage)((g->page + 1) % PG_COUNT);
    }
    switch (g->page) {
    case PG_HERO:    hero_tick(g, in); break;
    case PG_BAG:     bag_tick(g, in); break;
    case PG_SKILLS:  skills_tick(g, in); break;
    case PG_PARAGON: paragon_tick(g, in); break;
    case PG_TOWN:    town_tick(g, in); break;
    case PG_GOALS:   goals_tick(g, in); break;
    case PG_EMBERS:  embers_tick(g, in); break;
    default:         options_tick(g, in, now); break;
    }
}

static void draw_tabs(const Game *g)
{
    int i, w = SCREEN_W / PG_COUNT;
    gfx_fill_rect(0, 0, SCREEN_W, 14, RGB565(30, 22, 26));
    for (i = 0; i < PG_COUNT; i++) {
        int x = i * w;
        if (i == (int)g->page)
            gfx_fill_rect(x, 0, w, 14, C_HIL);
        font_draw(x + (w - font_text_width(page_names[i], 1)) / 2, 4, page_names[i], i == (int)g->page ? C_SEL : C_DIM, 1);
    }
    gfx_hline(0, 14, SCREEN_W, C_EDGE);
}

void menu_render(Game *g)
{
    gfx_clear(C_BG);
    draw_tabs(g);
    switch (g->page) {
    case PG_HERO:    hero_render(g); break;
    case PG_BAG:     bag_render(g); break;
    case PG_SKILLS:  skills_render(g); break;
    case PG_PARAGON: paragon_render(g); break;
    case PG_TOWN:    town_render(g); break;
    case PG_GOALS:   goals_render(g); break;
    case PG_EMBERS:  embers_render(g); break;
    default:         options_render(g); break;
    }
}
