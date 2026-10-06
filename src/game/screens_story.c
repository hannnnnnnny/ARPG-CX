/*
 * screens_story.c - the story: subtitles over the running battle (the
 * default, nothing ever waits for a key), full pages when the player
 * turns "story pages" on or rereads a chapter from the journal.
 */
#include "game.h"
#include "story.h"
#include "../gfx/font.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define C_TEXT  RGB565(230, 220, 200)
#define C_DIM   RGB565(130, 118, 104)
#define C_SEL   RGB565(255, 200, 90)
#define C_TITLE RGB565(255, 120, 50)
#define C_SHADE RGB565(30, 8, 4)

#define STORY_CPS 2                 /* characters revealed per tick (typewriter) */
#define SUB_TITLE_TICKS (2 * TICK_HZ)
#define SUB_LINE_TICKS (TICK_HZ * 5 / 2)

const char *const *story_page(int id, const char **title)
{
    if (id >= STORY_EPILOGUE) {
        *title = "EPILOGUE";
        return epilogue;
    }
    if (id >= STORY_VICTORY) {
        *title = act_defs[(id - STORY_VICTORY) % ACT_COUNT].boss_name;
        return act_defs[(id - STORY_VICTORY) % ACT_COUNT].victory;
    }
    *title = act_defs[id % ACT_COUNT].title;
    return act_defs[id % ACT_COUNT].intro;
}

static int line_count(const char *const *l)
{
    int n = 0;
    while (n < STORY_LINES && l[n])
        n++;
    return n;
}

/* ------------------------------------------------------------ subtitles */

void subtitle_tick(Game *g)
{
    const char *title;
    if (g->sub_event < 0)
        return;
    g->sub_t++;
    if (g->sub_t > SUB_TITLE_TICKS + line_count(story_page(g->sub_event, &title)) * SUB_LINE_TICKS)
        g->sub_event = -1;          /* the next queued event starts next tick */
}

void subtitle_render(const Game *g)
{
    const char *title;
    const char *const *l;
    int line, y = 166;
    if (g->sub_event < 0)
        return;
    l = story_page(g->sub_event, &title);
    gfx_dim_rect(0, y - 4, SCREEN_W, 34);
    font_draw_centered(y, title, g->sub_event >= STORY_VICTORY ? C_SEL : C_TITLE, C_SHADE, 1);
    if (g->sub_t < SUB_TITLE_TICKS)
        return;
    line = (g->sub_t - SUB_TITLE_TICKS) / SUB_LINE_TICKS;
    if (line < line_count(l))
        font_draw_centered(y + 12, l[line], C_TEXT, C_SHADE, 1);
}

/* ----------------------------------------------------------- full pages */

static int story_total_chars(int id)
{
    const char *title;
    const char *const *l = story_page(id, &title);
    int i, n = 0;
    for (i = 0; i < line_count(l); i++) {
        const char *p = T(l[i]);
        while (*p++) n++;
    }
    return n;
}

void story_tick(Game *g, Input *in)
{
    bool done = g->story_t * STORY_CPS >= story_total_chars(g->story_id);
    g->story_t++;
    if (!in_ok(in) && !in_back(in))
        return;
    if (!done) {
        g->story_t = 10000; /* first press reveals everything */
    } else {
        g->state = g->from_journal ? GS_MENU : GS_BATTLE;
        input_block_held(in);
    }
}

void story_render(Game *g)
{
    const char *title;
    const char *const *l = story_page(g->story_id, &title);
    int budget = g->story_t * STORY_CPS, i, y = 64;
    char buf[128];
    title_backdrop(g->tick);
    gfx_dim_rect(0, 0, SCREEN_W, SCREEN_H);
    font_draw_centered(26, title, g->story_id >= STORY_VICTORY ? C_SEL : C_TITLE, C_SHADE, 2);
    gfx_hline(40, 48, 240, RGB565(120, 60, 30));
    for (i = 0; i < line_count(l) && budget > 0; i++, y += 14) {
        const char *line = T(l[i]);
        int n = 0;
        /* reveal by bytes but never split a UTF-8 character */
        while (line[n] && n < (int)sizeof buf - 1 && (n < budget || ((uint8_t)line[n] & 0xC0) == 0x80)) {
            buf[n] = line[n];
            n++;
        }
        buf[n] = 0;
        budget -= n;
        font_draw((SCREEN_W - font_text_width(line, 1)) / 2, y, buf, C_TEXT, 1);
    }
    if (budget > 0)
        font_draw_centered(176, "PRESS ENTER", (g->tick >> 4) & 1 ? C_SEL : C_DIM, C_SHADE, 1);
}

/* -------------------------------------------------------------- journal */

int journal_events(const Profile *p, int out[16])
{
    int n = 0, act;
    for (act = 0; act < ACT_COUNT; act++) {
        if (story_seen(p->story_seen, story_seen_bit(STORY_INTRO + act)))
            out[n++] = STORY_INTRO + act;
        if (story_seen(p->story_seen, story_seen_bit(STORY_VICTORY + act)))
            out[n++] = STORY_VICTORY + act;
    }
    if (story_seen(p->story_seen, story_seen_bit(STORY_EPILOGUE)))
        out[n++] = STORY_EPILOGUE;
    return n;
}
