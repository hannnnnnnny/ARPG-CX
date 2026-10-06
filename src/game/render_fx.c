#include "render_fx.h"
#include "skills.h"
#include "../gfx/gfx.h"
#include "../gfx/font.h"
#include "../core/trig.h"

#define C_WHITE  RGB565(255, 250, 235)
#define C_FLAME  RGB565(255, 120, 30)
#define C_FLAME2 RGB565(255, 210, 80)
#define C_ROCK   RGB565(120, 90, 60)
#define C_DUST   RGB565(170, 150, 120)
#define C_BONE   RGB565(235, 228, 205)
#define C_BLOOD  RGB565(200, 30, 40)
#define C_GHOST  RGB565(120, 255, 200)

static const uint8_t class_style[CLASS_COUNT][CLASS_SKILLS] = {
    { VS_FLAY, VS_SLASH, VS_WHIRL, VS_HAMMER, VS_CLAW, VS_WARCRY, VS_BARRIER, VS_LEAP, VS_QUAKE, VS_ULT },
    { VS_SPARK, VS_FROSTBOLT, VS_FIREBALL, VS_SPARK, VS_SHARD, VS_FROSTNOVA, VS_BARRIER, VS_METEOR, VS_STORM,
      VS_TEMPEST },
    { VS_DAGGER, VS_ARROW, VS_POWERSHOT, VS_ARROW, VS_ARROW, VS_TRAP, VS_SHADOWDASH, VS_IMBUE, VS_IMBUE, VS_RAIN },
    { VS_SCYTHE, VS_BONE, VS_BONESPEAR, VS_BLIGHT, VS_BLOODSURGE, VS_PRISON, VS_MIST, VS_RAISE, VS_CORPSE,
      VS_LEGION },
    { VS_SPARK, VS_MAUL, VS_TORNADO, VS_LANDSLIDE, VS_CLAW, VS_STONE, VS_CYCLONE, VS_RAISE, VS_BOULDER, VS_TEMPEST },
    { VS_THRUST, VS_FIST, VS_QUILL, VS_PALM, VS_STINGER, VS_CENTIPEDE, VS_BARRIER, VS_SOAR, VS_JAGUAR, VS_ULT },
};

int fx_style_of(int vfx)
{
    if (vfx < CLASS_COUNT * CLASS_SKILLS)
        return class_style[vfx / CLASS_SKILLS][vfx % CLASS_SKILLS];
    switch (vfx) {
    case VX_BURN:        return VS_BURN;
    case VX_POISON_POOL: return VS_POOL;
    case VX_QUAKE_POOL:  return VS_QUAKE;
    case VX_CORPSE_BOOM: return VS_CORPSE;
    default:             return VS_DEFAULT;
    }
}

/* ------------------------------------------------------------- helpers */

/* Cheap stable hash so "random" sparks don't jitter between frames. */
int fx_hash(int a, int b)
{
    uint32_t h = (uint32_t)a * 2654435761u ^ (uint32_t)b * 40503u;
    h ^= h >> 13;
    return (int)(h & 0x7FFF);
}

/* Point on a circle; phase in 1/64 turns, y squashed for the 3/4 view. */
int fx_cx(int x, int r, int phase) { return x + isin(phase + 16) * r / 127; }
int fx_cy(int y, int r, int phase) { return y + isin(phase) * r / 200; }

void fx_arc(int x, int y, int r, int from, int span, uint16_t c)
{
    int a;
    for (a = from; a <= from + span; a++)
        gfx_pixel(fx_cx(x, r, a), fx_cy(y, r, a), c);
}

void fx_ellipse(int x, int y, int r, uint16_t c)
{
    fx_arc(x, y, r, 0, 63, c);
}

void fx_jagged(int x0, int y0, int x1, int y1, int seed, uint16_t c)
{
    int k, px = x0, py = y0;
    for (k = 1; k <= 4; k++) {
        int nx = x0 + (x1 - x0) * k / 4 + (k < 4 ? (fx_hash(seed, k) % 9) - 4 : 0);
        int ny = y0 + (y1 - y0) * k / 4 + (k < 4 ? (fx_hash(seed, k + 7) % 7) - 3 : 0);
        gfx_line(px, py, nx, ny, c);
        px = nx;
        py = ny;
    }
}

static int grow(const Effect *e) { return e->r * (e->t + 2) / (e->dur + 2); }

/* Scattered particles on a ring (debris, sparks, splinters). */
static void scatter(int x, int y, int r, int n, int seed, int lift, uint16_t c1, uint16_t c2)
{
    int k;
    for (k = 0; k < n; k++) {
        int a = fx_hash(k, seed) & 63, d = r + (fx_hash(k, seed + 1) % 6);
        gfx_fill_rect(fx_cx(x, d, a), fx_cy(y, d, a) - lift, 2, 2, k & 1 ? c1 : c2);
    }
}

/* ------------------------------------------------------- melee swings */

static void claw_marks(int x, int y, int f, int t, uint16_t c)
{
    int k, len = MIN(t * 3, 14);
    for (k = 0; k < 3; k++) {
        int sx = x + f * (4 + k * 4), sy = y - 8 + k;
        gfx_line(sx, sy, sx + f * len / 2, sy + len, c);
        gfx_line(sx + f, sy, sx + f + f * len / 2, sy + len, C_WHITE);
    }
}

static void swing_special(const Effect *e, int x, int y, int st, int f)
{
    int k;
    switch (st) {
    case VS_HAMMER:                        /* hammer head comes down, shock ring */
        gfx_fill_rect(x + f * 10 - 4, y - 14 + MIN(e->t * 4, 12), 8, 5, RGB565(150, 150, 165));
        gfx_rect(x + f * 10 - 4, y - 14 + MIN(e->t * 4, 12), 8, 5, e->color);
        fx_ellipse(x + f * 10, y + 2, e->t * 3, C_DUST);
        scatter(x + f * 10, y + 2, e->t * 2, 6, e->x, e->t / 2, C_ROCK, C_DUST);
        break;
    case VS_SCYTHE:                        /* a wide shadow scythe sweep */
        fx_arc(x, y - 2, e->r, (f > 0 ? 48 : 16) + e->t * 4, 26, e->color);
        fx_arc(x, y - 2, e->r - 3, (f > 0 ? 48 : 16) + e->t * 4, 22, C_WHITE);
        gfx_line(x, y - 4, fx_cx(x, e->r, (f > 0 ? 48 : 16) + e->t * 4 + 26),
                 fx_cy(y - 4, e->r, (f > 0 ? 48 : 16) + e->t * 4 + 26), RGB565(110, 70, 150));
        break;
    case VS_THRUST:                        /* a glaive thrust straight ahead */
        gfx_line(x + f * 4, y - 3, x + f * (8 + e->t * 3), y - 3, C_WHITE);
        gfx_line(x + f * 4, y - 2, x + f * (8 + e->t * 3), y - 2, e->color);
        gfx_pixel(x + f * (10 + e->t * 3), y - 3, C_FLAME2);
        break;
    case VS_FIST:                          /* a venomous punch with spatter */
        gfx_fill_rect(x + f * 10 - 3, y - 6, 6, 5, e->color);
        scatter(x + f * 12, y - 4, e->t * 2, 6, e->y, 0, e->color, RGB565(200, 255, 160));
        break;
    case VS_MAUL:                          /* a huge bear paw swipe */
        for (k = 0; k < 4; k++)
            fx_arc(x + f * 4, y, e->r - k * 3, (f > 0 ? 52 : 12) + e->t * 3, 14, k & 1 ? C_WHITE : C_ROCK);
        break;
    default:
        claw_marks(x, y, f, e->t, st == VS_FLAY ? C_BLOOD : e->color);
        break;
    }
}

static void fx_slash(const Effect *e, int x, int y)
{
    int st = fx_style_of(e->vfx), f = e->x2 >= 0 ? 1 : -1, k;
    if (st == VS_SLASH || st == VS_DEFAULT || st == VS_FLAY) {
        int sweep = e->t * 3, face = f > 0 ? 0 : 32;
        for (k = 0; k < 3; k++)            /* three sweeping crescents */
            fx_arc(x, y, e->r - k * 4, face - 14 + sweep - k * 2, 18, k == 0 ? C_WHITE : e->color);
        if (st == VS_FLAY)
            scatter(x + f * 10, y, 6 + e->t, 5, e->x, e->t / 2, C_BLOOD, RGB565(255, 80, 80));
        return;
    }
    swing_special(e, x, y, st, f);
}

/* --------------------------------------------------------------- novas */

static void fx_nova(const Effect *e, int x, int y)
{
    int r = grow(e), st = fx_style_of(e->vfx), k;
    switch (st) {
    case VS_FROSTNOVA:                     /* radiating ice spikes */
        fx_ellipse(x, y, r, e->color);
        for (k = 0; k < 64; k += 8)
            gfx_line(fx_cx(x, r, k), fx_cy(y, r, k), fx_cx(x, r + 6, k), fx_cy(y, r + 6, k), C_WHITE);
        break;
    case VS_PRISON:                        /* bone spikes erupt in a ring */
        for (k = 0; k < 64; k += 5) {
            int px = fx_cx(x, e->r, k), py = fx_cy(y, e->r, k), h = MIN(e->t, 7);
            gfx_vline(px, py - h, h, C_BONE);
            gfx_pixel(px, py - h - 1, C_WHITE);
        }
        break;
    case VS_BLOODSURGE:                    /* blood drawn inward, then a red wave */
        for (k = 0; k < 64; k += 6) {
            int d = e->r - e->t * 3;
            if (d > 4)
                gfx_fill_rect(fx_cx(x, d, k), fx_cy(y, d, k), 2, 2, C_BLOOD);
        }
        fx_ellipse(x, y, r, C_BLOOD);
        fx_ellipse(x, y, r - 2, RGB565(255, 90, 90));
        break;
    case VS_CYCLONE:                       /* swirling wind gusts */
        for (k = 0; k < 4; k++)
            fx_arc(x, y, r - k * 4, e->t * 6 + k * 16, 12, k & 1 ? C_WHITE : e->color);
        break;
    default:
        fx_ellipse(x, y, r, e->color);
        fx_ellipse(x, y, r - 2, C_WHITE);
        break;
    }
}

/* ---------------------------------------------------------- explosions */

static void fx_boom(const Effect *e, int x, int y)
{
    int r = grow(e), st = fx_style_of(e->vfx), k;
    switch (st) {
    case VS_FIREBALL:
        fx_ellipse(x, y, r, C_FLAME);
        fx_ellipse(x, y, r / 2, C_FLAME2);
        scatter(x, y, r, 10, e->x, e->t / 2, C_FLAME, C_FLAME2);
        break;
    case VS_BONESPEAR:
    case VS_BONE:                          /* the spear shatters into splinters */
        for (k = 0; k < 8; k++) {
            int a = k * 8 + (e->x & 7);
            gfx_line(fx_cx(x, r / 2, a), fx_cy(y, r / 2, a), fx_cx(x, r, a), fx_cy(y, r, a), C_BONE);
        }
        break;
    case VS_CORPSE:                        /* gore burst with a dark core */
        fx_ellipse(x, y, r, e->color);
        fx_ellipse(x, y, r * 2 / 3, RGB565(120, 200, 90));
        scatter(x, y, r, 12, e->y, e->t, C_BLOOD, RGB565(140, 220, 100));
        break;
    default:
        fx_ellipse(x, y, r, e->color);
        fx_ellipse(x, y, r / 2, C_WHITE);
        break;
    }
}

static void fx_whirl(const Effect *e, int x, int y)
{
    int k, base = e->t * 9 + e->x * 3;
    for (k = 0; k < 4; k++) {              /* four spinning blades */
        int a = base + k * 16;
        gfx_line(fx_cx(x, 6, a), fx_cy(y, 6, a), fx_cx(x, e->r, a), fx_cy(y, e->r, a), k & 1 ? C_WHITE : e->color);
        gfx_pixel(fx_cx(x, e->r + 1, a + 1), fx_cy(y, e->r + 1, a + 1), e->color);
    }
    fx_arc(x, y, e->r - 2, base, 20, e->color);
}

/* ------------------------------------------------- strikes from above */

static void falling_rock(int x, int y, int left, uint16_t c)
{
    int rx = x + left * 4, ry = y - left * 6, k;
    for (k = 1; k < 6; k++)
        gfx_fill_rect(rx + k * 3, ry - k * 4, 2, 2, k & 1 ? C_FLAME : C_FLAME2);
    gfx_fill_rect(rx - 3, ry - 3, 7, 7, C_ROCK);
    gfx_fill_rect(rx - 2, ry - 2, 3, 3, c);
}

static void airborne(const Effect *e, int x, int y, bool wings)
{
    int p = e->t * 256 / MAX(e->dur, 1);
    int hx = e->x2 + (x - e->x2) * p / 256, hy = e->y2 + (y - e->y2) * p / 256 - isin(p / 8) * 30 / 127;
    gfx_fill_rect(hx - 3, hy - 6, 6, 9, RGB565(150, 150, 170));
    gfx_rect(hx - 3, hy - 6, 6, 9, e->color);
    if (wings) {                           /* spirit eagle wings */
        gfx_line(hx - 3, hy - 3, hx - 10, hy - 8 + (e->t & 2) * 2, C_WHITE);
        gfx_line(hx + 3, hy - 3, hx + 10, hy - 8 + (e->t & 2) * 2, C_WHITE);
    }
    fx_ellipse(x, y, e->r * e->t / MAX(e->dur, 1), C_DUST);
}

static void fx_warn(const Effect *e, int x, int y)
{
    int left = e->dur - e->t, st = fx_style_of(e->vfx);
    switch (st) {
    case VS_METEOR: falling_rock(x, y, left, e->color); break;
    case VS_LEAP:   airborne(e, x, y, false); return;
    case VS_SOAR:   airborne(e, x, y, true); return;
    case VS_PALM:                          /* a spirit hand descends */
        gfx_fill_rect(x - 6, y - 10 - left * 3, 12, 8, e->color);
        gfx_vline(x - 5, y - 14 - left * 3, 4, e->color);
        gfx_vline(x - 1, y - 15 - left * 3, 5, e->color);
        gfx_vline(x + 3, y - 14 - left * 3, 4, e->color);
        break;
    case VS_LANDSLIDE:                     /* the ground bulges */
        scatter(x, y, e->r / 2, 6, e->x + e->t, 0, C_DUST, C_ROCK);
        break;
    default: break;
    }
    if ((e->t >> 1) & 1)
        fx_ellipse(x, y, e->r, e->color);
}

static void fx_impact(const Effect *e, int x, int y)
{
    int r = grow(e), st = fx_style_of(e->vfx), k;
    if (st == VS_LEAP || st == VS_SOAR || st == VS_PALM) {   /* crater and flying rocks */
        fx_ellipse(x, y, r, st == VS_PALM ? e->color : C_DUST);
        scatter(x, y, r * 2 / 3 + e->t, 8, 3, e->t / 3, C_ROCK, e->color);
        return;
    }
    if (st == VS_LANDSLIDE) {              /* stone pillars burst out of the floor */
        for (k = 0; k < 5; k++) {
            int px = x + (fx_hash(k, e->x) % (e->r + 1)) - e->r / 2, py = y + (fx_hash(k, e->y) % 9) - 4;
            int h = MIN(e->t * 3, 14) - (e->t > 12 ? (e->t - 12) * 2 : 0);
            if (h > 0) {
                gfx_fill_rect(px - 2, py - h, 5, h, C_ROCK);
                gfx_vline(px - 2, py - h, h, C_DUST);
            }
        }
        return;
    }
    /* meteor and friends: crater ring, fire column and sparks */
    fx_ellipse(x, y, r, e->color);
    fx_ellipse(x, y, r * 2 / 3, C_FLAME2);
    if (e->t < 6)
        gfx_fill_rect(x - 3, y - 30 + e->t * 5, 6, 30 - e->t * 5, C_FLAME2);
    scatter(x, y, r + e->t, 12, 3, e->t, C_FLAME, e->color);
}

/* ----------------------------------------------------------------- buffs */

static void buff_rings(const Effect *e, int x, int y, int st)
{
    int k, r = 6 + e->t;
    switch (st) {
    case VS_WARCRY:                        /* shout: jagged expanding rings */
        for (k = 0; k < 64; k += 4) {
            int jr = r + ((k >> 2) & 1) * 3;
            gfx_pixel(fx_cx(x, jr, k), fx_cy(y, jr, k), e->color);
            gfx_pixel(fx_cx(x, jr + 6, k + 2), fx_cy(y, jr + 6, k + 2), C_WHITE);
        }
        break;
    case VS_BARRIER:
    case VS_STONE:                         /* a shimmering dome / ring of stones */
        fx_arc(x, y - 6, 12, 32, 32, st == VS_STONE ? C_ROCK : e->color);
        fx_arc(x, y - 6, 13, 32 + e->t, 8, C_WHITE);
        if (st == VS_STONE)
            scatter(x, y, 12, 8, 7, 0, C_ROCK, C_DUST);
        break;
    case VS_IMBUE:                         /* bubbles of the imbued element */
        scatter(x, y - e->t, 6, 8, e->t / 3, 0, e->color, C_WHITE);
        break;
    case VS_MIST:                          /* the hero dissolves into red mist */
        scatter(x, y - 4, 4 + e->t / 2, 12, e->t / 2, e->t / 3, C_BLOOD, RGB565(255, 120, 120));
        break;
    case VS_JAGUAR:                        /* golden claw marks around the hero */
        claw_marks(x - 8, y, 1, e->t, RGB565(255, 200, 80));
        break;
    case VS_ULT:                           /* a pillar of light and a ground ring */
        gfx_fill_rect(x - 2, y - 40 + e->t, 4, 40 - e->t, e->color);
        gfx_vline(x, y - 40 + e->t, 40 - e->t, C_WHITE);
        fx_ellipse(x, y, r + 6, e->color);
        break;
    default:
        gfx_circle(x, y - e->t, MAX(1, e->r - e->t / 3), e->color);
        break;
    }
}

static void fx_dash(const Effect *e, int x, int y, int x2, int y2)
{
    int k, st = fx_style_of(e->vfx);
    uint16_t c = st == VS_STINGER ? RGB565(130, 230, 80) : RGB565(160, 90, 230);
    for (k = 0; k < 4; k++) {              /* after-images along the path */
        int px = x + (x2 - x) * k / 4, py = y + (y2 - y) * k / 4;
        if (k * 3 >= e->t)
            gfx_rect(px - 3, py - 8, 6, 10, c);
    }
    gfx_line(x, y - 3, x2, y2 - 3, c);
    gfx_line(x, y - 2, x2, y2 - 2, C_WHITE);
}

static void fx_raise(const Effect *e, int x, int y)
{
    int k;
    for (k = 0; k < 6; k++)                /* spectral sparks rising from the ground */
        gfx_pixel(x - 6 + k * 2 + (fx_hash(k, e->t / 3) & 1), y - (e->t + k * 3) % 16, e->color);
    fx_ellipse(x, y + 2, 7, e->color);
}

void fx_draw_effect(const Effect *e, int cam_x, int cam_y)
{
    int x = e->x - cam_x, y = e->y - cam_y;
    switch (e->kind) {
    case FX_SLASH:  fx_slash(e, x, y); break;
    case FX_NOVA:   fx_nova(e, x, y); break;
    case FX_BOOM:   fx_boom(e, x, y); break;
    case FX_WHIRL:  fx_whirl(e, x, y); break;
    case FX_WARN:   fx_warn(e, x, y); break;
    case FX_METEOR: fx_impact(e, x, y); break;
    case FX_BOLT:   /* lightning: re-forked every frame, with a side branch */
        fx_jagged(x, y, e->x2 - cam_x, e->y2 - cam_y, e->t * 31 + e->x, e->color);
        fx_jagged(x, y - 1, e->x2 - cam_x, e->y2 - cam_y - 1, e->t * 17 + e->y, C_WHITE);
        gfx_line((x + e->x2 - cam_x) / 2, (y + e->y2 - cam_y) / 2, (x + e->x2 - cam_x) / 2 + 5,
                 (y + e->y2 - cam_y) / 2 - 6, e->color);
        break;
    case FX_DASH:   fx_dash(e, x, y, e->x2 - cam_x, e->y2 - cam_y); break;
    case FX_RAISE:  fx_raise(e, x, y); break;
    case FX_PUFF:   gfx_circle(x, y, 3 + e->t, e->color); break;
    case FX_HEAL:
    case FX_LEVEL:  buff_rings(e, x, y, fx_style_of(e->vfx)); break;
    default:        fx_draw_sig(e, x, y); break;
    }
}

/* -------------------------------------------------------------- ground */

static void ground_quake(const Ground *g, int x, int y)
{
    int k;
    for (k = 0; k < 7; k++) {              /* radial cracks */
        int a = (fx_hash(k, g->x) & 63);
        fx_jagged(x, y, fx_cx(x, g->r, a), fx_cy(y, g->r, a), k * 13 + g->x, k & 1 ? g->color : C_ROCK);
    }
    for (k = 0; k < 4; k++) {              /* debris jumping up */
        int a = fx_hash(k, g->t / 6) & 63, d = fx_hash(k + 9, g->t / 6) % (g->r + 1);
        gfx_fill_rect(fx_cx(x, d, a), fx_cy(y, d, a) - (g->t % 6), 2, 2, C_DUST);
    }
}

static void bolt_from_sky(const Ground *g, int x, int y, uint16_t c)
{
    int a = fx_hash(g->t / 15, g->x) & 63, d = fx_hash(g->t / 15, g->y) % (g->r + 1);
    int tx = fx_cx(x, d, a), ty = fx_cy(y, d, a);
    fx_jagged(tx + 4, ty - 70, tx, ty, g->t, c);
    fx_jagged(tx + 5, ty - 70, tx + 1, ty, g->t + 3, C_WHITE);
    fx_ellipse(tx, ty, 4, C_WHITE);
}

static void ground_storm(const Ground *g, int x, int y, bool tempest)
{
    int k;
    fx_ellipse(x, y, g->r, RGB565(70, 70, 120));
    if ((g->t % 15) < 4)                   /* a strike on every damage pulse */
        bolt_from_sky(g, x, y, g->color);
    if (tempest)                           /* a wide swirl of wind around it */
        for (k = 0; k < 3; k++)
            fx_arc(x, y, g->r - k * 8, g->t * 3 + k * 20, 14, k & 1 ? C_WHITE : g->color);
    for (k = 0; k < 6; k++)
        gfx_pixel(fx_cx(x, fx_hash(k, g->t) % (g->r + 1), fx_hash(k, g->t + 1) & 63),
                  fx_cy(y, fx_hash(k, g->t) % (g->r + 1), fx_hash(k, g->t + 1) & 63), g->color);
}

static void ground_bubbles(const Ground *g, int x, int y, uint16_t ring)
{
    int k;
    for (k = 0; k < 14; k++) {             /* bubbles rising and popping */
        int a = fx_hash(k, g->x) & 63, d = fx_hash(k, g->y) % (g->r + 1), life = (g->t + k * 5) % 24;
        int bx = fx_cx(x, d, a), by = fx_cy(y, d, a) - life / 2;
        gfx_circle(bx, by, 1 + life / 10, life < 20 ? g->color : C_WHITE);
    }
    fx_arc(x, y, g->r, g->t, 48, ring);
}

static void ground_trap(const Ground *g, int x, int y)
{
    int k;
    for (k = 0; k < 9; k++) {              /* spikes popping in and out */
        int a = fx_hash(k, g->x) & 63, d = fx_hash(k, g->y) % (g->r + 1);
        int sx = fx_cx(x, d, a), sy = fx_cy(y, d, a), h = ((g->t + k * 4) % 16) < 8 ? 5 : 2;
        gfx_line(sx - 2, sy, sx, sy - h, g->color);
        gfx_line(sx, sy - h, sx + 2, sy, g->color);
    }
    fx_ellipse(x, y, g->r, RGB565(90, 80, 70));
}

static void ground_rain(const Ground *g, int x, int y)
{
    int k;
    for (k = 0; k < 10; k++) {             /* arrows falling */
        int a = fx_hash(k, g->x) & 63, d = fx_hash(k, g->y) % (g->r + 1), fall = (g->t * 6 + k * 23) % 40;
        int ax = fx_cx(x, d, a), ay = fx_cy(y, d, a) - 40 + fall;
        gfx_vline(ax, ay - 5, 5, g->color);
        gfx_pixel(ax - 1, ay - 5, RGB565(200, 200, 200));
        gfx_pixel(ax + 1, ay - 5, RGB565(200, 200, 200));
    }
    fx_ellipse(x, y, g->r, RGB565(90, 90, 90));
}

static void ground_flames(const Ground *g, int x, int y)
{
    int k;
    for (k = 0; k < 12; k++) {             /* flickering flames */
        int a = fx_hash(k, g->x) & 63, d = fx_hash(k, g->y) % (g->r + 1);
        int fx = fx_cx(x, d, a), fy = fx_cy(y, d, a), h = 2 + (fx_hash(k, g->t / 3) % 4);
        gfx_vline(fx, fy - h, h, (k + g->t / 4) & 1 ? C_FLAME : C_FLAME2);
    }
}

static void ground_legion(const Ground *g, int x, int y)
{
    int k;
    for (k = 0; k < 7; k++) {              /* skeletal hands claw out of the floor */
        int a = fx_hash(k, g->x) & 63, d = fx_hash(k, g->y) % (g->r + 1), up = (g->t + k * 7) % 20;
        int hx = fx_cx(x, d, a), hy = fx_cy(y, d, a), h = up < 10 ? up / 2 : (20 - up) / 2;
        gfx_vline(hx, hy - h, h, C_BONE);
        gfx_pixel(hx - 1, hy - h, C_BONE);
        gfx_pixel(hx + 1, hy - h, C_BONE);
    }
    fx_ellipse(x, y, g->r, RGB565(90, 80, 70));
}

static void ground_centipede(const Ground *g, int x, int y)
{
    int k, s;
    for (k = 0; k < 4; k++) {              /* centipedes crawling in circles */
        int a = g->t / 2 + k * 16, d = g->r * (k + 2) / 6;
        for (s = 0; s < 5; s++)
            gfx_fill_rect(fx_cx(x, d, a - s * 2), fx_cy(y, d, a - s * 2), 2, 2, s ? g->color : C_FLAME2);
    }
    fx_arc(x, y, g->r, g->t, 40, RGB565(60, 120, 40));
}

void fx_draw_ground(const Ground *g, int cam_x, int cam_y)
{
    int x = FX_TO_INT(g->x) - cam_x, y = FX_TO_INT(g->y) - cam_y, st = fx_style_of(g->vfx);
    switch (st) {
    case VS_QUAKE:     ground_quake(g, x, y); break;
    case VS_STORM:     ground_storm(g, x, y, false); break;
    case VS_TEMPEST:   ground_storm(g, x, y, true); break;
    case VS_POOL:      ground_bubbles(g, x, y, RGB565(60, 120, 40)); break;
    case VS_BLIGHT:    ground_bubbles(g, x, y, RGB565(90, 40, 120)); break;
    case VS_TRAP:      ground_trap(g, x, y); break;
    case VS_RAIN:      ground_rain(g, x, y); break;
    case VS_LEGION:    ground_legion(g, x, y); break;
    case VS_CENTIPEDE: ground_centipede(g, x, y); break;
    default:           ground_flames(g, x, y); break;
    }
    /* Element changed by an upgrade (fire trap, inferno...): keep it readable. */
    if (st != VS_BURN && g->hit.element == EL_FIRE && st != VS_STORM)
        ground_flames(g, x, y);
}
