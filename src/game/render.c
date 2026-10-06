/*
 * render.c - the battle view (320x200 above the HUD).
 *
 * 3/4 top-down: a wall cell with open floor below shows its brick face,
 * other walls show a dark cap. Torchlight: each visible cell picks one of
 * four pre-shaded tile copies by distance from the hero; unexplored cells
 * stay black (fog of war). Entities are drawn back-to-front by feet Y.
 * Legendary and better loot stands in a pillar of light you can see from
 * across the room, like in Diablo IV.
 */
#include "render.h"
#include "render_fx.h"
#include "world_int.h"
#include "skills.h"
#include "build.h"
#include "items.h"
#include "../gfx/tiles.h"
#include "../gfx/sprites.h"
#include "../gfx/font.h"
#include "../core/trig.h"
#include <stdlib.h>

#define VIEW_H 200
#define LIGHT_PX 120 /* torchlight radius */

typedef struct { int16_t y; uint8_t kind; uint8_t idx; } DrawItem;
enum { DI_DROP, DI_MON, DI_HERO, DI_PROJ, DI_ALLY };

static int cam_x, cam_y;
static int g_alpha = 256; /* 0..256: how far we are between the last two ticks */

void render_set_alpha(int alpha) { g_alpha = CLAMP(alpha, 0, 256); }

/* Interpolated pixel coordinate between the previous and current tick.
 * Large jumps (new floor, teleport) are shown immediately, never smeared. */
static int ipx(fx prev, fx cur)
{
    fx d = cur - prev;
    if (d > FX(24) || d < -FX(24))
        return FX_TO_INT(cur);
    return FX_TO_INT(prev + (fx)((int64_t)d * g_alpha / 256));
}

static void compute_camera(const World *w)
{
    cam_x = CLAMP(ipx(w->h.px, w->h.x) - SCREEN_W / 2, 0, MAP_W * TILE_SIZE - SCREEN_W);
    cam_y = CLAMP(ipx(w->h.py, w->h.y) - VIEW_H / 2, 0, MAP_H * TILE_SIZE - VIEW_H);
}

/* Light falls off with the pixel distance from the hero to each tile's
 * centre, so as the hero walks single tiles change shade one at a time. */
static int light_px(const World *w, fx x, fx y)
{
    int d = dist_px(w->h.x, w->h.y, x, y);
    return d <= 56 ? 0 : d <= 88 ? 1 : d <= LIGHT_PX ? 2 : 3;
}

static int tile_for(const World *w, int cx, int cy)
{
    uint8_t c = w->cell[cy][cx];
    if (c == CELL_STAIRS || (cx == w->stairs_x && cy == w->stairs_y))
        return TL_STAIRS;
    if (c == CELL_FLOOR)
        return w->var[cy][cx] < 5 ? TL_FLOOR0 : w->var[cy][cx] < 7 ? TL_FLOOR1 : TL_FLOOR2;
    return (cy + 1 < MAP_H && w->cell[cy + 1][cx] != CELL_WALL) ? TL_WALL_FRONT : TL_WALL_TOP;
}

static void draw_torch(int sx, int sy, int tick)
{
    uint16_t flame = (tick >> 4) & 1 ? RGB565(255, 190, 60) : RGB565(255, 150, 40);
    gfx_fill_rect(sx + 7, sy + 7, 2, 6, RGB565(90, 60, 30));
    gfx_fill_rect(sx + 6, sy + 3, 4, 4, flame);
    gfx_pixel(sx + 7, sy + 2 - ((tick >> 3) & 1), RGB565(255, 240, 160));
}

static void draw_map(const World *w)
{
    int cx0 = cam_x >> TILE_SHIFT, cy0 = cam_y >> TILE_SHIFT, cx, cy;
    for (cy = cy0; cy <= (cam_y + VIEW_H - 1) >> TILE_SHIFT && cy < MAP_H; cy++)
        for (cx = cx0; cx <= (cam_x + SCREEN_W - 1) >> TILE_SHIFT && cx < MAP_W; cx++) {
            int sx = cx * TILE_SIZE - cam_x, sy = cy * TILE_SIZE - cam_y, t, shade;
            if (!w->seen[cy][cx]) {
                gfx_fill_rect(sx, sy, TILE_SIZE, MIN(TILE_SIZE, VIEW_H - sy), 0);
                continue;
            }
            t = tile_for(w, cx, cy);
            shade = light_px(w, cell_center(cx), cell_center(cy));
            gfx_blit_tile(tile_px(t, shade), sx, sy, true);
            if (t == TL_WALL_FRONT && w->var[cy][cx] == 0 && (cx % 3) == 0 && shade < 3)
                draw_torch(sx, sy, w->tick + cx * 5);
        }
}

static bool visible_px(const World *w, fx x, fx y)
{
    return w->seen[px_to_cell(y)][px_to_cell(x)] && light_px(w, x, y) < 3;
}

static void shadow(int x, int y, int half)
{
    gfx_dim_rect(x - half, y - 1, half * 2, 3);
}

static void draw_hp_bar(int x, int y, int w, double frac, uint16_t c)
{
    gfx_fill_rect(x, y, w, 2, RGB565(30, 0, 0));
    gfx_fill_rect(x, y, (int)(w * CLAMP(frac, 0.0, 1.0)), 2, c);
}

/* ------------------------------------------------------------ monsters */

static void status_marks(const World *w, const Monster *m, int x, int top)
{
    int k;
    if (m->vuln) {                         /* Vulnerable: a purple chevron overhead */
        gfx_line(x - 3, top - 7, x, top - 4, RGB565(200, 110, 255));
        gfx_line(x, top - 4, x + 3, top - 7, RGB565(200, 110, 255));
    }
    if (m->stun)                           /* dizzy stars */
        gfx_pixel(x - 3 + ((w->tick >> 2) & 7), top - 2, RGB565(255, 230, 90));
    if (m->immob)                          /* roots / bone bars */
        gfx_hline(x - 4, top + 18, 8, RGB565(235, 228, 205));
    if (m->chill)
        gfx_pixel(x + 4, top + 1, RGB565(120, 200, 255));
    for (k = 0; k < DOT_KINDS; k++)        /* dripping DoTs in their element colour */
        if (m->dot_t[k]) {
            static const uint16_t c[DOT_KINDS] = { RGB565(255, 130, 40), RGB565(130, 230, 80),
                                                   RGB565(220, 30, 30), RGB565(180, 100, 240) };
            gfx_fill_rect(x - 4 + k * 3, top + 1 + ((w->tick + k * 3) >> 1 & 3), 2, 2, c[k]);
        }
}

static void draw_monster(const World *w, const Monster *m)
{
    int x = ipx(m->px, m->x) - cam_x, y = ipx(m->py, m->y) - cam_y;
    int size = m->boss ? 32 : 16, frame = (m->anim >> 3) & 1, top = y - size / 2 - (m->boss ? 8 : 4);
    unsigned flags = (m->face > 0 ? BLIT_FLIP_X : 0) | (m->flash ? BLIT_WHITE : 0);
    shadow(x, y + size / 2 - 2, size / 3);
    if (m->elite)
        gfx_circle(x, y + 2, 9, RGB565(70, 130, 240));
    gfx_blit(spr_mon(m->type, frame, m->boss), x - size / 2, top, flags);
    if (m->freeze)
        gfx_rect(x - size / 2, top, size, size, RGB565(140, 210, 255));
    status_marks(w, m, x, top);
    render_monster_extras(w, m, x, top);
    if (m->hp < m->max_hp)
        draw_hp_bar(x - size / 3, top - 3, size * 2 / 3, m->hp / m->max_hp,
                    m->vuln ? RGB565(200, 110, 255) : m->boss ? RGB565(255, 120, 40) : RGB565(220, 40, 40));
}

/* ------------------------------------------------------- hero and allies */

static void draw_hero(const World *w, const Profile *p)
{
    const HeroRT *h = &w->h;
    HeroLook look;
    int x = ipx(h->px, h->x) - cam_x, y = ipx(h->py, h->y) - cam_y;
    int frame = h->moving ? 2 + ((h->anim >> 3) & 1) : ((h->anim >> 5) & 1);
    unsigned flags = (h->face < 0 ? BLIT_FLIP_X : 0) | (h->flash ? BLIT_WHITE : 0);
    const Item *wp = &p->equip[SLOT_WEAPON], *oh = &p->equip[SLOT_OFFHAND];
    hero_look_from(&look, p);
    if (h->dead_t > 0) {
        gfx_blit(spr_hero_look(&look, 0), x - 8, y - 8, BLIT_FLIP_Y);
        return;
    }
    shadow(x, y + 7, 5);
    if (h->barrier > 0)                    /* barrier: a pale shell around the hero */
        fx_arc(x, y - 3, 11, w->tick, 40, RGB565(220, 230, 255));
    if (h->buff_t[BUFF_ULT] > 0 || h->buff_t[BUFF_BERSERK] > 0)
        gfx_circle(x, y, 12 + (w->tick & 1), h->buff_t[BUFF_ULT] > 0 ? w->accent : RGB565(230, 50, 50));
    if (h->channel_t > 0)
        gfx_circle(x, y, 14 + (w->tick & 3), RGB565(255, 220, 120));
    if (oh->used)
        fx_draw_offhand(oh->base, oh->rarity + 1, x, y, h->face, w->tick);
    gfx_blit(spr_hero_look(&look, frame), x - 8, y - 13, flags);
    if (wp->used)
        fx_draw_weapon(wp->base, wp->rarity + 1, x, y, h->face, h->attack_t, w->accent, w->tick);
}

static void draw_ally(const World *w, const Ally *a)
{
    int x = ipx(a->px, a->x) - cam_x, y = ipx(a->py, a->y) - cam_y;
    int rise = a->rise > 0 ? a->rise / 2 : 0;
    const Sprite *s = spr_ally(a->kind, (a->anim >> 3) & 1);
    shadow(x, y + 6, 4);
    gfx_blit(s, x - 8, y - 10 + rise, a->face > 0 ? BLIT_FLIP_X : 0);
    if (a->atk_cd > 24 && a->kind != AK_MAGE)  /* a bite / slash flash */
        gfx_pixel(x + a->face * 8, y - 4, RGB565(255, 255, 255));
    (void)w;
}

/* --------------------------------------------------------------- loot */

static uint16_t beam_color(const Item *it)
{
    switch (it->rarity) {
    case RAR_MYTHIC: return RGB565(200, 120, 255);
    case RAR_UNIQUE: return RGB565(255, 215, 120);
    default:         return RGB565(255, 140, 30);
    }
}

/* A pillar of light over legendary, unique and mythic drops. Ancestral
 * items get a taller beam with a white core and rising sparks. */
static void draw_beam(const World *w, const Drop *d, int x, int y)
{
    uint16_t c = beam_color(&d->item);
    int h = d->item.ancestral ? 90 : 56, k, pulse = (w->tick + d->t) & 31;
    gfx_blend_rect(x - 2, y - h, 5, h, c);
    gfx_blend_rect(x - 1, y - h - 10, 3, 10, c);
    if (d->item.ancestral) {
        gfx_vline(x, y - h, h, RGB565(255, 245, 230));
        for (k = 0; k < 4; k++)
            gfx_pixel(x - 4 + (fx_hash(k, d->t / 8) % 9), y - ((w->tick * 2 + k * 23) % h), RGB565(255, 90, 60));
    }
    fx_ellipse(x, y + 2, 6 + pulse / 8, c);
}

static void draw_drop(const World *w, const Drop *d)
{
    int x = FX_TO_INT(d->x) - cam_x, y = FX_TO_INT(d->y) - cam_y;
    int pop = d->t < 12 ? (12 - d->t) * (12 - d->t) / 12 : 0;   /* the item pops out of the kill */
    if (d->item.rarity >= RAR_LEGEND)
        draw_beam(w, d, x, y);
    gfx_blit(spr_icon(d->item.slot, d->item.rarity), x - 4, y - 4 - pop + ((d->t >> 4) & 1), 0);
    if (d->item.rarity >= RAR_RARE && dist_px(w->h.x, w->h.y, d->x, d->y) < 70) {
        char name[64];
        int tw;
        item_name(name, sizeof name, &d->item);
        tw = MIN(font_text_width(name, 1), 130);
        gfx_dim_rect(x - tw / 2 - 1, y - 16, tw + 2, 9);
        font_draw_fit(x - tw / 2, y - 15, name, tw, rarity_color((Rarity)d->item.rarity));
    }
}

static void draw_ground_items(const World *w)
{
    int i;
    for (i = 0; i < MAX_CORPSE; i++)
        if (w->co[i].alive && visible_px(w, w->co[i].x, w->co[i].y)) {
            int x = FX_TO_INT(w->co[i].x) - cam_x, y = FX_TO_INT(w->co[i].y) - cam_y;
            gfx_fill_rect(x - 4, y + 2, 8, 2, RGB565(110, 30, 30));
            gfx_hline(x - 3, y + 1, 6, RGB565(200, 190, 160));
        }
    for (i = 0; i < MAX_ORB; i++)
        if (w->orb[i].alive && visible_px(w, w->orb[i].x, w->orb[i].y)) {
            int x = FX_TO_INT(w->orb[i].x) - cam_x, y = FX_TO_INT(w->orb[i].y) - cam_y - ((w->tick >> 3) & 1);
            gfx_fill_rect(x - 2, y - 3, 5, 5, RGB565(210, 30, 50));
            gfx_pixel(x - 1, y - 2, RGB565(255, 160, 170));
            gfx_hline(x - 1, y - 5, 3, RGB565(230, 220, 200));
        }
}

/* --------------------------------------------------------------- sorting */

static int cmp_items(const void *a, const void *b)
{
    return ((const DrawItem *)a)->y - ((const DrawItem *)b)->y;
}

static int collect(const World *w, DrawItem *items)
{
    int n = 0, i;
    for (i = 0; i < MAX_DROP; i++)
        if (w->dr[i].alive && w->seen[px_to_cell(w->dr[i].y)][px_to_cell(w->dr[i].x)])
            items[n].y = (int16_t)(FX_TO_INT(w->dr[i].y) - 8), items[n].kind = DI_DROP, items[n++].idx = (uint8_t)i;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && visible_px(w, w->mon[i].x, w->mon[i].y))
            items[n].y = (int16_t)FX_TO_INT(w->mon[i].y), items[n].kind = DI_MON, items[n++].idx = (uint8_t)i;
    for (i = 0; i < MAX_PROJ; i++)
        if (w->pj[i].alive)
            items[n].y = (int16_t)(FX_TO_INT(w->pj[i].y) + 4), items[n].kind = DI_PROJ, items[n++].idx = (uint8_t)i;
    for (i = 0; i < MAX_ALLY; i++)
        if (w->al[i].alive)
            items[n].y = (int16_t)FX_TO_INT(w->al[i].y), items[n].kind = DI_ALLY, items[n++].idx = (uint8_t)i;
    items[n].y = (int16_t)FX_TO_INT(w->h.y);
    items[n].kind = DI_HERO;
    items[n++].idx = 0;
    return n;
}

static void draw_entities(const World *w, const Profile *p)
{
    DrawItem items[MAX_MON + MAX_DROP + MAX_PROJ + MAX_ALLY + 1];
    int n = collect(w, items), i;
    qsort(items, (size_t)n, sizeof items[0], cmp_items);
    for (i = 0; i < n; i++) {
        const Proj *pj = &w->pj[items[i].idx];
        switch (items[i].kind) {
        case DI_DROP: draw_drop(w, &w->dr[items[i].idx]); break;
        case DI_MON:  draw_monster(w, &w->mon[items[i].idx]); break;
        case DI_PROJ: fx_draw_proj(pj, ipx(pj->px, pj->x) - cam_x, ipx(pj->py, pj->y) - cam_y); break;
        case DI_ALLY: draw_ally(w, &w->al[items[i].idx]); break;
        default:      draw_hero(w, p); break;
        }
    }
}

static void draw_grounds(const World *w)
{
    int i;
    for (i = 0; i < MAX_GROUND; i++)
        if (w->gr[i].alive)
            fx_draw_ground(&w->gr[i], cam_x, cam_y);
}

static void draw_overlays(const World *w)
{
    int i;
    for (i = 0; i < MAX_FX; i++)
        if (w->fx[i].alive)
            fx_draw_effect(&w->fx[i], cam_x, cam_y);
}

static void draw_floaters(const World *w)
{
    int i;
    for (i = 0; i < MAX_FLOAT; i++)
        if (w->fl[i].alive) {
            const Floater *f = &w->fl[i];
            int scale = f->kind == FL_BIG ? 2 : 1;
            int x = f->x - cam_x - font_text_width(f->text, scale) / 2, y = f->y - cam_y - f->t / 2;
            font_draw(x + 1, y + 1, f->text, 0, scale);
            font_draw(x, y, f->text, f->color, scale);
        }
}

void render_world(const World *w, const Profile *p)
{
    tiles_build(w->theme); /* no-op unless the depth theme changed */
    compute_camera(w);
    draw_map(w);
    draw_ground_items(w);
    draw_grounds(w);
    render_event_object(w, cam_x, cam_y);
    draw_entities(w, p);
    draw_overlays(w);
    draw_floaters(w);
}
