/*
 * menu_codex.c - the CODEX page: every unique and mythic, found or still
 * a silhouette, where each one drops, and travel back to its guardian.
 */
#include "menu_int.h"
#include "aspects.h"
#include "build.h"
#include "codex.h"
#include "items.h"
#include "progress.h"
#include "../gfx/font.h"
#include "../gfx/sprites.h"
#include "../gfx/tiles.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define ROWS 9
#define ROW_Y 30
#define ROW_H 12
#define DETAIL_Y 146

static int entries(int *ids)
{
    return codex_entries(ids, UNIQUE_BITS);
}

/* Back to the deepest floor already reached where this unique's guardian waits. */
static void travel(Game *g, int unique)
{
    char buf[64];
    int f = codex_guardian_floor_below(codex_guardian(unique), g->p.best_floor);
    if (f <= 0) {
        game_toast(g, "NOT REACHED YET", C_BAD);
        return;
    }
    session_travel(&g->s, &g->p, f);
    g->state = GS_BATTLE;
    snprintf(buf, sizeof buf, T("TRAVELLED TO FLOOR %d"), f);
    game_toast(g, buf, C_GOOD);
}

void codex_tick(Game *g, Input *in)
{
    int ids[UNIQUE_BITS], n = entries(ids);
    menu_move(in, &g->sel[PG_CODEX], n);
    if (in_ok(in) && n > 0)
        travel(g, ids[CLAMP(g->sel[PG_CODEX], 0, n - 1)]);
}

static void source_text(char *out, size_t cap, int unique)
{
    int t = codex_guardian(unique);
    if (t < 0)
        snprintf(out, cap, "%s", T("DROPS ANYWHERE IN TORMENT, VERY RARELY"));
    else
        snprintf(out, cap, T("GUARDIAN OF THE %s: FLOOR %d, THEN EVERY 50"), T(theme_name(t)), 10 * (t + 1));
}

static void draw_row(const Game *g, int unique, int y, bool sel)
{
    const UniqueDef *u = unique_def(unique);
    bool found = prog_found(&g->p, unique);
    Rarity r = u->mythic ? RAR_MYTHIC : RAR_UNIQUE;
    int t = codex_guardian(unique);
    menu_row_highlight(y, ROW_H - 1, sel);
    gfx_blit(spr_icon(u->slot, found ? r : RAR_COMMON), 6, y + 1, 0);
    font_draw_fit(18, y + 2, found ? u->name : "???", 200, found ? rarity_color(r) : C_DIM);
    font_draw_fit(226, y + 2, t < 0 ? "ANYWHERE" : theme_name(t), 88, C_DIM);
}

static void draw_detail(const Game *g, int unique)
{
    char buf[160];
    const UniqueDef *u = unique_def(unique);
    bool found = prog_found(&g->p, unique);
    int y = DETAIL_Y, f = codex_guardian_floor_below(codex_guardian(unique), g->p.best_floor);
    gfx_hline(6, y - 4, 308, RGB565(90, 60, 40));
    if (found) {
        unique_text(buf, sizeof buf, u, 1000);
        y = font_draw_wrapped(6, y, buf, 308, C_TEXT);
    } else {
        y = font_draw_wrapped(6, y, "NOT FOUND YET. ITS POWER IS REVEALED WHEN IT DROPS.", 308, C_DIM);
    }
    if (u->cls != ANY_CLASS) {
        snprintf(buf, sizeof buf, T("%s ONLY"), T(class_defs[u->cls % CLASS_COUNT].name));
        font_draw(6, y, buf, u->cls == g->p.cls ? C_TEXT : C_BAD, 1);
        y += 10;
    }
    source_text(buf, sizeof buf, unique);
    y = font_draw_wrapped(6, y, buf, 308, C_SEL);
    if (codex_guardian(unique) >= 0)
        font_draw(6, y, f > 0 ? "ENTER: TRAVEL TO THIS GUARDIAN" : "NOT REACHED YET", f > 0 ? C_GOOD : C_DIM, 1);
}

void codex_render(Game *g)
{
    char buf[96];
    int ids[UNIQUE_BITS], n = entries(ids), i, found = 0, sel, top;
    for (i = 0; i < n; i++)
        found += prog_found(&g->p, ids[i]);
    snprintf(buf, sizeof buf, T("CODEX: %d/%d UNIQUES AND MYTHICS FOUND"), found, n);
    font_draw(6, 18, buf, C_SEL, 1);
    if (n == 0)
        return;
    sel = g->sel[PG_CODEX] = CLAMP(g->sel[PG_CODEX], 0, n - 1);
    top = menu_scroll(sel, n, ROWS);
    for (i = 0; i < ROWS && top + i < n; i++)
        draw_row(g, ids[top + i], ROW_Y + i * ROW_H, top + i == sel);
    draw_detail(g, ids[sel]);
    menu_footer(g, "UP/DOWN: BROWSE   ENTER: TRAVEL   ESC: BATTLE");
}
