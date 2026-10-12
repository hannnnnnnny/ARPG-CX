/*
 * render_side.c - the desktop's side panels for wide windows (fullscreen).
 *
 * The battle view is 4:3; on a 16:9 screen the game sits in the middle at
 * a whole-number scale and the bars either side would stay black. They
 * show what an idle player glances at instead: the floor map, the build's
 * numbers, recent loot and the floor event. Both panels are drawn into
 * one FB_W x FB_H buffer - the left panel in logical columns 0..79, the
 * right one in 80..159 - and the platform layer places them.
 */
#include "render.h"
#include "build.h"
#include "events.h"
#include "stats.h"
#include "../gfx/font.h"
#include "../gfx/tiles.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#ifdef GFX_HD

#define PANEL_W 80
#define C_BG     RGB565(14, 10, 11)
#define C_EDGE   RGB565(96, 62, 38)
#define C_HEAD   RGB565(232, 168, 84)
#define C_BODY   RGB565(206, 198, 186)
#define C_FAINT  RGB565(130, 120, 112)

static void panel_frame(int x0)
{
    gfx_fill_rect(x0, 0, PANEL_W, SCREEN_H, C_BG);
    gfx_rect(x0 + 2, 2, PANEL_W - 4, SCREEN_H - 4, C_EDGE);
}

/* A heading with a short rule under it; returns the y of the next line. */
static int heading(int x0, int y, const char *text)
{
    font_draw(x0 + 7, y, text, C_HEAD, 1);
    gfx_hline(x0 + 7, y + 10, PANEL_W - 14, C_EDGE);
    return y + 14;
}

static int line(int x0, int y, const char *text, uint16_t c)
{
    font_draw_fit(x0 + 7, y, text, PANEL_W - 14, c);
    return y + 10;
}

/* ---------------------------------------------------------------- map */

static uint16_t map_color(const World *w, int cx, int cy)
{
    uint8_t c = w->cell[cy][cx];
    if (cx == w->stairs_x && cy == w->stairs_y)
        return RGB565(255, 214, 90);
    if (c == CELL_WALL)
        return cy + 1 < MAP_H && w->cell[cy + 1][cx] != CELL_WALL ? RGB565(112, 100, 96) : 0;
    return RGB565(52, 48, 56);
}

/* 3x3 desktop pixels per cell: 144x96, explored cells only. */
static void draw_minimap(const World *w, int x0, int y0)
{
    int cx, cy, i, dx, dy;
    for (cy = 0; cy < MAP_H; cy++)
        for (cx = 0; cx < MAP_W; cx++) {
            uint16_t c = w->seen[cy][cx] ? map_color(w, cx, cy) : 0;
            if (!c)
                continue;
            for (dy = 0; dy < 3; dy++)
                for (dx = 0; dx < 3; dx++)
                    gfx_pixel_hd(x0, y0, cx * 3 + dx, cy * 3 + dy, c);
        }
    for (i = 0; i < MAX_MON; i++)
        if (w->mon[i].alive && w->seen[px_to_cell(w->mon[i].y)][px_to_cell(w->mon[i].x)])
            for (dy = 0; dy < 2; dy++)
                for (dx = 0; dx < 2; dx++)
                    gfx_pixel_hd(x0, y0, px_to_cell(w->mon[i].x) * 3 + dx, px_to_cell(w->mon[i].y) * 3 + dy,
                                 w->mon[i].boss ? RGB565(255, 120, 40) : RGB565(220, 50, 50));
    for (dy = -1; dy <= 2; dy++)
        for (dx = -1; dx <= 2; dx++)
            gfx_pixel_hd(x0, y0, px_to_cell(w->h.x) * 3 + dx, px_to_cell(w->h.y) * 3 + dy, RGB565(250, 250, 240));
}

/* --------------------------------------------------------------- panels */

static int floor_block(const World *w, int x0, int y)
{
    char buf[64];
    snprintf(buf, sizeof buf, T("FLOOR %d"), w->floor);
    y = line(x0, y, buf, C_BODY);
    y = line(x0, y, T(theme_name(w->theme)), C_FAINT);
    if (!w->boss_floor) {
        snprintf(buf, sizeof buf, T("SLAIN %d/%d"), MIN(w->kills, w->quota), w->quota);
        y = line(x0, y, buf, C_FAINT);
    }
    return y;
}

static int build_block(const World *w, const Profile *p, int x0, int y)
{
    const ClassDef *c = &class_defs[p->cls % CLASS_COUNT];
    char buf[64], n[16];
    y = line(x0, y, p->look.name, C_HEAD);
    y = line(x0, y, c->name, C_BODY);
    y = line(x0, y, c->preset[p->preset % PRESETS].name, RGB565(255, 170, 90));
    if (p->paragon_level > 0)
        snprintf(buf, sizeof buf, T("PARAGON %d"), p->paragon_level);
    else
        snprintf(buf, sizeof buf, T("LEVEL %d"), p->level);
    y = line(x0, y, buf, RGB565(190, 150, 255)) + 4;
    fmt_num(n, sizeof n, stats_dps(&w->st, p));
    snprintf(buf, sizeof buf, T("DPS %s"), n);
    y = line(x0, y, buf, C_BODY);
    snprintf(buf, sizeof buf, T("CRIT %d%%"), (int)(w->st.crit * 100 + 0.5));
    y = line(x0, y, buf, C_BODY);
    snprintf(buf, sizeof buf, T("APS %.2f"), w->st.aps);
    y = line(x0, y, buf, C_BODY);
    snprintf(buf, sizeof buf, T("MYTHICS %d"), (int)p->n_mythic);
    return line(x0, y, buf, RGB565(255, 120, 60));
}

static void left_panel(const World *w, const Profile *p)
{
    int y = heading(0, 8, T("MAP"));
    draw_minimap(w, 4, y);
    y = floor_block(w, 0, y + 52) + 8;
    y = heading(0, y, T("BUILD"));
    build_block(w, p, 0, y);
}

static void right_panel(const World *w)
{
    char buf[96];
    int i, y = heading(PANEL_W, 8, T("LOOT"));
    if (!w->loot.n)
        y = line(PANEL_W, y, T("NOTHING YET"), C_FAINT);
    for (i = 0; i < w->loot.n; i++)
        y = line(PANEL_W, y, w->loot.name[i], w->loot.color[i]);
    y = heading(PANEL_W, MAX(y + 8, 112), T("FLOOR EVENT"));
    events_d4_status(w, buf, sizeof buf);
    font_draw_wrapped(PANEL_W + 7, y, buf[0] ? buf : T("NONE ON THIS FLOOR"), PANEL_W - 14, buf[0] ? C_BODY : C_FAINT);
}

void render_side(const World *w, const Profile *p)
{
    panel_frame(0);
    panel_frame(PANEL_W);
    if (!w || !p)
        return;
    left_panel(w, p);
    right_panel(w);
}

#else

void render_side(const World *w, const Profile *p) { (void)w; (void)p; }

#endif
