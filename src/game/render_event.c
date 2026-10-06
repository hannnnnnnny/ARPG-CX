/*
 * render_event.c - floor event objects (shrine, cursed chest, fallen
 * adventurer), the treasure goblin's sack and champion name plates.
 */
#include "render_fx.h"
#include "events.h"
#include "world_int.h"
#include "../gfx/gfx.h"
#include "../gfx/font.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define C_STONE  RGB565(120, 112, 104)
#define C_STONE2 RGB565(80, 74, 70)
#define C_WOOD   RGB565(120, 70, 30)
#define C_GOLDEN RGB565(255, 210, 80)

static const uint16_t shrine_colors[SH_COUNT] = {
    RGB565(255, 230, 120), RGB565(255, 70, 70), RGB565(255, 200, 40),
    RGB565(170, 120, 255), RGB565(255, 140, 40), RGB565(110, 200, 255),
};

/* A bobbing marker so a waiting object is noticed from across the room. */
static void marker(int x, int y, int tick, uint16_t c)
{
    int b = (tick >> 3) & 3;
    gfx_line(x - 3, y - 22 + b, x, y - 18 + b, c);
    gfx_line(x, y - 18 + b, x + 3, y - 22 + b, c);
}

static void draw_shrine(const World *w, int x, int y)
{
    bool live = w->ev.state == ES_WAITING;
    uint16_t c = shrine_colors[w->ev.shrine % SH_COUNT];
    gfx_fill_rect(x - 6, y + 1, 12, 4, C_STONE2);
    gfx_fill_rect(x - 3, y - 9, 6, 11, C_STONE);
    gfx_hline(x - 4, y - 10, 8, C_STONE2);
    if (!live)
        return;
    fx_ellipse(x, y - 13, 3 + ((w->tick >> 2) & 1), c);
    gfx_fill_rect(x - 1, y - 14, 3, 3, c);
    gfx_blend_rect(x - 2, y - 40, 5, 26, c);
    marker(x, y - 14, w->tick, c);
}

static void draw_chest(const World *w, int x, int y)
{
    int shake = w->ev.state == ES_RUNNING ? ((w->tick >> 1) & 1) : 0;
    uint16_t glow = w->ev.state == ES_RUNNING ? RGB565(255, 60, 60) : RGB565(170, 80, 255);
    if (w->ev.state != ES_DONE)
        fx_ellipse(x, y + 2, 9 + ((w->tick >> 3) & 1), glow);
    x += shake;
    gfx_fill_rect(x - 6, y - 5, 12, 8, C_WOOD);
    gfx_rect(x - 6, y - 5, 12, 8, RGB565(60, 34, 14));
    gfx_hline(x - 6, y - 2, 12, C_GOLDEN);
    gfx_fill_rect(x - 1, y - 3, 2, 3, C_GOLDEN);
    if (w->ev.state == ES_DONE)
        gfx_fill_rect(x - 6, y - 9, 12, 3, RGB565(70, 40, 18));     /* the lid stands open */
    else
        marker(x, y - 4, w->tick, glow);
}

static void draw_fallen(const World *w, int x, int y)
{
    gfx_fill_rect(x - 5, y, 10, 2, RGB565(200, 190, 160));         /* bones */
    gfx_fill_rect(x - 7, y - 3, 4, 3, RGB565(225, 218, 195));       /* skull */
    gfx_pixel(x - 6, y - 2, 0);
    gfx_fill_rect(x + 2, y - 4, 5, 4, RGB565(100, 70, 40));         /* pack */
    if (w->ev.state != ES_WAITING)
        return;
    gfx_fill_rect(x - 1, y - 7, 4, 5, RGB565(235, 220, 170));       /* the page */
    marker(x, y - 6, w->tick, RGB565(235, 220, 170));
}

void render_event_object(const World *w, int cam_x, int cam_y)
{
    int x, y;
    if (w->ev.kind != EV_SHRINE && w->ev.kind != EV_CHEST && w->ev.kind != EV_FALLEN)
        return;
    if (!w->seen[w->ev.cy][w->ev.cx])
        return;
    x = w->ev.cx * TILE_SIZE + TILE_SIZE / 2 - cam_x;
    y = w->ev.cy * TILE_SIZE + TILE_SIZE / 2 - cam_y;
    if (w->ev.kind == EV_SHRINE)
        draw_shrine(w, x, y);
    else if (w->ev.kind == EV_CHEST)
        draw_chest(w, x, y);
    else
        draw_fallen(w, x, y);
}

/* Above the monster's sprite: goblin sack and labels, champion plates. */
void render_monster_extras(const World *w, const Monster *m, int x, int top)
{
    char name[96];
    int tw;
    if (m->goblin) {
        gfx_fill_rect(x - 8 * m->face - 3, top + 6, 6, 6, C_GOLDEN);
        gfx_pixel(x - 8 * m->face - 2 + ((w->tick >> 2) & 3), top + 4, RGB565(255, 250, 200));
    }
    if ((m->champ & CH_WARDED) && champion_warded(w, m))
        gfx_circle(x, top + 8, 11, RGB565(170, 220, 255));
    if (!m->aggro || (!m->goblin && !m->champ))
        return;
    if (m->goblin)
        snprintf(name, sizeof name, "%s", T("TREASURE GOBLIN"));
    else
        champion_name(name, sizeof name, m);
    tw = MIN(font_text_width(name, 1), 140);
    font_draw_fit(x - tw / 2, top - 13, name, tw, m->goblin ? C_GOLDEN : RGB565(110, 160, 255));
}
