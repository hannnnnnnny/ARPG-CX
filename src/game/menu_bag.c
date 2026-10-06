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
}
