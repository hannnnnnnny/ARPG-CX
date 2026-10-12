/*
 * render_door.c - the door choice between floors (doors.h): three cards
 * over the held battle view, the highlighted one framed, a countdown to
 * the hero's own pick.
 */
#include "render.h"
#include "doors.h"
#include "../gfx/gfx.h"
#include "../gfx/font.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define CARD_Y 30
#define CARD_W 96
#define CARD_H 158
#define CARD_DX 104
#define CARD_X0 8

#define C_TITLE RGB565(255, 200, 90)
#define C_LABEL RGB565(130, 118, 104)
#define C_CURSE RGB565(240, 90, 80)
#define C_GAIN  RGB565(110, 230, 110)

int door_card_at(int mx, int my)
{
    int i;
    for (i = 0; i < DOORS; i++)
        if (mx >= CARD_X0 + i * CARD_DX && mx < CARD_X0 + i * CARD_DX + CARD_W && my >= CARD_Y && my < CARD_Y + CARD_H)
            return i;
    return -1;
}

/* Stepped round top of the arch: half width of each row. */
static const int8_t arch_top[6] = { 4, 7, 9, 10, 11, 12 };

static void arch_shape(int cx, int y, int inset, int h, uint16_t c, bool blend)
{
    int r;
    for (r = 0; r < 6; r++) {
        int half = arch_top[r] - inset;
        if (half <= 0)
            continue;
        if (blend)
            gfx_blend_rect(cx - half, y + r + inset, half * 2 + 1, 1, c);
        else
            gfx_hline(cx - half, y + r + inset, half * 2 + 1, c);
    }
    if (blend)
        gfx_blend_rect(cx - 12 + inset, y + 6 + inset, 25 - inset * 2, h - 6 - inset, c);
    else
        gfx_fill_rect(cx - 12 + inset, y + 6 + inset, 25 - inset * 2, h - 6 - inset, c);
}

/* A stone arch with the door's light pouring out of it. */
static void draw_arch(int cx, int y, uint16_t c)
{
    arch_shape(cx, y, 0, 36, RGB565(120, 104, 96), false);
    arch_shape(cx, y, 2, 34, RGB565(18, 12, 14), false);
    arch_shape(cx, y, 3, 33, c, true);
    gfx_blend_rect(cx - 2, y + 8, 5, 28, c);
    gfx_hline(cx - 14, y + 36, 29, RGB565(120, 104, 96));
}

static void draw_card(const Door *d, int i, bool sel, bool pref)
{
    int x = CARD_X0 + i * CARD_DX, y = CARD_Y;
    uint16_t c = door_color(d);
    gfx_fill_rect(x, y, CARD_W, CARD_H, RGB565(20, 15, 17));
    gfx_rect(x, y, CARD_W, CARD_H, sel ? C_TITLE : RGB565(96, 62, 38));
    if (sel)
        gfx_rect(x - 1, y - 1, CARD_W + 2, CARD_H + 2, C_TITLE);
    if (pref)
        font_draw(x + CARD_W - 4 - font_text_width("AUTO", 1), y + 3, "AUTO", c, 1);
    draw_arch(x + CARD_W / 2, y + 8, c);
    y = font_draw_wrapped(x + 4, y + 50, door_name(d), CARD_W - 8, c) + 3;
    font_draw(x + 4, y, "CURSE", C_LABEL, 1);
    y = font_draw_wrapped(x + 4, y + 10, door_curse_text(d->curse), CARD_W - 8, C_CURSE) + 3;
    font_draw(x + 4, y, "REWARD", C_LABEL, 1);
    font_draw_wrapped_clip(x + 4, y + 10, door_reward_text(d->reward), CARD_W - 8, C_GAIN, CARD_Y + CARD_H - 2);
}

void render_doors(const Session *s, const Profile *p)
{
    char buf[96];
    int i, pref = doors_auto_pick(s->doors, p->door_pref);
    gfx_dim_rect(0, 0, 320, 200);
    gfx_dim_rect(0, 0, 320, 200);
    gfx_fill_rect(0, 0, 320, 27, RGB565(10, 7, 9));
    snprintf(buf, sizeof buf, T("FLOOR %d - CHOOSE A DOOR"), p->floor);
    font_draw_centered(6, buf, C_TITLE, RGB565(0, 0, 0), 1);
    font_draw_centered(17, "EACH DOOR CURSES THE FLOOR AND PAYS FOR IT", C_LABEL, RGB565(0, 0, 0), 1);
    for (i = 0; i < DOORS; i++)
        draw_card(&s->doors[i], i, i == s->door_sel, i == pref);
    snprintf(buf, sizeof buf, T("LEFT/RIGHT, ENTER: GO IN   %d"), (s->door_t + TICK_HZ - 1) / TICK_HZ);
    font_draw_centered(CARD_Y + CARD_H + 4, buf, C_LABEL, RGB565(0, 0, 0), 1);
}
