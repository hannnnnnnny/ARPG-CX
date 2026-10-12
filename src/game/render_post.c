/*
 * render_post.c - desktop-only atmosphere on top of the battle view:
 * ambient particles for each depth theme and a soft glow (bloom) around
 * fire, lightning, lava and crit numbers.
 *
 * Both work on the 640x480 desktop framebuffer (GFX_HD). The calculator
 * build compiles them away, so its frame time is unchanged. Particles are
 * a pure function of the tick and the camera - no state, deterministic in
 * the headless runner like everything else.
 */
#include "render_fx.h"
#include "../gfx/gfx.h"

#ifdef GFX_HD

#define VIEW_H 200
#define TOP_ROWS 24              /* logical rows of status text that never feed the glow */
#define BW (FB_W / 4)            /* glow buffer: a quarter of the framebuffer per side */
#define BH (VIEW_H * GFX_S / 4)
#define PARTICLES 56

/* ------------------------------------------------------------ particles */

typedef struct { uint16_t col[2]; int8_t vx, vy; uint8_t sway, size; } Ambient;

/* Per theme: crypt dust, catacomb dust, hellforge embers, frost snow, void motes.
 * vx / vy in 1/16 logical px per tick; sway is a sideways wobble in px. */
static const Ambient amb[5] = {
    { { RGB565(196, 180, 150), RGB565(140, 128, 110) }, 1, -1, 3, 1 },
    { { RGB565(214, 190, 140), RGB565(150, 126, 90) }, 2, -1, 3, 1 },
    { { RGB565(255, 160, 56), RGB565(255, 214, 120) }, 1, -9, 4, 1 },
    { { RGB565(232, 242, 252), RGB565(176, 198, 222) }, -2, 6, 6, 2 },
    { { RGB565(206, 126, 255), RGB565(150, 92, 222) }, 0, -3, 7, 1 },
};

static int wrap(int v, int m) { return ((v % m) + m) % m; }

/* Blend 'c' over the framebuffer pixel at k/4 strength. */
static void blend_px(uint16_t *fb, int x, int y, uint16_t c, int k)
{
    uint16_t d;
    int r, g, b;
    if ((unsigned)x >= FB_W || (unsigned)y >= VIEW_H * GFX_S)
        return;
    d = fb[y * FB_W + x];
    r = ((d >> 11) * (4 - k) + (c >> 11) * k) / 4;
    g = (((d >> 5) & 63) * (4 - k) + ((c >> 5) & 63) * k) / 4;
    b = ((d & 31) * (4 - k) + (c & 31) * k) / 4;
    fb[y * FB_W + x] = (uint16_t)((r << 11) | (g << 5) | b);
}

void render_ambient(const World *w, int cam_x, int cam_y)
{
    const Ambient *a = &amb[((w->theme % 5) + 5) % 5];
    uint16_t *fb = gfx_target();
    int i;
    for (i = 0; i < PARTICLES; i++) {
        int hx = fx_hash(i, 911), hy = fx_hash(i, 4271), hz = fx_hash(i, 77);
        int t = w->tick, speed = 8 + (hz & 7);
        /* half-speed parallax against the camera reads as dust in the air */
        int x = wrap(hx + a->vx * t * speed / 128 - cam_x / 2 + a->sway * fx_cx(0, 16, t * 2 + hz) / 16, SCREEN_W);
        int y = wrap(hy + a->vy * t * speed / 128 - cam_y / 2, VIEW_H - TOP_ROWS) + TOP_ROWS;
        int wx = x + cam_x, wy = y + cam_y, k = 2 + (hz >> 3 & 1), dx, dy;
        if (wx < 0 || wy < 0 || wx >= MAP_W * TILE_SIZE || wy >= MAP_H * TILE_SIZE
            || !w->seen[wy / TILE_SIZE][wx / TILE_SIZE])
            continue;
        for (dy = 0; dy < a->size; dy++)
            for (dx = 0; dx < a->size; dx++)
                blend_px(fb, x * GFX_S + dx, y * GFX_S + dy, a->col[hz >> 4 & 1], k);
    }
}

/* ----------------------------------------------------------------- glow */

static uint16_t g_glow[3][BH][BW];   /* r, g, b in 8-bit units */

/* Bright, saturated pixels feed the glow; grey text and stone do not. */
static void gather(const uint16_t *fb)
{
    int bx, by, x, y;
    for (by = 0; by < BH; by++)
        for (bx = 0; bx < BW; bx++) {
            int r = 0, g = 0, b = 0;
            for (y = by * 4; y < by * 4 + 4; y++)
                for (x = bx * 4; x < bx * 4 + 4; x++) {
                    uint16_t c = fb[y * FB_W + x];
                    int cr = (c >> 11) * 8, cg = ((c >> 5) & 63) * 4, cb = (c & 31) * 8;
                    int hi = MAX(cr, MAX(cg, cb)), lo = MIN(cr, MIN(cg, cb));
                    if (y >= TOP_ROWS * GFX_S && hi > 160 && hi - lo > 80) {
                        r += cr; g += cg; b += cb;
                    }
                }
            g_glow[0][by][bx] = (uint16_t)(r / 16);
            g_glow[1][by][bx] = (uint16_t)(g / 16);
            g_glow[2][by][bx] = (uint16_t)(b / 16);
        }
}

/* Two box-blur passes per axis approximate a soft gaussian. */
static void blur_axis(uint16_t buf[BH][BW], bool vertical)
{
    static uint16_t line[BW > BH ? BW : BH];
    int n = vertical ? BH : BW, lines = vertical ? BW : BH, a, i, k;
    for (a = 0; a < lines; a++)
        for (k = 0; k < 2; k++) {
            for (i = 0; i < n; i++) {
                int s = 0, j;
                for (j = -2; j <= 2; j++) {
                    int q = CLAMP(i + j, 0, n - 1);
                    s += vertical ? buf[q][a] : buf[a][q];
                }
                line[i] = (uint16_t)(s / 5);
            }
            for (i = 0; i < n; i++)
                *(vertical ? &buf[i][a] : &buf[a][i]) = line[i];
        }
}

static uint16_t add_glow(uint16_t c, int x, int y)
{
    int bx = MIN(x / 4, BW - 1), by = MIN(y / 4, BH - 1);
    int r = (c >> 11) * 8 + g_glow[0][by][bx] * 3 / 4;
    int g = ((c >> 5) & 63) * 4 + g_glow[1][by][bx] * 3 / 4;
    int b = (c & 31) * 8 + g_glow[2][by][bx] * 3 / 4;
    return RGB565(MIN(r, 255), MIN(g, 255), MIN(b, 255));
}

void render_bloom(void)
{
    uint16_t *fb = gfx_target();
    int c, x, y;
    gather(fb);
    for (c = 0; c < 3; c++) {
        blur_axis(g_glow[c], false);
        blur_axis(g_glow[c], true);
    }
    for (y = 0; y < BH * 4; y++)
        for (x = 0; x < FB_W; x++)
            fb[y * FB_W + x] = add_glow(fb[y * FB_W + x], x, y);
}

#else  /* the calculator keeps its frame budget for the game itself */

void render_ambient(const World *w, int cam_x, int cam_y) { (void)w; (void)cam_x; (void)cam_y; }
void render_bloom(void) {}

#endif
