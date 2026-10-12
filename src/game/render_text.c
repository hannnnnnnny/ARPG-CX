/*
 * render_text.c - floating combat text (damage numbers, barks, pickups).
 *
 * A burst of hits used to print its numbers on top of each other, so two
 * big crits read as one smeared number. Numbers are now laid out youngest
 * first: the newest keeps its place and older ones that would cover it
 * move up a line; one that runs out of room above the battle view is
 * dropped for this frame rather than drawn over the status lines.
 */
#include "render_fx.h"
#include "../gfx/font.h"

#define STACK_TRIES 6   /* lines an older number may climb before giving way */
#define TEXT_TOP 22     /* first logical row below the two status lines */

static bool boxes_overlap(const TextBox *a, const TextBox *b)
{
    return a->x < b->x + b->w && b->x < a->x + a->w && a->y < b->y + b->h && b->y < a->y + a->h;
}

/* Index of the first shown box before 'i' that 'b' covers, or -1. */
static int first_overlap(const TextBox *boxes, int i, const TextBox *b)
{
    int j;
    for (j = 0; j < i; j++)
        if (boxes[j].shown && boxes_overlap(b, &boxes[j]))
            return j;
    return -1;
}

void fx_stack_boxes(TextBox *boxes, int n, int min_y)
{
    int i, k;
    for (i = 0; i < n; i++) {
        TextBox *b = &boxes[i];
        int hit = first_overlap(boxes, i, b);
        for (k = 0; hit >= 0 && k < STACK_TRIES; k++) {
            b->y = (int16_t)(boxes[hit].y - b->h);
            hit = first_overlap(boxes, i, b);
        }
        b->shown = hit < 0 && b->y >= min_y;
    }
}

static int floater_scale(const Floater *f)
{
    return f->kind == FL_MEGA ? (f->t < 5 ? 3 : 2) : f->kind == FL_BIG ? 2 : 1;
}

static void draw_floater(const Floater *f, int x, int y, int scale)
{
    if (f->kind == FL_MEGA) {              /* signature hits: a thick outline that pops */
        font_draw(x - 1, y, f->text, 0, scale);
        font_draw(x + 2, y, f->text, 0, scale);
        font_draw(x, y - 1, f->text, 0, scale);
        font_draw(x, y + 2, f->text, 0, scale);
    }
    font_draw(x + 1, y + 1, f->text, 0, scale);
    font_draw(x, y, f->text, f->color, scale);
}

/* Live floaters, youngest (smallest t) first. */
static int youngest_first(const World *w, uint8_t *order)
{
    int n = 0, i, j;
    for (i = 0; i < MAX_FLOAT; i++)
        if (w->fl[i].alive) {
            for (j = n++; j > 0 && w->fl[order[j - 1]].t > w->fl[i].t; j--)
                order[j] = order[j - 1];
            order[j] = (uint8_t)i;
        }
    return n;
}

void render_floaters(const World *w, int cam_x, int cam_y)
{
    static TextBox boxes[MAX_FLOAT];
    uint8_t order[MAX_FLOAT];
    int n = youngest_first(w, order), i;
    for (i = 0; i < n; i++) {
        const Floater *f = &w->fl[order[i]];
        int scale = floater_scale(f), tw = font_text_width(f->text, scale);
        boxes[i].x = (int16_t)(f->x - cam_x - tw / 2);
        boxes[i].y = (int16_t)(f->y - cam_y - f->t / 2);
        boxes[i].w = (int16_t)tw;
        boxes[i].h = (int16_t)(FONT_H * scale + 1);
    }
    fx_stack_boxes(boxes, n, TEXT_TOP);
    for (i = n - 1; i >= 0; i--)          /* oldest first, so the newest ends on top */
        if (boxes[i].shown)
            draw_floater(&w->fl[order[i]], boxes[i].x, boxes[i].y, floater_scale(&w->fl[order[i]]));
}
