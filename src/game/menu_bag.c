/*
 * menu_bag.c - BAG page: carried items with their power change against
 * what is equipped, the item tooltip, equip / salvage / lock.
 */
#include "menu_int.h"
#include "items.h"
#include "stats.h"
#include "../gfx/font.h"
#include "../gfx/sprites.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define BAG_ROWS 8

void bag_tick(Game *g, Input *in)
{
    int *sel = &g->sel[PG_BAG], i;
    double gold;
    menu_move(in, sel, BAG_SIZE + 1);
    i = *sel;
    if (i == BAG_SIZE) {
        if (in_ok(in))
            g->confirm = CF_SALVAGE_ALL;
        return;
    }
    if (in_ok(in) && prog_equip_from_bag(&g->p, i)) {
        game_profile_changed(g);
        game_toast(g, "EQUIPPED", C_GOOD);
    } else if (in_alt(in)) {
        if (g->p.bag[i].locked) {
            game_toast(g, "ITEM IS LOCKED (CTRL)", C_BAD);
        } else if (prog_salvage_bag(&g->p, i, &gold)) {
            char buf[128], n[16];
            fmt_num(n, sizeof n, gold);
            snprintf(buf, sizeof buf, T("SALVAGED: %s GOLD + MATERIALS"), n);
            game_toast(g, buf, C_SEL);
        }
    } else if (in_lock(in) && g->p.bag[i].used) {
        g->p.bag[i].locked = !g->p.bag[i].locked;
    }
}

/* Power change of each bag item. The calculator has no FPU, so a full
 * stats comparison per row per frame is too slow: refresh a few times a
 * second, or at once when the item in a row changes. */
static double g_ratio[BAG_SIZE];
static uint32_t g_ratio_key[BAG_SIZE];

static double bag_ratio(const Game *g, int i)
{
    const Item *it = &g->p.bag[i];
    uint32_t key = (uint32_t)it->ilvl * 2654435761u ^ (uint32_t)it->power_roll ^ (uint32_t)(it->main * 977.0)
                 ^ (uint32_t)((g->tick + i * 2) / 15) << 20;   /* rows refresh staggered */
    if (g_ratio_key[i] != key) {
        g_ratio_key[i] = key;
        g_ratio[i] = item_upgrade_ratio(&g->p, it);
    }
    return g_ratio[i];
}

static void bag_row(const Game *g, int i, int y)
{
    const Item *it = &g->p.bag[i];
    char buf[128], name[64];
    double r;
    if (!it->used) {
        font_draw(18, y, "-", RGB565(70, 60, 56), 1);
        return;
    }
    gfx_blit(spr_icon(it->slot, it->rarity), 6, y - 1, 0);
    item_name(name, sizeof name, it);
    font_draw_fit(18, y, name, it->ancestral ? 174 : 200, rarity_color((Rarity)it->rarity));
    if (it->ancestral)
        font_draw(196, y, "ANC", RGB565(255, 90, 60), 1);
    snprintf(buf, sizeof buf, "%d", item_power(it));
    font_draw(222, y, buf, C_DIM, 1);
    r = bag_ratio(g, i) * 100.0;
    snprintf(buf, sizeof buf, "%s%d%%", r >= 0 ? "+" : "", (int)r);
    font_draw(298 - font_text_width(buf, 1), y, buf, r > 0.5 ? C_GOOD : r < -0.5 ? C_BAD : C_DIM, 1);
    if (it->locked)
        font_draw(306, y, "L", C_SEL, 1);
}

/* ------------------------------------------------- hover comparison */

#define TIP_W 150

/* Bag index under the mouse, or -1. */
static int hovered_item(const Game *g, int top)
{
    int row, idx;
    if (!g->mouse || g->my < 20 || g->my >= 20 + BAG_ROWS * 11 || g->mx < 4 || g->mx >= 300)
        return -1;
    row = (g->my - 20) / 11;
    idx = top + row;
    return idx < BAG_SIZE && g->p.bag[idx].used ? idx : -1;
}

static void tip_line(int x, int *y, const StatDelta *d)
{
    char buf[96];
    double v = d->delta * 100.0;
    uint16_t c = v > 0 ? C_GOOD : C_BAD;
    /* relative changes as whole percents, percentage points with a decimal */
    snprintf(buf, sizeof buf, d->relative ? "%s%.0f%% %s" : "%s%.1f%% %s", v > 0 ? "+" : "", v, T(d->label));
    font_draw_fit(x, *y, buf, TIP_W - 8, c);
    *y += 10;
}

/* A box at the cursor: what this item changes against what it would
 * replace, stat by stat, and the overall power change. */
static void hover_tooltip(Game *g, int idx)
{
    static StatDelta d[STAT_DELTAS];
    static int n, cached = -1, cached_t;
    const Item *it = &g->p.bag[idx];
    char name[64], buf[64];
    int i, h, x, y;
    double r;
    if (cached != idx || g->tick - cached_t > 15) {     /* a full stats pass: a few times a second */
        n = stats_compare(&g->p, it, d, STAT_DELTAS);
        cached = idx;
        cached_t = g->tick;
    }
    h = 34 + MAX(n, 1) * 10;
    x = MIN(g->mx + 10, SCREEN_W - TIP_W - 2);
    y = MIN(g->my + 6, FOOTER_Y - h - 2);
    gfx_fill_rect(x, y, TIP_W, h, C_BG);
    gfx_rect(x, y, TIP_W, h, rarity_color((Rarity)it->rarity));
    item_name(name, sizeof name, it);
    font_draw_fit(x + 4, y + 3, name, TIP_W - 8, rarity_color((Rarity)it->rarity));
    font_draw(x + 4, y + 13, T(g->p.equip[item_target_slot(&g->p, it)].used ? "VS EQUIPPED" : "SLOT IS EMPTY"),
              C_DIM, 1);
    y += 23;
    for (i = 0; i < n; i++)
        tip_line(x + 4, &y, &d[i]);
    if (!n)
        font_draw(x + 4, (y += 10) - 10, T("NO STAT CHANGES"), C_DIM, 1);
    r = bag_ratio(g, idx) * 100.0;
    snprintf(buf, sizeof buf, T("%s%d%% POWER"), r >= 0 ? "+" : "", (int)r);
    font_draw(x + 4, y + 1, buf, r > 0.5 ? C_GOOD : r < -0.5 ? C_BAD : C_DIM, 1);
}

void bag_render(Game *g)
{
    int sel = g->sel[PG_BAG], top = menu_scroll(sel, BAG_SIZE + 1, BAG_ROWS), i, used = 0;
    char buf[128];
    for (i = 0; i < BAG_SIZE; i++)
        used += g->p.bag[i].used;
    for (i = 0; i < BAG_ROWS; i++) {
        int idx = top + i, y = 20 + i * 11;
        menu_row_highlight(y, 11, idx == sel);
        if (idx == BAG_SIZE)
            font_draw(18, y, "[ SALVAGE ALL UNLOCKED ]", C_SEL, 1);
        else if (idx < BAG_SIZE)
            bag_row(g, idx, y);
    }
    snprintf(buf, sizeof buf, "%d/%d", used, BAG_SIZE);
    font_draw(SCREEN_W - 4 - font_text_width(buf, 1), 110, buf, C_DIM, 1);
    gfx_hline(4, 108, 200, C_EDGE);
    if (sel < BAG_SIZE && g->p.bag[sel].used) {
        const Item *it = &g->p.bag[sel];
        menu_item_card(g, it, 6, 112, 300, &g->p.equip[item_target_slot(&g->p, it)]);
    }
    menu_footer(g, "ENTER: EQUIP  DEL: SALVAGE  CTRL: LOCK");
    if (hovered_item(g, top) >= 0)
        hover_tooltip(g, hovered_item(g, top));
}
