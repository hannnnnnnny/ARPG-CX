/*
 * screens.c - title, save slots, offline report and the yes/no dialog used
 * for every destructive or costly action (new game over a save, rebirth,
 * salvage all, respec, build switch, paragon reset, delete save).
 */
#include "game.h"
#include "render.h"
#include "balance.h"
#include "build.h"
#include "paragon.h"
#include "../gfx/font.h"
#include "../gfx/sprites.h"
#include "../core/bignum.h"
#include "../core/trig.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

#define C_TEXT  RGB565(230, 220, 200)
#define C_DIM   RGB565(130, 118, 104)
#define C_SEL   RGB565(255, 200, 90)
#define C_TITLE RGB565(255, 120, 50)
#define C_SHADE RGB565(30, 8, 4)
#define C_GOOD  RGB565(110, 230, 110)
#define C_BAD   RGB565(240, 90, 80)

void begin_battle(Game *g, uint32_t now, bool fresh)
{
    session_start(&g->s, &g->p);
    world_message(&g->s.w, fresh ? "TAB: MENU   ESC: OPTIONS" : "WELCOME BACK", RGB565(255, 200, 120));
    g->has_save = true;
    game_save(g, now);
}

static void open_slots(Game *g, bool for_new)
{
    game_scan_slots(g);
    g->state = GS_SLOTS;
    g->slot_new = for_new;
    g->slot_sel = g->slot - 1;
}

static void start_new(Game *g)
{
    g->state = GS_CLASS_SELECT;
    g->class_for_rebirth = false;
    g->class_sel = 0;
}

static void start_continue(Game *g, uint32_t now)
{
    bool away = prog_offline(&g->p, now, &g->off);
    begin_battle(g, now, false);
    g->state = away ? GS_OFFLINE : GS_BATTLE;
}

/* ------------------------------------------------------------ confirms */

static void toast_result(Game *g, bool ok, const char *done)
{
    if (ok) {
        game_profile_changed(g);
        game_toast(g, done, C_GOOD);
    } else {
        game_toast(g, "NOT ENOUGH GOLD", C_BAD);
    }
}

static void confirm_salvage(Game *g)
{
    double gold;
    char buf[128], n[16];
    int count = prog_salvage_all_unlocked(&g->p, &gold);
    fmt_num(n, sizeof n, gold);
    snprintf(buf, sizeof buf, T("SALVAGED %d ITEMS FOR %s GOLD"), count, n);
    game_toast(g, buf, C_SEL);
}

static void confirm_delete(Game *g)
{
    char path[300];
    game_slot_path(g, g->slot_sel + 1, path, sizeof path);
    if (path[0])
        remove(path);
    game_scan_slots(g);
    game_toast(g, "SAVE DELETED", C_DIM);
}

static void confirm_yes(Game *g, ConfirmKind k)
{
    switch (k) {
    case CF_NEW_GAME:      start_new(g); break;
    case CF_REBIRTH:
        g->state = GS_CLASS_SELECT;          /* choose the next life's class */
        g->class_for_rebirth = true;
        g->class_sel = g->p.cls;
        break;
    case CF_RESET_SKILLS:  toast_result(g, prog_respec_skills(&g->p), "SKILL POINTS REFUNDED"); break;
    case CF_SWITCH_PRESET: toast_result(g, prog_switch_preset(&g->p, g->preset_pick), "BUILD SWITCHED"); break;
    case CF_RESET_PARAGON: toast_result(g, prog_respec_paragon(&g->p), "PARAGON POINTS REFUNDED"); break;
    case CF_DELETE_SLOT:   confirm_delete(g); break;
    case CF_SALVAGE_ALL:   confirm_salvage(g); break;
    default: break;
    }
}

/* Returns true while a confirmation dialog is consuming input. */
bool screens_confirm_tick(Game *g, Input *in, uint32_t now)
{
    (void)now;
    if (g->confirm == CF_NONE)
        return false;
    if (in_back(in)) {
        g->confirm = CF_NONE;
    } else if (in_ok(in)) {
        ConfirmKind k = g->confirm;
        g->confirm = CF_NONE;
        confirm_yes(g, k);
    }
    input_block_held(in);
    return true;
}

static void confirm_lines(Game *g, const char **l1, const char **l2, char *buf, size_t cap)
{
    char n[16];
    fmt_num(n, sizeof n, prog_respec_cost(&g->p));
    switch (g->confirm) {
    case CF_NEW_GAME:   *l1 = "OVERWRITE THIS SLOT?"; *l2 = "THE HERO SAVED HERE WILL BE ERASED."; break;
    case CF_REBIRTH:
        fmt_num(n, sizeof n, ember_reward(g->p.best_floor));
        snprintf(buf, cap, T("GAIN %s EMBERS. LEVEL, GEAR, GOLD RESET."), n);
        *l1 = "REBIRTH NOW?";
        *l2 = buf;
        break;
    case CF_SALVAGE_ALL: *l1 = "SALVAGE ALL UNLOCKED BAG ITEMS?"; *l2 = "LOCKED ITEMS (CTRL) ARE KEPT."; break;
    case CF_RESET_SKILLS:
    case CF_SWITCH_PRESET:
    case CF_RESET_PARAGON:
        *l1 = g->confirm == CF_RESET_SKILLS ? "REFUND ALL SKILL POINTS?" : g->confirm == CF_SWITCH_PRESET
            ? "SWITCH TO THIS BUILD? (FULL RESPEC)" : "REFUND ALL PARAGON POINTS?";
        snprintf(buf, cap, T("COST: %s GOLD. EVERY POINT COMES BACK."), n);
        *l2 = buf;
        break;
    case CF_DELETE_SLOT: *l1 = "DELETE THIS SAVE?"; *l2 = "THIS HERO WILL BE GONE FOREVER."; break;
    default: break;
    }
}

void confirm_render(Game *g)
{
    const char *line1 = "", *line2 = "";
    char buf[128];
    confirm_lines(g, &line1, &line2, buf, sizeof buf);
    gfx_dim_rect(0, 0, SCREEN_W, SCREEN_H);
    gfx_fill_rect(20, 80, 280, 76, RGB565(24, 16, 20));
    gfx_rect(20, 80, 280, 76, RGB565(255, 120, 50));
    font_draw_centered(92, line1, C_SEL, C_SHADE, 1);
    font_draw_centered(108, line2, C_TEXT, C_SHADE, 1);
    font_draw_centered(134, "ENTER: YES      ESC: NO", C_DIM, C_SHADE, 1);
}

/* --------------------------------------------------------------- title */

enum { TI_CONTINUE, TI_NEW, TI_LOAD, TI_LANG, TI_QUIT, TI_COUNT };

/* LANGUAGE on the title: cycles through the five languages at once. */
static void title_language(Game *g, int step)
{
    g->p.lang = (uint8_t)((g->p.lang + step + LANG_COUNT) % LANG_COUNT);
    lang_set(g->p.lang);
}

void title_tick(Game *g, Input *in, uint32_t now)
{
    if (screens_confirm_tick(g, in, now))
        return;
    {
        int step = in_up(in) ? TI_COUNT - 1 : in_down(in) ? 1 : 0, guard = 0;
        if (step)
            g->title_sel = (g->title_sel + step) % TI_COUNT;
        /* Without a save, CONTINUE and LOAD are skipped in the move direction. */
        while (!g->has_save && (g->title_sel == TI_CONTINUE || g->title_sel == TI_LOAD) && guard++ < TI_COUNT)
            g->title_sel = (g->title_sel + (step ? step : 1)) % TI_COUNT;
    }
    if (in_back(in)) {
        g->quit = true;
        return;
    }
    if (g->title_sel == TI_LANG && (in_left(in) || in_right(in)))
        title_language(g, in_left(in) ? -1 : 1);
    if (!in_ok(in))
        return;
    input_block_held(in);
    switch (g->title_sel) {
    case TI_LANG:     title_language(g, 1); break;
    case TI_CONTINUE: start_continue(g, now); break;
    case TI_NEW:      open_slots(g, true); break;
    case TI_LOAD:     open_slots(g, false); break;
    default:          g->quit = true; break;
    }
}

/* Rising embers over a dark gradient. */
void title_backdrop(int tick)
{
    int y, i;
    for (y = 0; y < SCREEN_H; y++)
        gfx_hline(0, y, SCREEN_W, RGB565(10 + y / 12, 4 + y / 40, 8));
    for (i = 0; i < 40; i++) {
        int x = (i * 73 + isin(tick / 3 + i * 9) / 16) % SCREEN_W;
        int py = SCREEN_H - ((tick * (1 + i % 3) + i * 37) % SCREEN_H);
        gfx_pixel(x, py, i % 3 ? RGB565(255, 120, 40) : RGB565(255, 210, 120));
    }
    gfx_fill_rect(0, 196, SCREEN_W, 44, RGB565(26, 20, 24));
    gfx_hline(0, 196, SCREEN_W, RGB565(80, 60, 50));
}

static void title_heroes(const Game *g)
{
    HeroLook l;
    int c;
    for (c = 0; c < CLASS_COUNT; c++) {     /* the six classes stand guard */
        memset(&l, 0, sizeof l);
        l.cls = (uint8_t)c;
        l.skin = (uint8_t)(c * 2 % SKIN_TONES);
        l.hair = (uint8_t)(c % 6);
        l.hair_color = (uint8_t)(c * 3 % HAIR_COLORS);
        l.eyes = (uint8_t)(c % EYE_COLORS);
        l.cloth = (uint8_t)c;
        l.chest_mat = l.boot_mat = l.pants_mat = (uint8_t)(c % 3 + 2);
        l.chest_tier = (uint8_t)(c % 4);
        gfx_blit(spr_hero_look(&l, 2 + (((g->tick >> 3) + c) & 1)), 30 + c * 46, 210, c & 1 ? BLIT_FLIP_X : 0);
    }
}

void title_render(Game *g)
{
    static const char *const items[TI_COUNT] = { "CONTINUE", "NEW GAME", "LOAD GAME", "", "QUIT" };
    int i;
    char buf[128];
    title_backdrop(g->tick);
    font_draw_centered(22, "ASHEN", C_TITLE, C_SHADE, 5);
    font_draw_centered(62, "DEPTHS", C_SEL, C_SHADE, 5);
    font_draw_centered(104, "AN IDLE DESCENT", C_DIM, C_SHADE, 1);
    for (i = 0; i < TI_COUNT; i++) {
        bool on = g->title_sel == i, enabled = (i != TI_CONTINUE && i != TI_LOAD) || g->has_save;
        char item[64];
        if (i == TI_LANG)                  /* always shown bilingually so anyone can find it */
            snprintf(item, sizeof item, "LANGUAGE / %s", lang_name(g->p.lang));
        else
            snprintf(item, sizeof item, "%s", T(items[i]));
        snprintf(buf, sizeof buf, on ? "> %s <" : "%s", item);
        game_ui_row(116 + i * 12 - 2, 12, on);
        font_draw_centered(116 + i * 12, buf, !enabled ? RGB565(70, 60, 56) : on ? C_SEL : C_TEXT, C_SHADE, 1);
    }
    if (g->has_save) {
        snprintf(buf, sizeof buf, T("SLOT %d: %s, %s LV %d  FLOOR %d"), g->slot, g->p.look.name,
                 T(class_defs[g->p.cls % CLASS_COUNT].name), g->p.level, g->p.floor);
        font_draw_centered(180, buf, C_DIM, C_SHADE, 1);
    } else if (g->save_status == SAVE_CORRUPT) {
        font_draw_centered(180, "SAVE FILE WAS DAMAGED - STARTING FRESH", RGB565(255, 90, 90), C_SHADE, 1);
    }
    title_heroes(g);
}

/* ---------------------------------------------------------- save slots */

void slots_tick(Game *g, Input *in, uint32_t now)
{
    const SlotInfo *si;
    if (screens_confirm_tick(g, in, now))
        return;
    if (in_up(in))   g->slot_sel = (g->slot_sel + SAVE_SLOTS - 1) % SAVE_SLOTS;
    if (in_down(in)) g->slot_sel = (g->slot_sel + 1) % SAVE_SLOTS;
    si = &g->slots[g->slot_sel];
    if (in_back(in)) {
        g->state = GS_TITLE;
        input_block_held(in);
        return;
    }
    if (in_alt(in) && si->used) {
        g->confirm = CF_DELETE_SLOT;
        return;
    }
    if (!in_ok(in))
        return;
    input_block_held(in);
    if (g->slot_new) {
        game_select_slot(g, g->slot_sel + 1);
        if (si->used)
            g->confirm = CF_NEW_GAME;    /* overwriting a hero needs a yes */
        else
            start_new(g);
    } else if (si->used && game_load_slot(g, g->slot_sel + 1)) {
        start_continue(g, now);
    } else {
        game_toast(g, si->status == SAVE_CORRUPT ? "THIS SAVE IS DAMAGED" : "EMPTY SLOT", C_BAD);
    }
}

static void slot_row(const Game *g, int i, int y)
{
    const SlotInfo *si = &g->slots[i];
    const ClassDef *c = &class_defs[si->cls % CLASS_COUNT];
    char buf[128];
    snprintf(buf, sizeof buf, "%s - %s %s", si->name, T(c->name), T(c->preset[si->preset % PRESETS].name));
    font_draw(52, y + 18, buf, C_TEXT, 1);
    if (si->level >= LEVEL_CAP)
        snprintf(buf, sizeof buf, T("PARAGON %d  FLOOR %d  BEST %d  REBIRTHS %d"), si->paragon, si->floor,
                 si->best_floor, si->rebirths);
    else
        snprintf(buf, sizeof buf, T("LEVEL %d  FLOOR %d  BEST %d  REBIRTHS %d"), si->level, si->floor, si->best_floor,
                 si->rebirths);
    font_draw(52, y + 28, buf, C_DIM, 1);
    (void)g;
}

void slots_render(Game *g)
{
    int i;
    char buf[128];
    title_backdrop(g->tick);
    font_draw_centered(14, g->slot_new ? "NEW GAME - CHOOSE A SLOT" : "LOAD GAME", C_SEL, C_SHADE, 2);
    for (i = 0; i < SAVE_SLOTS; i++) {
        const SlotInfo *si = &g->slots[i];
        int y = 44 + i * 46;
        bool on = i == g->slot_sel;
        game_ui_rect(20, y, 280, 40, on);
        gfx_fill_rect(20, y, 280, 40, on ? RGB565(60, 30, 20) : RGB565(20, 12, 14));
        gfx_rect(20, y, 280, 40, on ? C_SEL : RGB565(80, 60, 50));
        snprintf(buf, sizeof buf, T("SLOT %d"), i + 1);
        font_draw(28, y + 6, buf, on ? C_SEL : C_DIM, 1);
        if (si->used)
            slot_row(g, i, y);
        else
            font_draw(52, y + 22, si->status == SAVE_CORRUPT ? "DAMAGED SAVE" : "EMPTY",
                      si->status == SAVE_CORRUPT ? C_BAD : C_DIM, 1);
    }
    font_draw_centered(190, "ENTER: SELECT   DEL: DELETE   ESC: BACK", C_DIM, C_SHADE, 1);
}

/* ------------------------------------------------------------- offline */

void offline_render(Game *g)
{
    char buf[128], n[16];
    const OfflineReport *r = &g->off;
    uint32_t h = r->seconds / 3600, m = (r->seconds / 60) % 60;
    title_backdrop(g->tick);
    font_draw_centered(30, "WHILE YOU WERE AWAY", C_SEL, C_SHADE, 2);
    snprintf(buf, sizeof buf, T("YOUR HERO FOUGHT FOR %uH %02uM"), (unsigned)h, (unsigned)m);
    font_draw_centered(62, buf, C_TEXT, C_SHADE, 1);
    if (r->raw_seconds > r->seconds)
        font_draw_centered(74, "(OFFLINE TIME IS CAPPED - SEE EMBERS: PATIENCE)", C_DIM, C_SHADE, 1);
    fmt_num(n, sizeof n, r->kills);
    snprintf(buf, sizeof buf, T("MONSTERS SLAIN   %s"), n);
    font_draw(70, 96, buf, C_TEXT, 1);
    fmt_num(n, sizeof n, r->gold);
    snprintf(buf, sizeof buf, T("GOLD             %s"), n);
    font_draw(70, 110, buf, RGB565(250, 212, 80), 1);
    fmt_num(n, sizeof n, r->xp);
    snprintf(buf, sizeof buf, T("EXPERIENCE       %s"), n);
    font_draw(70, 124, buf, RGB565(190, 150, 255), 1);
    snprintf(buf, sizeof buf, T("LEVELS GAINED    %d"), r->levels);
    font_draw(70, 138, buf, C_TEXT, 1);
    snprintf(buf, sizeof buf, T("ITEMS KEPT       %d  (SALVAGED %d)"), r->items_kept, r->items_salvaged);
    font_draw(70, 152, buf, C_TEXT, 1);
    font_draw_centered(176, "PRESS ENTER", (g->tick >> 4) & 1 ? C_SEL : C_DIM, C_SHADE, 1);
}
