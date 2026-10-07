/*
 * render_myth.c - the look of the mythic powers and of impacts: falling
 * stars, crescent waves, the singularity, the reaper's scythe, hit sparks
 * and gibs; the circling blades and violet motes around a hero who wears
 * mythics, and the cold tint while time stands still.
 */
#include "render_fx.h"
#include "world_int.h"
#include "../gfx/gfx.h"

#define C_WHITE  RGB565(255, 250, 235)
#define C_STAR   RGB565(255, 236, 150)
#define C_VIOLET RGB565(200, 120, 255)
#define C_TIME   RGB565(120, 170, 230)

static int progress(const Effect *e, int full)
{
    return e->dur > 0 ? full * MIN(e->t, e->dur) / e->dur : full;
}

/* A star streaking down at a slant, then a cross of light where it lands. */
static void star(const Effect *e, int x, int y)
{
    int left = MAX(0, 4 - e->t), sx = x - left * 10, sy = y - left * 26;
    if (left > 0) {
        gfx_line(sx - 12, sy - 30, sx, sy, e->color);
        gfx_fill_rect(sx - 2, sy - 2, 5, 5, C_WHITE);
        return;
    }
    fx_ellipse(x, y, 4 + e->t * 2, e->t < 7 ? C_WHITE : e->color);
    if (e->t < 7) {
        gfx_line(x - 12, y, x + 12, y, C_WHITE);
        gfx_line(x, y - 9, x, y + 6, C_WHITE);
        gfx_line(x - 6, y - 4, x + 6, y + 3, e->color);
        gfx_line(x - 6, y + 3, x + 6, y - 4, e->color);
    }
}

/* A pale crescent racing from the hero along the line (x, y) -> (x2, y2). */
static void crescent(const Effect *e, int x, int y, int x2, int y2)
{
    int p = progress(e, 64), cx = x + (x2 - x) * p / 64, cy = y + (y2 - y) * p / 64, k;
    int nx = (y2 - y) / 10, ny = -(x2 - x) / 10;      /* across the flight, ~17 px each side */
    for (k = -2; k <= 2; k++)
        gfx_line(cx - nx + k, cy - ny, cx + (x2 - x) / 14 + k, cy + (y2 - y) / 14, k ? e->color : C_WHITE);
    gfx_line(cx, cy, cx + nx, cy + ny, e->color);
    gfx_line(x + (x2 - x) * p / 80, y + (y2 - y) * p / 80, cx, cy, RGB565(160, 150, 140));
}

/* The singularity: arms spiralling in, a dark core that grows. */
static void vortex(const Effect *e, int x, int y)
{
    int k, r = e->r * (e->dur - e->t) / MAX(1, e->dur) + 4;
    for (k = 0; k < 6; k++) {
        int a = (k * 11 + e->t * 5) & 63;
        gfx_line(fx_cx(x, r, a), fx_cy(y, r, a), fx_cx(x, r / 3, a + 10), fx_cy(y, r / 3, a + 10), e->color);
    }
    gfx_fill_rect(x - 3, y - 3, 7, 7, 0);
    fx_ellipse(x, y, 5, e->color);
}

/* The reaper's scythe sweeping over the fallen. */
static void reap(const Effect *e, int x, int y)
{
    int k, a = 8 + progress(e, 40);
    for (k = 0; k < 3; k++)
        fx_arc(x, y - 6, 14 - k, a - 20, 20, k ? e->color : C_WHITE);
    gfx_line(x, y - 22, x, y + 4, RGB565(120, 90, 70));
}

/* Hit spark: a short star of lines that fades out. */
static void spark(const Effect *e, int x, int y)
{
    int r = e->r - e->t, k;
    if (r <= 0)
        return;
    for (k = 0; k < 4; k++) {
        int a = (k * 16 + fx_hash(e->x, e->y)) & 63;
        gfx_line(x, y, fx_cx(x, r, a), fx_cy(y, r, a) - r / 3, k & 1 ? e->color : C_WHITE);
    }
}

/* Gibs: chunks thrown up and out, landing in a splatter. */
static void gibs(const Effect *e, int x, int y)
{
    int k, t = e->t;
    for (k = 0; k < 7; k++) {
        int a = fx_hash(k, e->x) & 63, d = e->r * MIN(t, 8) / 8 + (k & 3);
        int lift = t < 8 ? (8 - t) * (k % 3 + 1) / 2 : 0;
        gfx_fill_rect(fx_cx(x, d, a), fx_cy(y, d, a) - lift, k & 1 ? 2 : 3, 2, e->color);
    }
    if (t >= 6)
        fx_ellipse(x, y + 1, e->r / 3 + 2, e->color);
}

void fx_draw_myth(const Effect *e, int x, int y, int x2, int y2)
{
    switch (e->kind) {
    case FX_STAR:     star(e, x, y); break;
    case FX_CRESCENT: crescent(e, x, y, x2, y2); break;
    case FX_VORTEX:   vortex(e, x, y); break;
    case FX_REAP:     reap(e, x, y); break;
    case FX_SPARK:    spark(e, x, y); break;
    case FX_GIB:      gibs(e, x, y); break;
    default:          break;
    }
}

/* ---------------------------------------------------------------- aura */

static void blades(const World *w, int x, int y)
{
    int k;
    for (k = 0; k < 4; k++) {
        int a = (w->myth.blade_a + k * 16) & 63, bx = fx_cx(x, 26, a), by = fx_cy(y, 26, a) - 4;
        gfx_line(bx - 3, by + 2, bx + 3, by - 2, C_WHITE);
        gfx_line(bx - 2, by + 2, bx + 2, by - 2, RGB565(160, 200, 255));
    }
}

void render_myth_aura(const World *w, int x, int y)
{
    int k;
    if (!w->st.b.myth_any)
        return;
    for (k = 0; k < 3; k++) {                        /* violet motes rising around the hero */
        int a = fx_hash(k, w->tick / 4) & 63;
        gfx_pixel(fx_cx(x, 10, a), fx_cy(y, 10, a) - (w->tick + k * 7) % 18, k ? C_VIOLET : C_STAR);
    }
    if (w->st.b.myth[MY_BLADES] > 0)
        blades(w, x, y);
    if (w->myth.streak > 0)
        fx_ellipse(x, y + 2, 8 + w->myth.streak / 25, RGB565(230, 40, 50));
}

void render_myth_tint(const World *w, int view_h)
{
    if (w->myth.stop_t > 0)
        gfx_blend_rect(0, 0, SCREEN_W, view_h, C_TIME);
}
