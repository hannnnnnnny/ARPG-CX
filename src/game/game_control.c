/*
 * game_control.c - keyboard and mouse on the desktop:
 *
 *   battle   the HeroCommand of each tick (see world_control.c), clicks on
 *            the HUD's skill and potion buttons, Space for auto battle
 *   menus    click a tab, click a row to select it and again to use it,
 *            wheel to scroll, right click to go back
 *   sound    the world's sound events and interface clicks
 *
 * Menus stay keyboard driven underneath: a click is turned into the same
 * UP / DOWN / OK presses, so every page works with the mouse without
 * knowing about it. On the calculator none of this ever fires.
 */
#include "game.h"
#include "render.h"
#include "../core/platform.h"
#include "../core/sound.h"
#include "../gfx/gfx.h"
#include "../gfx/font.h"
#include "../i18n/i18n.h"
#include <string.h>

#define UI_ROWS 64
#define VIEW_H  200            /* battle view above the HUD */
#define SLOT_X  62             /* HUD skill bar: x = SLOT_X + slot * SLOT_DX, y = SLOT_Y */
#define SLOT_DX 20
#define SLOT_Y  203

typedef struct { int16_t x, y, w, h; bool on; } UiRow;

/* Rows drawn by the last frame (ticks never run in the middle of a frame). */
static UiRow g_rows[UI_ROWS];
static int   g_nrows;

/* ------------------------------------------------------------ ui rows */

void game_ui_rect(int x, int y, int w, int h, bool on)
{
    if (g_nrows < UI_ROWS) {
        UiRow *r = &g_rows[g_nrows++];
        r->x = (int16_t)x;
        r->y = (int16_t)y;
        r->w = (int16_t)w;
        r->h = (int16_t)h;
        r->on = on;
    }
}

void game_ui_row(int y, int h, bool on)
{
    game_ui_rect(0, y, SCREEN_W, h, on);
}

void game_ui_frame(void)
{
    g_nrows = 0;
}

/* Clicked row relative to the selected one (0 = the selected row itself);
 * false when the click is not on a row of a list with a selection. */
static bool clicked_row(int mx, int my, int *delta)
{
    int i, hit = -1, sel = -1;
    for (i = 0; i < g_nrows; i++) {
        const UiRow *r = &g_rows[i];
        if (mx >= r->x && mx < r->x + r->w && my >= r->y && my < r->y + r->h)
            hit = i;
        if (g_rows[i].on)
            sel = i;
    }
    if (hit < 0 || sel < 0)
        return false;
    *delta = hit - sel;
    return true;
}

/* ---------------------------------------------------------------- menus */

/* Inject one queued navigation press per tick; the page reacts as if
 * the arrow keys and ENTER had been used. */
static void replay_nav(Game *g, Input *in)
{
    if (g->nav_pending > 0) {
        in->pressed |= BTN_DOWN;
        g->nav_pending--;
    } else if (g->nav_pending < 0) {
        in->pressed |= BTN_UP;
        g->nav_pending++;
    } else if (g->nav_ok) {
        in->pressed |= BTN_OK;
        g->nav_ok = false;
    }
}

static bool menu_tab_click(Game *g, const Input *in)
{
    if (g->state != GS_MENU || in->my >= 14)
        return false;
    g->page = (MenuPage)CLAMP(in->mx / (SCREEN_W / PG_COUNT), 0, PG_COUNT - 1);
    return true;
}

void game_mouse_ui(Game *g, Input *in)
{
    int delta;
    if (!in->mouse || g->state == GS_BATTLE)
        return;
    replay_nav(g, in);
    if (in_pressed(in, BTN_WHEEL_UP))   in->pressed |= BTN_UP;
    if (in_pressed(in, BTN_WHEEL_DOWN)) in->pressed |= BTN_DOWN;
    if (in_pressed(in, BTN_MOUSE_R))    in->pressed |= BTN_BACK;
    if (!in_click(in) || menu_tab_click(g, in))
        return;
    if (g->confirm != CF_NONE || g->state == GS_STORY || g->state == GS_OFFLINE) {
        in->pressed |= BTN_OK;                   /* dialogs and pages: click = continue */
        return;
    }
    if (clicked_row(in->mx, in->my, &delta)) {
        g->nav_pending = delta;
        g->nav_ok = delta == 0;
    }
}

/* --------------------------------------------------------------- battle */

static int hud_slot_at(const Input *in)
{
    int s;
    for (s = 0; s < BAR_SLOTS; s++)
        if (in->mx >= SLOT_X + s * SLOT_DX - 1 && in->mx < SLOT_X + s * SLOT_DX + 17
            && in->my >= SLOT_Y - 1 && in->my < SLOT_Y + 17)
            return s;
    return -1;
}

static bool hud_potion_at(const Input *in)
{
    return in->mx >= 42 && in->mx < 58 && in->my >= SLOT_Y && in->my < SLOT_Y + 16;
}

static int8_t key_cast(const Input *in)
{
    int s;
    for (s = 0; s < BAR_SLOTS; s++)
        if (in_pressed(in, BTN_SKILL1 << s))
            return (int8_t)s;
    return -1;
}

/* Mouse part of the command: world clicks, HUD buttons, right click = slot 2. */
static void mouse_command(const Input *in, HeroCommand *c)
{
    int cx, cy;
    bool in_view = in->my < VIEW_H && in->my >= 11;
    if (!in->mouse)
        return;
    render_camera(&cx, &cy);
    c->tx = FX_FROM_INT(in->mx + cx);
    c->ty = FX_FROM_INT(in->my + cy);
    if (in_click(in) && !in_view) {
        if (hud_slot_at(in) >= 0)
            c->cast = (int8_t)hud_slot_at(in);
        c->potion = c->potion || hud_potion_at(in);
    } else if (in_view) {
        c->click = in_click(in);
        c->hold = in_held(in, BTN_MOUSE_L);
    }
    if (in_pressed(in, BTN_MOUSE_R) && in_view)
        c->cast = 1;
}

void game_hero_command(Game *g, const Input *in)
{
    HeroCommand *c = &g->s.w.cmd;
    memset(c, 0, sizeof *c);
    c->cast = -1;
    if (g->state != GS_BATTLE)
        return;
    c->mx = (int8_t)(in_held(in, BTN_RIGHT) - in_held(in, BTN_LEFT));
    c->my = (int8_t)(in_held(in, BTN_DOWN) - in_held(in, BTN_UP));
    c->potion = in_pressed(in, BTN_POTION);
    c->cast = key_cast(in);
    mouse_command(in, c);
    c->active = c->mx || c->my || c->click || c->hold || c->cast >= 0 || c->potion;
}

/* ---------------------------------------------------------------- sound */

void game_sfx(const Game *g, int id)
{
    plat_sound(id, g->p.sound_vol);
}

/* Interface clicks for every screen that is not the battle view. */
void game_ui_sounds(const Game *g, const Input *in)
{
    if (g->state == GS_BATTLE)
        return;
    if (in_tab(in))
        game_sfx(g, SND_PAGE);
    else if (in_ok(in))
        game_sfx(g, SND_UI_OK);
    else if (in_back(in))
        game_sfx(g, SND_UI_BACK);
    else if (in_up(in) || in_down(in) || in_left(in) || in_right(in))
        game_sfx(g, SND_UI_MOVE);
}

void game_world_sounds(const Game *g)
{
    int id;
    uint64_t bits = g->s.w.snd;
    for (id = 0; bits && id < SND_COUNT; id++)
        if (bits & ((uint64_t)1 << id))
            game_sfx(g, id);
}

/* ------------------------------------------------------------- overlays */

static bool near_px(int ax, int ay, fx bx, fx by, int r)
{
    int dx = ax - FX_TO_INT(bx), dy = ay - FX_TO_INT(by);
    return dx * dx + dy * dy <= r * r;
}

/* With a mouse: the monster under the cursor, slot hotkeys and the mode. */
void game_control_render(const Game *g)
{
    const World *w = &g->s.w;
    int cx, cy, s, i;
    if (!g->mouse || g->state != GS_BATTLE)
        return;
    render_camera(&cx, &cy);
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && near_px(g->mx + cx, g->my + cy, w->mon[i].x, w->mon[i].y, 14)) {
            gfx_circle(FX_TO_INT(w->mon[i].x) - cx, FX_TO_INT(w->mon[i].y) - cy + 2, 10, RGB565(255, 70, 60));
            break;
        }
    for (s = 0; s < BAR_SLOTS; s++) {
        char k[2] = { (char)('1' + s), 0 };
        font_draw(SLOT_X + s * SLOT_DX + 11, SLOT_Y + 10, k, RGB565(255, 230, 150), 1);
    }
    font_draw(232, 214, g->p.auto_battle ? (w->ctl.manual_t > 0 ? "MANUAL" : "AUTO") : "MANUAL",
              g->p.auto_battle && w->ctl.manual_t == 0 ? RGB565(120, 200, 255) : RGB565(255, 200, 90), 1);
}
