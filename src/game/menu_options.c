/*
 * menu_options.c - EMBERS page (rebirth shop) and OPTIONS page (automation,
 * damage numbers, story mode, appearance, journal, saving).
 */
#include "menu_int.h"
#include "balance.h"
#include "paragon.h"
#include "story.h"
#include "../gfx/font.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

static const char *const up_names[UP_COUNT] = { "MIGHT", "VIGOR", "GREED", "WISDOM", "FORTUNE", "HASTE",
                                                "HEAD START", "PATIENCE" };
static const char *const up_desc[UP_COUNT] = {
    "[X]12% DAMAGE", "+15% LIFE", "+20% GOLD", "+20% EXPERIENCE",
    "RARER LOOT", "+5% ATTACK AND MOVE", "+5 STARTING FLOOR", "+4H OFFLINE CAP",
};

/* -------------------------------------------------------------- embers */

void embers_tick(Game *g, Input *in)
{
    int *sel = &g->sel[PG_EMBERS], i;
    menu_move(in, sel, UP_COUNT + 1);
    if (!in_ok(in))
        return;
    i = *sel;
    if (i == UP_COUNT) {
        if (prog_can_rebirth(&g->p))
            g->confirm = CF_REBIRTH;
        else
            game_toast(g, "REACH FLOOR 20 TO REBIRTH", C_BAD);
    } else if (prog_buy_upgrade(&g->p, (UpgradeId)i)) {
        game_profile_changed(g);
        game_toast(g, "UPGRADE BOUGHT", C_SEL);
    } else {
        game_toast(g, "NOT ENOUGH EMBERS", C_BAD);
    }
}

void embers_render(Game *g)
{
    char buf[128], n[16], c[16];
    int i, sel = g->sel[PG_EMBERS];
    fmt_num(n, sizeof n, g->p.embers);
    snprintf(buf, sizeof buf, T("EMBERS: %s    REBIRTHS: %d"), n, g->p.rebirths);
    font_draw(6, 20, buf, RGB565(255, 120, 60), 1);
    for (i = 0; i < UP_COUNT; i++) {
        int y = 36 + i * 16;
        bool maxed = g->p.up[i] >= upgrade_max((UpgradeId)i);
        menu_row_highlight(y, 15, i == sel);
        snprintf(buf, sizeof buf, "%s %d", T(up_names[i]), g->p.up[i]);
        font_draw(6, y, buf, C_TEXT, 1);
        font_draw(110, y, up_desc[i], C_DIM, 1);
        fmt_num(c, sizeof c, upgrade_cost(g->p.up[i]));
        snprintf(buf, sizeof buf, "%s", maxed ? "MAX" : c);
        font_draw(314 - font_text_width(buf, 1), y, buf,
                  maxed ? C_DIM : g->p.embers >= upgrade_cost(g->p.up[i]) ? C_GOOD : C_BAD, 1);
    }
    menu_row_highlight(36 + UP_COUNT * 16, 15, sel == UP_COUNT);
    fmt_num(n, sizeof n, ember_reward(g->p.best_floor));
    if (prog_can_rebirth(&g->p))
        snprintf(buf, sizeof buf, T("REBIRTH NOW: +%s EMBERS (BEST FLOOR %d)"), n, g->p.best_floor);
    else
        snprintf(buf, sizeof buf, T("REBIRTH UNLOCKS AT FLOOR %d (BEST %d)"), REBIRTH_MIN_FLOOR, g->p.best_floor);
    font_draw(6, 36 + UP_COUNT * 16, buf, prog_can_rebirth(&g->p) ? C_SEL : C_DIM, 1);
    font_draw(6, 182, "REBIRTH RESETS LEVEL, GEAR, GOLD AND FLOOR.", C_DIM, 1);
    font_draw(6, 192, "EMBERS, UPGRADES, CODEX AND LOOK STAY.", C_DIM, 1);
    menu_footer(g, "ENTER: BUY / REBIRTH   ESC: BATTLE");
}

/* ------------------------------------------------------------- options */

enum {
    OPT_LANG, OPT_EQUIP, OPT_SALVAGE, OPT_MODE, OPT_SKILLS, OPT_PARAGON, OPT_CRAFT, OPT_DMGNUM, OPT_STORY, OPT_HELM,
    OPT_LOOK, OPT_JOURNAL, OPT_RESET_PARA, OPT_POWER, OPT_FPS, OPT_SAVE, OPT_EXIT, OPT_COUNT
};
#define OPT_VISIBLE 12
#define JOURNAL_VISIBLE 11

static void journal_tick(Game *g, Input *in)
{
    int ev[JOURNAL_MAX], n = journal_events(&g->p, ev);
    menu_move(in, &g->journal_sel, MAX(n, 1));
    if (in_back(in)) {
        g->journal = false;
        input_block_held(in);
    } else if (in_ok(in) && n > 0) {
        g->story_id = ev[g->journal_sel % n];
        g->story_t = 0;
        g->from_journal = true;
        g->state = GS_STORY;
        input_block_held(in);
    }
}

static void toggle(Profile *p, int opt)
{
    switch (opt) {
    case OPT_LANG:    p->lang = (uint8_t)((p->lang + 1) % LANG_COUNT); lang_set(p->lang); break;
    case OPT_EQUIP:   p->auto_equip = !p->auto_equip; break;
    case OPT_SALVAGE: p->salvage_upto = p->salvage_upto == 255 ? RAR_COMMON
                                      : p->salvage_upto >= RAR_RARE ? 255 : (uint8_t)(p->salvage_upto + 1); break;
    case OPT_MODE:    p->mode = p->mode == MODE_PUSH ? MODE_FARM : MODE_PUSH; break;
    case OPT_SKILLS:  p->auto_skills = !p->auto_skills; if (p->auto_skills) build_auto_spend(p); break;
    case OPT_PARAGON: p->auto_paragon = !p->auto_paragon; if (p->auto_paragon) paragon_auto(p); break;
    case OPT_CRAFT:   p->auto_craft = !p->auto_craft; break;
    case OPT_DMGNUM:  p->dmg_numbers = (uint8_t)((p->dmg_numbers + 1) % DMGNUM_COUNT); break;
    case OPT_STORY:   p->story_pause = !p->story_pause; break;
    case OPT_HELM:    p->look.show_helm = !p->look.show_helm; break;
    case OPT_POWER:   p->low_power = !p->low_power; break;
    case OPT_FPS:     p->show_fps = !p->show_fps; break;
    default: break;
    }
}

void options_tick(Game *g, Input *in, uint32_t now)
{
    int sel;
    if (g->journal) {
        journal_tick(g, in);
        return;
    }
    menu_move(in, &g->sel[PG_OPTIONS], OPT_COUNT);
    if (!in_ok(in))
        return;
    sel = g->sel[PG_OPTIONS];
    if (sel == OPT_LOOK) {
        create_open(g, true);
    } else if (sel == OPT_JOURNAL) {
        g->journal = true;
        g->journal_sel = 0;
    } else if (sel == OPT_RESET_PARA) {
        g->confirm = CF_RESET_PARAGON;
    } else if (sel == OPT_SAVE) {
        game_save(g, now);
        game_toast(g, g->save_status == SAVE_OK ? "SAVED" : "SAVE FAILED", g->save_status == SAVE_OK ? C_GOOD : C_BAD);
    } else if (sel == OPT_EXIT) {
        game_save(g, now);
        game_scan_slots(g);
        g->state = GS_TITLE;
    } else {
        toggle(&g->p, sel);
        game_profile_changed(g);
    }
    input_block_held(in);
}

static void option_value(const Profile *p, int opt, char *v, size_t cap)
{
    static const char *const salv[] = { "COMMON", "MAGIC", "RARE" };
    static const char *const dmg[DMGNUM_COUNT] = { "ALL", "BIG HITS", "OFF" };
    v[0] = '\0';
    switch (opt) {
    case OPT_LANG:    snprintf(v, cap, "%s", lang_name(p->lang)); break;
    case OPT_EQUIP:   snprintf(v, cap, "%s", p->auto_equip ? "ON" : "OFF"); break;
    case OPT_SALVAGE: snprintf(v, cap, "%s", p->salvage_upto == 255 ? "OFF" : salv[MIN(p->salvage_upto, 2)]); break;
    case OPT_MODE:    snprintf(v, cap, "%s", p->mode == MODE_PUSH ? "PUSH" : "FARM"); break;
    case OPT_SKILLS:  snprintf(v, cap, "%s", p->auto_skills ? "ON" : "OFF"); break;
    case OPT_PARAGON: snprintf(v, cap, "%s", p->auto_paragon ? "ON" : "OFF"); break;
    case OPT_CRAFT:   snprintf(v, cap, "%s", p->auto_craft ? "ON" : "OFF"); break;
    case OPT_DMGNUM:  snprintf(v, cap, "%s", dmg[p->dmg_numbers % DMGNUM_COUNT]); break;
    case OPT_STORY:   snprintf(v, cap, "%s", p->story_pause ? "FULL PAGES" : "SUBTITLES"); break;
    case OPT_HELM:    snprintf(v, cap, "%s", p->look.show_helm ? "ON" : "OFF"); break;
    case OPT_POWER:   snprintf(v, cap, "%s", p->low_power ? "ON" : "OFF"); break;
    case OPT_FPS:     snprintf(v, cap, "%s", p->show_fps ? "ON" : "OFF"); break;
    default: break;
    }
}

static void damage_legend(void)
{
    font_draw(6, 178, "NUMBERS:", C_DIM, 1);
    font_draw(60, 178, "NORMAL", RGB565(240, 240, 240), 1);
    font_draw(102, 178, "CRIT", RGB565(255, 220, 50), 1);
    font_draw(132, 178, "VULNERABLE", RGB565(200, 110, 255), 1);
    font_draw(198, 178, "CRIT+VULN", RGB565(255, 100, 210), 1);
    font_draw(60, 188, "OVERPOWER", RGB565(90, 220, 255), 1);
    font_draw(120, 188, "OVERPOWER+CRIT", RGB565(255, 150, 40), 1);
    font_draw(214, 188, "DOT = ELEMENT", RGB565(130, 230, 80), 1);
}

static void journal_render(Game *g)
{
    int ev[JOURNAL_MAX], n = journal_events(&g->p, ev), i;
    int sel = g->journal_sel % MAX(n, 1), top = menu_scroll(sel, n, JOURNAL_VISIBLE);
    char buf[64];
    snprintf(buf, sizeof buf, T("LOST PAGES %d/%d"), story_lore_found(&g->p), LORE_COUNT);
    font_draw(6, 20, "JOURNAL - CHAPTERS SO FAR", C_SEL, 1);
    font_draw(SCREEN_W - 6 - font_text_width(buf, 1), 20, buf, C_DIM, 1);
    if (n == 0)
        font_draw(6, 36, "NOTHING YET. DESCEND!", C_DIM, 1);
    for (i = top; i < n && i < top + JOURNAL_VISIBLE; i++) {
        const char *title;
        int y = 34 + (i - top) * 14;
        story_page(ev[i], &title);
        menu_row_highlight(y, 13, i == sel);
        font_draw(12, y, title, ev[i] >= STORY_LORE ? RGB565(200, 170, 120) : ev[i] >= STORY_VICTORY ? C_SEL : C_TEXT, 1);
    }
    menu_footer(g, "ENTER: READ   ESC: BACK");
}

void options_render(Game *g)
{
    static const char *const names[OPT_COUNT] = {
        "LANGUAGE", "AUTO EQUIP UPGRADES", "AUTO SALVAGE UP TO", "FLOOR MODE", "AUTO SPEND SKILLS", "AUTO PARAGON",
        "AUTO CRAFT (FORGE, GEMS, ELIXIRS)", "DAMAGE NUMBERS", "STORY", "SHOW HELM", "APPEARANCE...", "JOURNAL...",
        "RESET PARAGON BOARD", "LOW POWER (15 FPS)", "FPS COUNTER", "SAVE NOW", "SAVE AND EXIT TO TITLE",
    };
    char v[48], buf[128];
    int sel = g->sel[PG_OPTIONS], top = menu_scroll(sel, OPT_COUNT, OPT_VISIBLE), i;
    if (g->journal) {
        journal_render(g);
        return;
    }
    for (i = 0; i < OPT_VISIBLE && top + i < OPT_COUNT; i++) {
        int y = 20 + i * 12, o = top + i;
        menu_row_highlight(y, 12, o == sel);
        font_draw(6, y, names[o], C_TEXT, 1);
        option_value(&g->p, o, v, sizeof v);
        font_draw(314 - font_text_width(v, 1), y, v, C_SEL, 1);
    }
    snprintf(buf, sizeof buf, T("KILLS %.0f   PLAYED %dH %02dM   BEST FLOOR %d"), g->p.total_kills,
             (int)(g->p.play_seconds / 3600), (int)(g->p.play_seconds / 60) % 60, MAX(g->p.best_floor_ever,
             g->p.best_floor));
    font_draw(6, 166, buf, C_DIM, 1);
    damage_legend();
    if (g->save_status == SAVE_IO_ERROR)
        font_draw(6, 156, "WARNING: LAST SAVE FAILED", C_BAD, 1);
    menu_footer(g, "ENTER: CHANGE   ESC: BATTLE");
}
