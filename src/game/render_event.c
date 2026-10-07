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
#define C_RIFT   RGB565(190, 110, 255)

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

/* A dark obelisk with a burning skull; it pulses red while its waves last. */
static void draw_cursed(const World *w, int x, int y)
{
    uint16_t c = w->ev.state == ES_DONE ? RGB565(90, 80, 80) : RGB565(255, 60, 50);
    gfx_fill_rect(x - 7, y + 1, 14, 4, RGB565(50, 30, 34));
    gfx_fill_rect(x - 4, y - 14, 8, 16, RGB565(70, 44, 52));
    gfx_fill_rect(x - 2, y - 20, 4, 6, RGB565(70, 44, 52));
    gfx_fill_rect(x - 2, y - 11, 4, 3, c);
    gfx_pixel(x - 1, y - 10, 0);
    gfx_pixel(x + 1, y - 10, 0);
    if (w->ev.state == ES_DONE)
        return;
    fx_ellipse(x, y + 3, 10 + ((w->tick >> 2) & 1) + (w->ev.state == ES_RUNNING ? 4 : 0), c);
    if (w->ev.state == ES_WAITING)
        marker(x, y - 18, w->tick, c);
}

/* A spinning violet tear in the air; a scar on the ground once sealed. */
static void draw_rift(const World *w, int x, int y)
{
    int k, r = w->ev.state == ES_RUNNING && !w->ev.stage ? 10 + ((w->tick >> 2) & 1) : 6;
    if (w->ev.state == ES_DONE) {
        gfx_hline(x - 6, y, 12, RGB565(90, 60, 120));
        return;
    }
    gfx_blend_rect(x - r, y - r * 2, r * 2, r * 3, RGB565(60, 20, 90));
    for (k = 0; k < 6; k++) {
        int a = (w->tick * 3 + k * 43) & 255, dx = (r * (a < 128 ? a - 64 : 192 - a)) / 64;
        gfx_line(x, y - r, x + dx, y - r - r + (k * 7 % r), C_RIFT);
    }
    fx_ellipse(x, y - r, r, C_RIFT);
    fx_ellipse(x, y - r, r / 2, RGB565(255, 230, 255));
    if (w->ev.state == ES_WAITING)
        marker(x, y - r * 2, w->tick, C_RIFT);
}

void render_event_object(const World *w, int cam_x, int cam_y)
{
    int x, y;
    if (w->ev.kind != EV_SHRINE && w->ev.kind != EV_CHEST && w->ev.kind != EV_FALLEN && w->ev.kind != EV_CURSED
        && w->ev.kind != EV_RIFT)
        return;
    if (!w->seen[w->ev.cy][w->ev.cx])
        return;
    x = w->ev.cx * TILE_SIZE + TILE_SIZE / 2 - cam_x;
    y = w->ev.cy * TILE_SIZE + TILE_SIZE / 2 - cam_y;
    if (w->ev.kind == EV_CURSED)
        draw_cursed(w, x, y);
    else if (w->ev.kind == EV_RIFT)
        draw_rift(w, x, y);
    else if (w->ev.kind == EV_SHRINE)
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
    if (m->special == MS_HUNT)                       /* the blood mark over its head */
        gfx_fill_rect(x - 1, top - 4 + ((w->tick >> 3) & 1), 3, 3, RGB565(255, 50, 40));
    if (!m->aggro || (!m->goblin && !m->champ && !m->special))
        return;
    if (m->goblin)
        snprintf(name, sizeof name, "%s", T("TREASURE GOBLIN"));
    else if (m->special == MS_BUTCHER)
        snprintf(name, sizeof name, "%s", T("THE FLESHRENDER"));
    else if (m->special == MS_HUNT) {
        char base[80];
        champion_name(base, sizeof base, m);
        tjoin(name, sizeof name, "BLOODMARKED", base);
    } else
        champion_name(name, sizeof name, m);
    tw = MIN(font_text_width(name, 1), 140);
    font_draw_fit(x - tw / 2, top - 13, name, tw, m->goblin ? C_GOLDEN : m->special ? RGB565(255, 80, 60)
                  : RGB565(110, 160, 255));
}
