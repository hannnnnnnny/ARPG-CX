#include "game.h"
#include "render.h"
#include "build.h"
#include "paragon.h"
#include "skills.h"
#include "../core/platform.h"
#include "../i18n/i18n.h"
#include "../gfx/font.h"
#include "../gfx/gfx.h"
#include <stdio.h>
#include <string.h>

#define AUTOSAVE_TICKS (120 * TICK_HZ)

void game_toast(Game *g, const char *text, uint16_t color)
{
    snprintf(g->toast, sizeof g->toast, "%s", text);
    g->toast_color = color;
    g->toast_t = 2 * TICK_HZ;
}

void game_profile_changed(Game *g)
{
    session_profile_changed(&g->s, &g->p);
}

void game_save(Game *g, uint32_t now)
{
    g->p.save_time = now;
    g->save_status = g->save_path[0] ? save_write(&g->p, g->save_path) : SAVE_IO_ERROR;
    g->autosave_t = 0;
}

/* "dir/AshenDepths.sav.tns" -> "dir/AshenDepths2.sav.tns" */
void game_slot_path(const Game *g, int slot, char *out, size_t cap)
{
    const char *base = g->save_base, *dot;
    out[0] = 0;
    if (!base || cap < 8)
        return;
    dot = strstr(base, ".sav");
    if (!dot || (size_t)(dot - base) + 16 > cap)
        return;
    snprintf(out, cap, "%.*s%d%s", (int)(dot - base), base, slot, dot);
}

static void slot_info(SlotInfo *si, const Profile *p)
{
    si->cls = p->cls;
    si->preset = p->preset;
    si->level = p->level;
    si->paragon = p->paragon_level;
    si->floor = p->floor;
    si->best_floor = p->best_floor;
    si->rebirths = p->rebirths;
    si->save_time = p->save_time;
    memcpy(si->name, p->look.name, sizeof si->name);
}

void game_scan_slots(Game *g)
{
    static Profile tmp;   /* large: keep it off the calculator's stack */
    int i;
    g->has_save = false;
    for (i = 0; i < SAVE_SLOTS; i++) {
        char path[300];
        SlotInfo *si = &g->slots[i];
        memset(si, 0, sizeof *si);
        game_slot_path(g, i + 1, path, sizeof path);
        si->status = path[0] ? save_load(&tmp, path) : SAVE_MISSING;
        si->used = si->status == SAVE_OK;
        if (!si->used)
            continue;
        slot_info(si, &tmp);
        g->has_save = true;
    }
}

void game_select_slot(Game *g, int slot)
{
    g->slot = CLAMP(slot, 1, SAVE_SLOTS);
    game_slot_path(g, g->slot, g->save_path, sizeof g->save_path);
}

bool game_load_slot(Game *g, int slot)
{
    game_select_slot(g, slot);
    g->save_status = g->save_path[0] ? save_load(&g->p, g->save_path) : SAVE_MISSING;
    if (g->save_status != SAVE_OK)
        return false;
    lang_set(g->p.lang);
    if (g->p.auto_skills)
        build_auto_spend(&g->p);   /* converted saves come with refunded points */
    if (g->p.auto_paragon)
        paragon_auto(&g->p);
    return true;
}

/* First run after the slot update: the old single save becomes slot 1. */
static void migrate_legacy_save(Game *g)
{
    static Profile tmp;
    char path[300];
    game_slot_path(g, 1, path, sizeof path);
    if (!g->save_base || !path[0] || save_load(&tmp, path) != SAVE_MISSING)
        return;
    if (save_load(&tmp, g->save_base) == SAVE_OK)
        save_write(&tmp, path);    /* the old file is left untouched */
}

void game_init(Game *g, const char *save_path, uint32_t now)
{
    int i, latest = 0;
    memset(g, 0, sizeof *g);
    g->save_base = save_path;
    g->upgrade_pick = -1;
    g->sub_event = -1;
    rng_seed(&g->craft_rng, now ^ 0xC4AFu);
    migrate_legacy_save(g);
    game_scan_slots(g);
    /* Continue = the most recently saved slot. */
    for (i = 0; i < SAVE_SLOTS; i++)
        if (g->slots[i].used && (latest == 0 || g->slots[i].save_time > g->slots[latest - 1].save_time))
            latest = i + 1;
    if (latest)
        game_load_slot(g, latest);
    else {
        game_select_slot(g, 1);
        prog_new(&g->p, now ^ 0x5EEDu, CLASS_BARBARIAN);
    }
    g->state = GS_TITLE;
}

/* Story events: subtitles over the battle, or full pages if wanted. */
static void battle_story(Game *g, Input *in)
{
    if (g->s.story_pending < 0)
        return;
    if (g->p.story_pause && g->state == GS_BATTLE) {
        g->story_id = g->s.story_pending;
        g->story_t = 0;
        g->from_journal = false;
        session_story_shown(&g->s, &g->p);
        g->state = GS_STORY;
        input_block_held(in);
    } else if (g->sub_event < 0) {
        g->sub_event = g->s.story_pending;
        g->sub_t = 0;
        session_story_shown(&g->s, &g->p);
    }
}

static void battle_tick(Game *g, Input *in, uint32_t now)
{
    session_tick(&g->s, &g->p);
    subtitle_tick(g);
    if (++g->autosave_t >= AUTOSAVE_TICKS)
        game_save(g, now);
    battle_story(g, in);
    if (g->state != GS_BATTLE)
        return;
    if (in_tab(in) || in_ok(in)) {
        g->state = GS_MENU;
        g->page = PG_HERO;
        input_block_held(in);
    } else if (in_back(in)) {
        g->state = GS_MENU;
        g->page = PG_OPTIONS;
        input_block_held(in);
    }
}

void game_tick(Game *g, Input *in, uint32_t now)
{
    g->tick++;
    if (g->toast_t > 0)
        g->toast_t--;
    if (in_pressed(in, BTN_DEBUG))
        g->p.show_fps = !g->p.show_fps;
    switch (g->state) {
    case GS_TITLE:        title_tick(g, in, now); break;
    case GS_SLOTS:        slots_tick(g, in, now); break;
    case GS_CLASS_SELECT: class_select_tick(g, in, now); break;
    case GS_CREATE:       create_tick(g, in, now); break;
    case GS_STORY:        story_tick(g, in); break;
    case GS_OFFLINE:
        if (g->tick > 20 && (in_ok(in) || in_back(in))) {
            g->state = GS_BATTLE;
            input_block_held(in);
        }
        break;
    case GS_BATTLE:       battle_tick(g, in, now); break;
    case GS_MENU:
        battle_tick(g, in, now); /* the fight goes on behind the menu */
        menu_tick(g, in, now);
        break;
    }
}

static void render_perf(const Game *g)
{
    char buf[48];
    snprintf(buf, sizeof buf, "FPS %d L %d.%dMS R %d.%dMS", g->perf.fps,
             g->perf.logic_us / 1000, (g->perf.logic_us / 100) % 10,
             g->perf.render_us / 1000, (g->perf.render_us / 100) % 10);
    gfx_fill_rect(0, 22, font_text_width(buf, 1) + 4, 18, 0);
    font_draw(2, 23, buf, RGB565(90, 255, 90), 1);
    font_draw(2, 32, plat_clock_desc(), RGB565(90, 255, 90), 1);
}

static void render_toast(const Game *g)
{
    int w = font_text_width(g->toast, 1);
    gfx_fill_rect((SCREEN_W - w) / 2 - 4, 186, w + 8, 12, RGB565(10, 8, 14));
    gfx_rect((SCREEN_W - w) / 2 - 4, 186, w + 8, 12, g->toast_color);
    font_draw((SCREEN_W - w) / 2, 188, g->toast, g->toast_color, 1);
}

void game_render(Game *g)
{
    switch (g->state) {
    case GS_TITLE:        title_render(g); break;
    case GS_SLOTS:        slots_render(g); break;
    case GS_CLASS_SELECT: class_select_render(g); break;
    case GS_CREATE:       create_render(g); break;
    case GS_STORY:        story_render(g); break;
    case GS_OFFLINE:      offline_render(g); break;
    case GS_BATTLE:
        render_world(&g->s.w, &g->p);
        render_hud(&g->s.w, &g->p);
        subtitle_render(g);
        break;
    case GS_MENU:         menu_render(g); break;
    }
    if (g->confirm != CF_NONE)
        confirm_render(g);
    if (g->toast_t > 0)
        render_toast(g);
    if (g->p.show_fps)
        render_perf(g);
}
