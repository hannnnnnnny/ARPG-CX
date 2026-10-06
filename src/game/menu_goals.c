/*
 * menu_goals.c - the GOALS page: renown, the three bounties and the
 * achievement list.
 */
#include "menu_int.h"
#include "goals.h"
#include "../gfx/font.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define ACH_VISIBLE 7
#define ACH_Y 124
#define C_GOLDEN RGB565(255, 210, 80)

void goals_tick(Game *g, Input *in)
{
    menu_move(in, &g->sel[PG_GOALS], ACH_COUNT);
}

static void progress_bar(int x, int y, int w, double frac, uint16_t c)
{
    gfx_fill_rect(x, y, w, 4, RGB565(40, 30, 30));
    gfx_fill_rect(x, y, (int)(w * CLAMP(frac, 0.0, 1.0)), 4, c);
}

static void draw_renown(const Profile *p)
{
    char buf[96];
    int n = ach_count(p), tier = renown_tier(p);
    snprintf(buf, sizeof buf, T("RENOWN %d"), tier);
    font_draw(6, 20, buf, C_GOLDEN, 1);
    snprintf(buf, sizeof buf, T("ACHIEVEMENTS %d/%d"), n, ACH_COUNT);
    font_draw(314 - font_text_width(buf, 1), 20, buf, C_DIM, 1);
    snprintf(buf, sizeof buf, T("+%d%% DAMAGE  +%d%% LIFE  (NEXT TIER: %d MORE)"), (int)(tier * RENOWN_DMG_PCT),
             (int)(tier * RENOWN_DMG_PCT), RENOWN_PER_TIER - n % RENOWN_PER_TIER);
    font_draw_fit(6, 31, buf, 308, C_TEXT);
}

static void draw_bounties(const Profile *p)
{
    int i;
    font_draw(6, 46, "BOUNTIES (AUTOMATIC)", C_SEL, 1);
    for (i = 0; i < BOUNTY_SLOTS; i++) {
        const Bounty *b = &p->bounty[i];
        char t[64], buf[24];
        int y = 58 + i * 14;
        if (b->kind == BT_NONE)
            continue;
        bounty_text(t, sizeof t, b);
        font_draw_fit(12, y, t, 180, C_TEXT);
        progress_bar(196, y + 2, 70, b->need ? (double)b->have / b->need : 0, C_GOLDEN);
        snprintf(buf, sizeof buf, "%d/%d", b->have, b->need);
        font_draw(314 - font_text_width(buf, 1), y, buf, C_DIM, 1);
    }
    font_draw(12, 98, "EACH BOUNTY PAYS GOLD, MATERIALS AND AN ITEM.", C_DIM, 1);
}

static void draw_achievement(const Profile *p, int id, int y, bool sel)
{
    char desc[64], a[16], b[16], buf[40];
    bool done = ach_earned(p, id);
    menu_row_highlight(y - 1, 11, sel);
    if (done)
        gfx_fill_rect(7, y, 6, 6, C_GOOD);
    else
        gfx_rect(7, y, 6, 6, C_DIM);
    font_draw_fit(18, y, ach_defs[id].name, 112, done ? C_GOOD : C_TEXT);
    ach_text(desc, sizeof desc, id);
    font_draw_fit(134, y, desc, done ? 180 : 126, C_DIM);
    if (done)
        return;
    fmt_num(a, sizeof a, (double)MIN(ach_progress(p, id), ach_defs[id].need));
    fmt_num(b, sizeof b, (double)ach_defs[id].need);
    snprintf(buf, sizeof buf, "%s/%s", a, b);
    font_draw(314 - font_text_width(buf, 1), y, buf, C_DIM, 1);
}

void goals_render(Game *g)
{
    int sel = g->sel[PG_GOALS] % ACH_COUNT, top = menu_scroll(sel, ACH_COUNT, ACH_VISIBLE), i;
    draw_renown(&g->p);
    draw_bounties(&g->p);
    font_draw(6, ACH_Y - 12, "ACHIEVEMENTS", C_SEL, 1);
    for (i = top; i < ACH_COUNT && i < top + ACH_VISIBLE; i++)
        draw_achievement(&g->p, i, ACH_Y + (i - top) * 13, i == sel);
    menu_footer(g, "UP/DOWN: SCROLL   ESC: BATTLE");
}
