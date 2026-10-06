/*
 * render_sig.c - the look of the signature builds: bolts from the sky,
 * bursts of bone, rings of fire, falling meteors, the hero's aura and the
 * giant bone spear. A few dozen line / pixel draws each, like the rest of
 * render_fx.c, so the calculator keeps its frame rate.
 */
#include "render_fx.h"
#include "world_int.h"
#include "../gfx/gfx.h"

#define C_WHITE  RGB565(255, 250, 235)
#define C_STORM  RGB565(140, 220, 255)
#define C_STORM2 RGB565(60, 120, 255)
#define C_BONE   RGB565(240, 232, 210)
#define C_BONE2  RGB565(170, 160, 140)
#define C_FIRE   RGB565(255, 140, 40)
#define C_FIRE2  RGB565(255, 220, 90)

static int progress(const Effect *e, int full)
{
    return e->dur > 0 ? full * MIN(e->t, e->dur) / e->dur : full;
}

/* A bolt out of the sky: a light column, a forked core and a splash. */
static void skybolt(const Effect *e, int x, int y)
{
    int top = y - 110;
    if (e->t < 4)
        gfx_blend_rect(x - 4, top, 9, y - top, e->color);
    if (e->t < 8) {
        fx_jagged(x + 6, top, x, y, e->t * 7 + e->x, e->color);
        fx_jagged(x + 5, top, x - 1, y, e->t * 11 + e->y, C_WHITE);
        fx_jagged(x, y - 40, x - 14, y - 22, e->t + e->x, C_STORM2);
    }
    fx_ellipse(x, y, 4 + e->t, e->t < 6 ? C_WHITE : e->color);
    if (e->t < 6) {
        gfx_line(x - 8, y, x + 8, y, C_WHITE);
        gfx_line(x, y - 6, x, y + 4, C_WHITE);
    }
}

/* Bone shards flying out of a nova, with the ring they leave behind. */
static void shards(const Effect *e, int x, int y)
{
    int r = progress(e, e->r), k;
    for (k = 0; k < 12; k++) {
        int a = k * 64 / 12 + (fx_hash(k, e->x) & 3);
        int x0 = fx_cx(x, r / 2, a), y0 = fx_cy(y, r / 2, a), x1 = fx_cx(x, r, a), y1 = fx_cy(y, r, a);
        gfx_line(x0, y0, x1, y1, k & 1 ? C_BONE : C_BONE2);
        gfx_pixel(x1, y1, C_WHITE);
    }
    fx_ellipse(x, y, r, C_BONE);
    if (e->t < 3)
        fx_ellipse(x, y, r / 3 + 2, C_WHITE);
}

/* The second explosion of a fireball: a ring of fire and flying embers. */
static void firering(const Effect *e, int x, int y)
{
    int r = progress(e, e->r), k;
    fx_ellipse(x, y, r, C_FIRE);
    fx_ellipse(x, y, r * 4 / 5, C_FIRE2);
    if (e->t < 4)
        gfx_blend_rect(x - r / 2, y - r / 4, r, r / 2, C_FIRE);
    for (k = 0; k < 10; k++) {
        int a = fx_hash(k, e->x + e->t) & 63;
        gfx_pixel(fx_cx(x, r + (k & 3), a), fx_cy(y, r + (k & 3), a) - e->t / 2, k & 1 ? C_FIRE2 : C_FIRE);
    }
}

/* A meteor in its last moments: a burning streak down to the warning ring. */
static void fall(const Effect *e, int x, int y)
{
    int left = e->dur - e->t, sx = x + left * 6, sy = y - left * 16;
    gfx_line(sx + 10, sy - 24, sx, sy, C_FIRE);
    gfx_line(sx + 11, sy - 24, sx + 1, sy, C_FIRE2);
    gfx_fill_rect(sx - 3, sy - 3, 7, 7, C_FIRE2);
    gfx_fill_rect(sx - 2, sy - 2, 5, 5, C_WHITE);
}

void fx_draw_sig(const Effect *e, int x, int y)
{
    switch (e->kind) {
    case FX_SKYBOLT:  skybolt(e, x, y); break;
    case FX_SHARDS:   shards(e, x, y); break;
    case FX_FIRERING: firering(e, x, y); break;
    case FX_FALL:     fall(e, x, y); break;
    default:          break;
    }
}

/* ---------------------------------------------------------------- aura */

static void storm_aura(const World *w, int x, int y)
{
    int k, t = w->tick;
    for (k = 0; k < 3; k++) {
        int a = (t * 3 + k * 21) & 63;
        gfx_pixel(fx_cx(x, 11, a), fx_cy(y, 11, a) - 4, C_STORM);
        gfx_pixel(fx_cx(x, 9, a + 3), fx_cy(y, 9, a + 3) - 5, C_WHITE);
    }
    if ((t & 15) < 3)
        fx_jagged(x - 7, y - 14, x + 6, y + 2, t, C_STORM);
    gfx_pixel(x - 2 * w->h.face, y - 9, C_STORM);   /* glowing eyes of the wolf */
}

static void bone_aura(const World *w, int x, int y)
{
    int k;
    for (k = 0; k < 3; k++) {
        int a = (w->tick * 2 + k * 21) & 63, px = fx_cx(x, 12, a), py = fx_cy(y, 12, a) - 4;
        gfx_line(px - 2, py + 1, px + 2, py - 1, C_BONE);
    }
}

static void fire_aura(const World *w, int x, int y)
{
    int k;
    fx_ellipse(x, y + 2, 9 + ((w->tick >> 2) & 1), C_FIRE);
    for (k = 0; k < 4; k++) {
        int a = fx_hash(k, w->tick / 3) & 63;
        gfx_pixel(fx_cx(x, 8, a), fx_cy(y, 8, a) - (w->tick + k * 5) % 14, k & 1 ? C_FIRE2 : C_FIRE);
    }
}

void render_sig_aura(const World *w, int x, int y)
{
    switch (w->st.b.sig) {
    case SIG_STORMWOLF: storm_aura(w, x, y); break;
    case SIG_BONESPEAR: bone_aura(w, x, y); break;
    case SIG_INFERNO:   fire_aura(w, x, y); break;
    default:            break;
    }
}

/* The giant bone spear: a long lance with a glowing tip and a bone trail. */
void render_sig_giant(const Proj *pj, int x, int y)
{
    int dx = pj->vx > 0 ? 1 : pj->vx < 0 ? -1 : 0, dy = pj->vy > 0 ? 1 : pj->vy < 0 ? -1 : 0, k;
    for (k = 0; k < 3; k++)
        gfx_line(x - dx * 26, y - dy * 26 + k - 1, x + dx * 6, y + dy * 6 + k - 1, k == 1 ? C_WHITE : C_BONE);
    gfx_fill_rect(x + dx * 6 - 2, y + dy * 6 - 2, 5, 5, C_WHITE);
    for (k = 0; k < 4; k++)
        gfx_pixel(x - dx * (30 + k * 5), y - dy * (30 + k * 5) + (k & 1), C_BONE2);
}

/* Screen shake offset for this frame; flash of light over the battle view. */
int render_sig_shake(const World *w, int axis)
{
    int px = w->sig.shake_px;
    if (w->sig.shake_t <= 0 || px <= 0)
        return 0;
    return (int)((unsigned)fx_hash(w->tick + axis * 7, w->sig.shake_t) % (unsigned)(2 * px + 1)) - px;
}

void render_sig_flash(const World *w, int view_h)
{
    if (w->sig.flash_t > 0)
        gfx_blend_rect(0, 0, SCREEN_W, view_h, w->sig.flash_color);
}
