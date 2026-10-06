/*
 * menu_skills.c - SKILLS page: the skill tree (10 actives in clusters,
 * 6 passives, 3 key passives), the action bar, build presets (the Diablo
 * IV style "builds": switching is a full respec) and auto spending.
 *
 * ENTER learns the next step of a skill (rank, enhancement, upgrade),
 * DEL opens the upgrade choice, CTRL puts a skill on / off the bar.
 * Doing anything by hand turns the auto planner off.
 */
#include "menu_int.h"
#include "build.h"
#include "skills.h"
#include "../data/icons.h"
#include "../gfx/font.h"
#include "../gfx/sprites.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define ROW_PASSIVE CLASS_SKILLS
#define ROW_KEY (CLASS_SKILLS + CLASS_PASSIVES)
#define ROW_PRESET (ROW_KEY + CLASS_KEYS)
#define ROW_AUTO (ROW_PRESET + 1)
#define ROW_RESET (ROW_PRESET + 2)
#define ROWS (ROW_PRESET + 3)
#define VISIBLE 11

static void go_manual(Game *g)
{
    if (g->p.auto_skills) {
        g->p.auto_skills = 0;
        game_toast(g, "AUTO SPEND OFF: MANUAL BUILD", C_SEL);
    }
}

static void learn_active(Game *g, int i)
{
    const Profile *p = &g->p;
    if (p->skill_rank[i] < SKILL_MAX_RANK && skill_rank_up(&g->p, i)) {
        go_manual(g);
    } else if (p->skill_rank[i] > 0 && !p->skill_enh[i] && skill_enhance(&g->p, i)) {
        go_manual(g);
    } else if (p->skill_enh[i]) {
        g->upgrade_pick = i;
        g->upgrade_sel = p->skill_upg[i] == 2 ? 1 : 0;
        return;
    } else {
        game_toast(g, p->skill_points ? "LOCKED: SPEND MORE POINTS FIRST" : "NO SKILL POINTS", C_BAD);
        return;
    }
    game_profile_changed(g);
}

static void toggle_bar(Game *g, int i)
{
    Profile *p = &g->p;
    int s, target = -1;
    if (skill_on_bar(p, i)) {
        for (s = 0; s < BAR_SLOTS; s++)
            if (p->bar[s] == i)
                p->bar[s] = NO_SKILL;
    } else {
        for (s = BAR_SLOTS - 1; s >= 0; s--)   /* first free slot, else one of the same cluster */
            if (p->bar[s] == NO_SKILL || (target < 0 && skill_def(p->cls, p->bar[s])->cat == skill_def(p->cls, i)->cat))
                target = s;
        if (target < 0 || !skill_bar_set(p, target, i)) {
            game_toast(g, p->skill_rank[i] ? "THE BAR IS FULL" : "LEARN THE SKILL FIRST", C_BAD);
            return;
        }
    }
    go_manual(g);
    game_profile_changed(g);
}

static void upgrade_dialog_tick(Game *g, Input *in)
{
    if (in_up(in) || in_down(in))
        g->upgrade_sel = 1 - g->upgrade_sel;
    if (in_ok(in)) {
        if (skill_set_upgrade(&g->p, g->upgrade_pick, g->upgrade_sel + 1)) {
            go_manual(g);
            game_profile_changed(g);
        } else {
            game_toast(g, "NO SKILL POINTS", C_BAD);
        }
        g->upgrade_pick = -1;
    } else if (in_back(in)) {
        g->upgrade_pick = -1;
    }
    input_block_held(in);
}

static void control_row(Game *g, int row)
{
    if (row == ROW_PRESET) {
        g->preset_pick = (g->p.preset + 1) % PRESETS;
        g->confirm = CF_SWITCH_PRESET;
    } else if (row == ROW_AUTO) {
        g->p.auto_skills = !g->p.auto_skills;
        if (g->p.auto_skills)
            build_auto_spend(&g->p);
        game_profile_changed(g);
    } else {
        g->confirm = CF_RESET_SKILLS;
    }
}

void skills_tick(Game *g, Input *in)
{
    int *sel = &g->sel[PG_SKILLS], row;
    if (g->upgrade_pick >= 0) {
        upgrade_dialog_tick(g, in);
        return;
    }
    menu_move(in, sel, ROWS);
    row = *sel;
    if (row >= ROW_PRESET) {
        if (in_ok(in))
            control_row(g, row);
        return;
    }
    if (row < ROW_PASSIVE) {
        if (in_ok(in)) learn_active(g, row);
        else if (in_alt(in) && g->p.skill_enh[row]) { g->upgrade_pick = row; g->upgrade_sel = 0; }
        else if (in_lock(in)) toggle_bar(g, row);
    } else if (in_ok(in)) {
        bool ok = row < ROW_KEY ? passive_rank_up(&g->p, row - ROW_PASSIVE) : key_passive_set(&g->p, row - ROW_KEY + 1);
        if (ok) {
            go_manual(g);
            game_profile_changed(g);
        } else {
            game_toast(g, g->p.skill_points ? "LOCKED OR MAXED" : "NO SKILL POINTS", C_BAD);
        }
    }
}

/* -------------------------------------------------------------- drawing */

static void row_icon(const Game *g, int row, int x, int y, char *name, size_t cap, char *val, size_t vcap)
{
    const Profile *p = &g->p;
    int cls = p->cls % CLASS_COUNT;
    if (row < ROW_PASSIVE) {
        const SkillDef *d = skill_def(cls, row);
        gfx_blit(spr_skill_icon(d->icon, d->color), x, y, 0);
        if (!p->skill_rank[row])
            gfx_dim_rect(x, y, 16, 16);
        snprintf(name, cap, "%s", d->name);
        snprintf(val, vcap, "%d/%d%s%s", p->skill_rank[row], SKILL_MAX_RANK, p->skill_enh[row] ? "+" : "",
                 p->skill_upg[row] ? (p->skill_upg[row] == 1 ? "A" : "B") : "");
    } else if (row < ROW_KEY) {
        const PassiveDef *d = &passive_defs[cls][row - ROW_PASSIVE];
        gfx_blit(spr_skill_icon(d->icon, RGB565(150, 140, 120)), x, y, 0);
        snprintf(name, cap, "%s", d->name);
        snprintf(val, vcap, "%d/%d", p->passive[row - ROW_PASSIVE], PASSIVE_MAX_RANK);
    } else {
        bool on = p->key_passive == row - ROW_KEY + 1;
        gfx_blit(spr_skill_icon(IC_KEY, on ? C_LEG : RGB565(120, 90, 50)), x, y, 0);
        snprintf(name, cap, "%s", key_defs[cls][row - ROW_KEY].name);
        snprintf(val, vcap, "%s", on ? "ACTIVE" : "KEY");
    }
}

static void list_row(const Game *g, int row, int y, bool on)
{
    char name[64], val[12];
    const char *label;
    if (on)
        gfx_fill_rect(2, y - 1, 160, 18, C_HIL);
    if (row >= ROW_PRESET) {
        label = row == ROW_PRESET ? "SWITCH BUILD..." : row == ROW_AUTO ? (g->p.auto_skills ? "AUTO SPEND: ON"
              : "AUTO SPEND: OFF") : "REFUND ALL POINTS";
        font_draw(8, y + 5, label, C_SEL, 1);
        return;
    }
    row_icon(g, row, 4, y, name, sizeof name, val, sizeof val);
    font_draw_fit(24, y, name, 124, on ? C_SEL : C_TEXT);
    font_draw(24, y + 10, val, row < ROW_PASSIVE && skill_on_bar(&g->p, row) ? C_GOOD : C_DIM, 1);
    if (row < ROW_PASSIVE && skill_on_bar(&g->p, row))
        font_draw(150, y + 5, "B", C_GOOD, 1);
}

static int detail_active(const Game *g, int i, int y)
{
    const SkillDef *d = skill_def(g->p.cls, i);
    const SkillRT *rt = &g->s.w.st.b.skill[i];
    char buf[128];
    int x = 168, w = 148;
    font_draw(x, y, skill_cat_name((SkillCat)d->cat), C_DIM, 1);
    if (skill_points_spent(&g->p) < skill_cat_gate((SkillCat)d->cat)) {
        snprintf(buf, sizeof buf, T("OPENS AT %d POINTS"), skill_cat_gate((SkillCat)d->cat));
        font_draw(x + 70, y, buf, C_BAD, 1);
    }
    y = font_draw_wrapped(x, y + 10, d->desc, w, C_TEXT);
    if (d->behavior == SB_BUFF)
        snprintf(buf, sizeof buf, T("STRENGTH %d%%  RANK %d"), (int)rt->coef, rt->rank);
    else
        snprintf(buf, sizeof buf, T("%d%% WEAPON DMG  RANK %d"), (int)(rt->coef * 100 + 0.5), rt->rank);
    font_draw(x, y, buf, C_SEL, 1);
    if (rt->cost != 0)
        snprintf(buf, sizeof buf, "%s %d", T(rt->cost > 0 ? "COST" : "GENERATES"), (int)(rt->cost > 0 ? rt->cost : -rt->cost));
    else
        snprintf(buf, sizeof buf, T("COOLDOWN %d.%dS"), rt->cd_ticks / TICK_HZ, rt->cd_ticks % TICK_HZ * 10 / TICK_HZ);
    font_draw(x, y + 10, buf, C_DIM, 1);
    font_draw(x, y + 22, d->enh.name, g->p.skill_enh[i] ? C_GOOD : C_DIM, 1);
    y = font_draw_wrapped(x + 4, y + 31, d->enh.desc, w - 4, C_DIM);
    snprintf(buf, sizeof buf, "A: %s", T(d->upg[0].name));
    font_draw(x, y + 2, buf, g->p.skill_upg[i] == 1 ? C_LEG : C_DIM, 1);
    snprintf(buf, sizeof buf, "B: %s", T(d->upg[1].name));
    font_draw(x, y + 12, buf, g->p.skill_upg[i] == 2 ? C_LEG : C_DIM, 1);
    return y + 24;
}

static void details(const Game *g, int row)
{
    int cls = g->p.cls % CLASS_COUNT, x = 168, y = 32;
    char buf[128];
    if (row < ROW_PASSIVE) {
        detail_active(g, row, y);
    } else if (row < ROW_KEY) {
        const PassiveDef *d = &passive_defs[cls][row - ROW_PASSIVE];
        font_draw(x, y, "PASSIVE (PER RANK)", C_DIM, 1);
        font_draw_wrapped(x, y + 12, d->desc, 148, C_TEXT);
    } else if (row < ROW_PRESET) {
        snprintf(buf, sizeof buf, T("KEY PASSIVE - NEEDS %d POINTS"), key_passive_gate());
        font_draw(x, y, buf, C_DIM, 1);
        font_draw_wrapped(x, y + 12, key_defs[cls][row - ROW_KEY].desc, 148, C_LEG);
        font_draw_wrapped(x, y + 60, "ONLY ONE KEY PASSIVE CAN BE ACTIVE. SWITCHING IS FREE.", 148, C_DIM);
    } else {
        const BuildPreset *pr = &class_defs[cls].preset[g->p.preset % PRESETS];
        font_draw(x, y, "CURRENT BUILD", C_DIM, 1);
        font_draw(x, y + 12, pr->name, C_LEG, 1);
        font_draw_wrapped(x, y + 24, pr->desc, 148, C_TEXT);
        font_draw_wrapped(x, y + 60, "SWITCHING BUILD REFUNDS EVERY POINT (FREE BEFORE LEVEL 15).", 148, C_DIM);
    }
}

static void bar_preview(const Game *g)
{
    int s;
    font_draw(168, 182, "ACTION BAR", C_DIM, 1);
    for (s = 0; s < BAR_SLOTS; s++) {
        int i = g->p.bar[s], x = 168 + s * 24;
        gfx_rect(x - 1, 191, 18, 18, C_EDGE);
        if (i != NO_SKILL)
            gfx_blit(spr_skill_icon(skill_def(g->p.cls, i)->icon, skill_def(g->p.cls, i)->color), x, 192, 0);
    }
}

static void upgrade_dialog(const Game *g)
{
    const SkillDef *d = skill_def(g->p.cls, g->upgrade_pick);
    int r;
    gfx_fill_rect(30, 70, 260, 84, RGB565(24, 16, 20));
    gfx_rect(30, 70, 260, 84, C_SEL);
    font_draw(40, 78, "CHOOSE AN UPGRADE FOR", C_DIM, 1);
    font_draw(172, 78, d->name, d->color, 1);
    for (r = 0; r < 2; r++) {
        int y = 96 + r * 22;
        if (g->upgrade_sel == r)
            gfx_fill_rect(36, y - 3, 248, 21, C_HIL);
        font_draw(42, y, d->upg[r].name, g->upgrade_sel == r ? C_SEL : C_TEXT, 1);
        font_draw(52, y + 9, d->upg[r].desc, C_DIM, 1);
    }
    font_draw(40, 142, "UP/DOWN: PICK  ENTER: SET  ESC: CANCEL", C_DIM, 1);
}

void skills_render(Game *g)
{
    int sel = g->sel[PG_SKILLS], top = menu_scroll(sel, ROWS, VISIBLE), i;
    char buf[128];
    snprintf(buf, sizeof buf, T("POINTS %d  SPENT %d"), g->p.skill_points, skill_points_spent(&g->p));
    font_draw(168, 18, buf, g->p.skill_points ? C_SEL : C_DIM, 1);
    for (i = 0; i < VISIBLE && top + i < ROWS; i++)
        list_row(g, top + i, 18 + i * 18, top + i == sel);
    gfx_vline(164, 16, 200, C_EDGE);
    details(g, sel);
    bar_preview(g);
    menu_footer(g, "ENTER: LEARN  DEL: UPGRADE  CTRL: BAR  TAB: PAGE");
    if (g->upgrade_pick >= 0)
        upgrade_dialog(g);
}
