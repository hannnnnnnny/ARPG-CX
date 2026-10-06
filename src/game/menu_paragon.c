/*
 * menu_paragon.c - BOARD page: the four 15x15 paragon boards. Arrows move
 * the cursor, ENTER buys a node next to an owned one, DEL flips through
 * the boards, CTRL slots a glyph into an owned glyph socket.
 */
#include "menu_int.h"
#include "paragon.h"
#include "../gfx/font.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define CELL 9
#define GX 6
#define GY 22

void paragon_tick(Game *g, Input *in)
{
    int b = g->para_board;
    if (in_left(in))  g->para_x = (g->para_x + BOARD_N - 1) % BOARD_N;
    if (in_right(in)) g->para_x = (g->para_x + 1) % BOARD_N;
    if (in_up(in))    g->para_y = (g->para_y + BOARD_N - 1) % BOARD_N;
    if (in_down(in))  g->para_y = (g->para_y + 1) % BOARD_N;
    if (in_alt(in))
        g->para_board = (b + 1) % PARAGON_BOARDS;
    if (in_ok(in)) {
        if (paragon_buy(&g->p, b, g->para_x, g->para_y)) {
            g->p.auto_paragon = 0;           /* a hand-made board stays as it is */
            game_profile_changed(g);
        } else {
            game_toast(g, prog_paragon_available(&g->p) > 0 ? "MUST TOUCH AN OWNED NODE" : "NO PARAGON POINTS", C_BAD);
        }
    }
    if (in_lock(in) && paragon_node(g->p.cls, b, g->para_x, g->para_y)->type == PN_GLYPH) {
        int cur = g->p.glyph[b] ? (g->p.glyph[b] - 1) & 1 : 1;
        if (glyph_socket(&g->p, b, !cur))
            game_profile_changed(g);
        else
            game_toast(g, "BUY THE SOCKET FIRST", C_BAD);
    }
}

static uint16_t node_color(int type, bool owned)
{
    static const uint16_t on[] = { 0, RGB565(110, 220, 110), RGB565(200, 200, 190), RGB565(110, 150, 255),
                                   RGB565(255, 220, 80), RGB565(255, 140, 30), RGB565(230, 60, 80),
                                   RGB565(255, 255, 255) };
    uint16_t c = on[type % 8];
    return owned ? c : (uint16_t)((c >> 2) & 0x39E7);
}

static void draw_board(const Game *g)
{
    int b = g->para_board, x, y;
    for (y = 0; y < BOARD_N; y++)
        for (x = 0; x < BOARD_N; x++) {
            const ParaNode *n = paragon_node(g->p.cls, b, x, y);
            int sx = GX + x * CELL, sy = GY + y * CELL;
            bool owned = paragon_owned(&g->p, b, x, y);
            if (n->type == PN_NONE)
                continue;
            gfx_fill_rect(sx + 1, sy + 1, CELL - 2, CELL - 2, node_color(n->type, owned));
            if (!owned && paragon_can_buy(&g->p, b, x, y))
                gfx_rect(sx, sy, CELL, CELL, RGB565(150, 140, 120));
        }
    if (g->p.glyph[b]) {                     /* the glyph's radius */
        int r = glyph_radius(g->p.glyph_lvl[(g->p.glyph[b] - 1) % GLYPH_COUNT]);
        gfx_rect(GX + (BOARD_N / 2 - r) * CELL, GY + (BOARD_N / 2 - r) * CELL, (2 * r + 1) * CELL, (2 * r + 1) * CELL,
                 RGB565(230, 60, 80));
    }
    gfx_rect(GX + g->para_x * CELL - 1, GY + g->para_y * CELL - 1, CELL + 2, CELL + 2,
             (g->tick >> 3) & 1 ? C_SEL : C_TEXT);
}

static void glyph_info(const Game *g, int x, int y)
{
    char buf[128];
    int b = g->para_board, gid;
    if (!g->p.glyph[b]) {
        font_draw_wrapped(x, y, "NO GLYPH. BUY THE SOCKET, THEN CTRL.", 160, C_DIM);
        return;
    }
    gid = (g->p.glyph[b] - 1) % GLYPH_COUNT;
    snprintf(buf, sizeof buf, T("GLYPH: %s  LV %d"), T(glyph_defs[gid].name), MAX(1, g->p.glyph_lvl[gid]));
    font_draw(x, y, buf, RGB565(230, 60, 80), 1);
    glyph_text(buf, sizeof buf, gid, g->p.glyph_lvl[gid]);
    y = font_draw_wrapped(x, y + 10, buf, 160, C_TEXT);
    snprintf(buf, sizeof buf, T("STAT IN RADIUS %d/25"), glyph_stat_in_radius(&g->p, b));
    font_draw(x, y, buf, C_DIM, 1);
    font_draw_wrapped(x, y + 10, glyph_defs[gid].bonus_desc, 160, glyph_stat_in_radius(&g->p, b) >= 25 ? C_GOOD : C_DIM);
}

static void node_info(const Game *g, int x, int y)
{
    const ParaNode *n = paragon_node(g->p.cls, g->para_board, g->para_x, g->para_y);
    char buf[128];
    if (n->type == PN_LEGEND) {
        const ParaLegend *l = &para_legends[g->p.cls % CLASS_COUNT][MAX(g->para_board - 1, 0)];
        font_draw(x, y, l->name, C_LEG, 1);
        font_draw_wrapped(x, y + 10, l->desc, 160, C_TEXT);
        return;
    }
    paragon_node_text(buf, sizeof buf, n, g->p.cls);
    font_draw_wrapped(x, y, buf, 160, node_color(n->type, true));
}

void paragon_render(Game *g)
{
    static const char *const names[PARAGON_BOARDS] = { "STARTING BOARD", "BOARD II", "BOARD III", "BOARD IV" };
    char buf[128];
    int x = 148;
    draw_board(g);
    font_draw(x, 20, names[g->para_board], C_SEL, 1);
    if (!paragon_board_open(&g->p, g->para_board))
        font_draw(x, 30, "LOCKED: BUY THE GATE BELOW", C_BAD, 1);
    snprintf(buf, sizeof buf, T("POINTS %d  (PARAGON %d)"), prog_paragon_available(&g->p), g->p.paragon_level);
    font_draw(x, 42, buf, prog_paragon_available(&g->p) ? C_GOOD : C_DIM, 1);
    if (g->p.level < PARAGON_START)
        font_draw_wrapped(x, 54, "PARAGON POINTS START AT LEVEL 50.", 160, C_DIM);
    node_info(g, x, 70);
    glyph_info(g, x, 120);
    snprintf(buf, sizeof buf, T("AUTO: %s (OPTIONS)"), T(g->p.auto_paragon ? "ON" : "OFF"));
    font_draw(x, 196, buf, C_DIM, 1);
    menu_footer(g, "ARROWS: MOVE  ENTER: BUY  DEL: BOARD  CTRL: GLYPH");
}
