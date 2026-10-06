/*
 * render_gear.c - projectiles and the hero's weapons. Weapons are drawn in
 * code so they can swing, and take the colour of their material (rarity):
 * legendary and better gear glints.
 */
#include "render_fx.h"
#include "items.h"
#include "skills.h"
#include "../gfx/gfx.h"
#include "../gfx/sprites.h"
#include "../core/trig.h"

#define C_WHITE  RGB565(255, 250, 235)
#define C_FLAME  RGB565(255, 120, 30)
#define C_FLAME2 RGB565(255, 210, 80)
#define C_WOOD   RGB565(120, 80, 45)
#define C_WOOD2  RGB565(80, 50, 28)
#define C_BONE   RGB565(235, 228, 205)

/* ---------------------------------------------------------- projectiles */

static void streak(int x, int y, fx vx, fx vy, int len, uint16_t shaft, uint16_t head)
{
    int d = MAX(1, ABS(FX_TO_INT(vx)) + ABS(FX_TO_INT(vy)));
    int tx = x - FX_TO_INT(vx) * len / d, ty = y - FX_TO_INT(vy) * len / d;
    gfx_line(tx, ty, x, y, shaft);
    gfx_pixel(x, y, head);
}

static void arrow(int x, int y, fx vx, fx vy, int len, uint16_t shaft, uint16_t head)
{
    int d = MAX(1, ABS(FX_TO_INT(vx)) + ABS(FX_TO_INT(vy)));
    int tx = x - FX_TO_INT(vx) * len / d, ty = y - FX_TO_INT(vy) * len / d;
    streak(x, y, vx, vy, len, shaft, head);
    gfx_pixel(tx, ty - 1, RGB565(200, 60, 60));   /* fletching */
    gfx_pixel(tx, ty + 1, RGB565(200, 60, 60));
}

static void orb(int x, int y, int r, uint16_t c)
{
    gfx_fill_rect(x - r, y - r, 2 * r + 1, 2 * r + 1, c);
    gfx_pixel(x, y, C_WHITE);
}

static void trail(int x, int y, fx vx, fx vy, int n, uint16_t a, uint16_t b)
{
    int k;
    for (k = 1; k <= n; k++)
        gfx_fill_rect(x - FX_TO_INT(vx) * k / 2, y - FX_TO_INT(vy) * k / 2, 2, 2, k & 1 ? a : b);
}

static void proj_special(const Proj *pj, int x, int y, int st, uint16_t c)
{
    int k;
    switch (st) {
    case VS_TORNADO:                       /* a spinning column of wind */
        for (k = 0; k < 5; k++)
            gfx_hline(x - 5 + k + ((pj->life + k) & 1), y - 12 + k * 3, 10 - k * 2, k & 1 ? C_WHITE : c);
        break;
    case VS_BOULDER:                       /* a rolling boulder */
        gfx_fill_rect(x - 4, y - 7, 9, 8, RGB565(120, 90, 60));
        gfx_rect(x - 4, y - 7, 9, 8, RGB565(70, 50, 30));
        gfx_pixel(x - 2 + (pj->life & 3), y - 5, RGB565(190, 160, 120));
        break;
    case VS_QUILL:                         /* a glowing feather */
        streak(x, y, pj->vx, pj->vy, 7, c, C_WHITE);
        gfx_pixel(x - FX_TO_INT(pj->vx), y - FX_TO_INT(pj->vy) - 1, C_WHITE);
        break;
    case VS_DAGGER:                        /* a spinning blade */
        gfx_line(x - 2 + (pj->life & 1) * 4, y - 2, x + 2 - (pj->life & 1) * 4, y + 2, C_WHITE);
        gfx_pixel(x, y, c);
        break;
    case VS_BONE:
        streak(x, y, pj->vx, pj->vy, 4, C_BONE, C_WHITE);
        break;
    case VS_BONESPEAR:                     /* a long lance of bone with a pale trail */
        streak(x, y, pj->vx, pj->vy, 12, C_BONE, C_WHITE);
        streak(x, y + 1, pj->vx, pj->vy, 8, RGB565(170, 160, 130), C_BONE);
        break;
    default:
        orb(x, y, 2, c);
        break;
    }
}

void fx_draw_proj(const Proj *pj, int x, int y)
{
    uint16_t c = element_color((Element)pj->hit.element);
    int st = fx_style_of(pj->vfx);
    if (pj->vfx == VX_ENEMY) {
        orb(x, y, 2, pj->enemy_el == EL_FIRE ? C_FLAME : RGB565(200, 60, 255));
        return;
    }
    if (pj->vfx == VX_MINION_BOLT) {
        orb(x, y, 1, RGB565(180, 100, 240));
        gfx_pixel(x - FX_TO_INT(pj->vx), y - FX_TO_INT(pj->vy), RGB565(110, 60, 160));
        return;
    }
    switch (st) {
    case VS_FIREBALL:
        trail(x, y, pj->vx, pj->vy, 3, C_FLAME, C_FLAME2);
        orb(x, y, 2, c);
        break;
    case VS_FROSTBOLT:
    case VS_SHARD:
        arrow(x, y, pj->vx, pj->vy, 6, c, C_WHITE);
        break;
    case VS_POWERSHOT:
        arrow(x, y, pj->vx, pj->vy, 10, RGB565(210, 180, 120), C_WHITE);
        trail(x, y, pj->vx, pj->vy, 4, C_WHITE, c);
        break;
    case VS_ARROW:
        arrow(x, y, pj->vx, pj->vy, 6, RGB565(200, 170, 110), c);
        break;
    default:
        proj_special(pj, x, y, st, c);
        break;
    }
}

/* -------------------------------------------------------------- weapons */

static uint16_t metal(int mat) { return mat ? material_color(mat) : RGB565(200, 205, 215); }

/* Legendary, unique and mythic gear glints now and then. */
static void glint(int x, int y, int mat, int tick)
{
    static const uint16_t glow[3] = { RGB565(255, 160, 40), RGB565(255, 230, 150), RGB565(220, 150, 255) };
    if (mat >= RAR_LEGEND + 1 && ((tick >> 2) & 7) == 0)
        gfx_pixel(x, y, glow[CLAMP(mat - RAR_LEGEND - 1, 0, 2)]);
}

/* Tip of a weapon held at the hand, swung by attack_t. */
static void swing_tip(int hx, int hy, int f, int attack_t, int len, int *tx, int *ty)
{
    int phase = attack_t > 0 ? 8 - attack_t : -1;
    int ang = phase < 0 ? 10 : 56 - phase * 7;
    *tx = hx + f * isin(ang + 16) * len / 127;
    *ty = hy - isin(ang) * len / 127;
}

static void blade(int x, int y, int f, int attack_t, int mat, int len, int tick)
{
    int hx = x + f * 4, hy = y + 1, tx, ty;
    swing_tip(hx, hy, f, attack_t, len, &tx, &ty);
    gfx_line(hx, hy, tx, ty, metal(mat));
    gfx_line(hx, hy + 1, tx, ty + 1, RGB565(110, 115, 130));
    gfx_fill_rect(hx - 1, hy - 1, 3, 3, RGB565(250, 210, 80));
    glint(tx, ty, mat, tick);
}

static void hafted(int x, int y, int f, int attack_t, int mat, int kind, int tick)
{
    int hx = x + f * 4, hy = y + 2, tx, ty;
    swing_tip(hx, hy, f, attack_t, 11, &tx, &ty);
    gfx_line(hx, hy, tx, ty, C_WOOD);
    if (kind == WK_AXE) {                  /* axe head */
        gfx_fill_rect(tx - 1, ty - 1, 3, 4, metal(mat));
        gfx_pixel(tx + f * 2, ty, metal(mat));
    } else if (kind == WK_MACE) {          /* flanged ball */
        gfx_fill_rect(tx - 2, ty - 2, 4, 4, metal(mat));
        gfx_pixel(tx - 2, ty - 2, RGB565(60, 60, 70));
    } else {                               /* scythe: long curved blade */
        gfx_line(tx, ty, tx - f * 7, ty + 3, metal(mat));
        gfx_line(tx, ty + 1, tx - f * 6, ty + 4, RGB565(110, 115, 130));
    }
    glint(tx, ty, mat, tick);
}

static void pole(int x, int y, int f, int attack_t, int mat, int kind, uint16_t accent, int tick)
{
    int lean = attack_t > 0 ? 5 : 1;
    int bx = x + f * 6, by = y + 7, tx = bx + f * lean, ty = y - 12;
    if (kind == WK_GLAIVE && attack_t > 0) {   /* the glaive thrusts forward */
        gfx_line(x - f * 2, y, x + f * 14, y - 2, C_WOOD);
        gfx_line(x + f * 14, y - 2, x + f * 19, y - 3, metal(mat));
        glint(x + f * 19, y - 3, mat, tick);
        return;
    }
    gfx_line(bx, by, tx, ty, C_WOOD);
    gfx_line(bx + f, by, tx + f, ty, C_WOOD2);
    if (kind == WK_STAFF) {                /* orb in the build's colour */
        gfx_fill_rect(tx - 1, ty - 3, 4, 4, attack_t > 0 ? C_WHITE : accent);
        gfx_pixel(tx + ((tick >> 3) & 1 ? 2 : -1), ty - 4 - ((tick >> 2) & 1), accent);
    } else if (kind == WK_GLAIVE) {
        gfx_line(tx, ty, tx, ty - 4, metal(mat));
    } else {                               /* quarterstaff: metal caps */
        gfx_fill_rect(tx - 1, ty - 1, 3, 2, metal(mat));
    }
    glint(tx, ty - 2, mat, tick);
}

static void bow(int x, int y, int f, int attack_t, int mat, bool cross, int tick)
{
    int cx = x + f * 4, a, pull = attack_t > 4 ? 4 : attack_t > 0 ? 2 : 0;
    uint16_t limb = mat >= RAR_LEGEND + 1 ? metal(mat) : RGB565(140, 95, 50);
    if (cross) {                           /* crossbow: stock forward, limbs across */
        gfx_line(x, y + 1, x + f * 10, y + 1, C_WOOD);
        gfx_line(x + f * 8, y - 4, x + f * 8, y + 6, limb);
        gfx_pixel(x + f * 11, y + 1, C_WHITE);
        glint(x + f * 8, y - 4, mat, tick);
        return;
    }
    for (a = -14; a <= 14; a++)            /* the limb: half an ellipse facing forward */
        gfx_pixel(cx + f * isin(a + 16) * 5 / 127, y + isin(a) * 9 / 127, limb);
    gfx_line(cx, y - 9, cx - f * pull, y, RGB565(220, 220, 220));
    gfx_line(cx - f * pull, y, cx, y + 9, RGB565(220, 220, 220));
    if (attack_t > 4)
        gfx_line(cx - f * pull, y, cx + f * 8, y, RGB565(210, 180, 120)); /* nocked arrow */
    glint(cx + f * 5, y, mat, tick);
}

void fx_draw_weapon(int kind, int mat, int x, int y, int face, int attack_t, uint16_t accent, int tick)
{
    switch (kind) {
    case WK_SWORD:    blade(x, y, face, attack_t, mat, 10, tick); break;
    case WK_DAGGER:   blade(x, y, face, attack_t, mat, 6, tick); break;
    case WK_AXE:
    case WK_MACE:
    case WK_SCYTHE:   hafted(x, y, face, attack_t, mat, kind, tick); break;
    case WK_STAFF:
    case WK_GLAIVE:
    case WK_QSTAFF:   pole(x, y, face, attack_t, mat, kind, accent, tick); break;
    case WK_WAND:
        gfx_line(x + face * 4, y + 1, x + face * 9, y - 4 - (attack_t > 0 ? 2 : 0), C_WOOD);
        gfx_pixel(x + face * 9, y - 5 - (attack_t > 0 ? 2 : 0), attack_t > 0 ? C_WHITE : accent);
        break;
    case WK_BOW:      bow(x, y, face, attack_t, mat, false, tick); break;
    case WK_CROSSBOW: bow(x, y, face, attack_t, mat, true, tick); break;
    default:          break;
    }
}

void fx_draw_offhand(int kind, int mat, int x, int y, int face, int tick)
{
    int bx = x - face * 6, by = y + 1;
    switch (kind) {
    case OK_SHIELD:
        gfx_fill_rect(bx - 3, by - 4, 6, 8, metal(mat));
        gfx_rect(bx - 3, by - 4, 6, 8, RGB565(40, 36, 44));
        gfx_pixel(bx, by - 1, RGB565(250, 210, 80));
        break;
    case OK_FOCUS:                         /* an orb floating by the off hand */
        gfx_fill_rect(bx - 1, by - 8 + ((tick >> 4) & 1), 3, 3, metal(mat));
        gfx_pixel(bx, by - 7 + ((tick >> 4) & 1), C_WHITE);
        break;
    case OK_TOTEM:
        gfx_vline(bx, by - 5, 7, C_WOOD);
        gfx_fill_rect(bx - 1, by - 7, 3, 3, metal(mat));
        break;
    case OK_DAGGER:
    case OK_AXE:
        gfx_line(bx, by + 2, bx - face * 4, by - 4, metal(mat));
        if (kind == OK_AXE)
            gfx_fill_rect(bx - face * 4 - 1, by - 5, 3, 3, metal(mat));
        break;
    default:
        return;
    }
    glint(bx, by - 4, mat, tick);
}
