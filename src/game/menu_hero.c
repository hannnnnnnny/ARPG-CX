/*
 * menu_hero.c - HERO page: the paper doll (ten gear slots around the
 * hero, who wears what is equipped) with the item tooltip, and the
 * character sheet (DEL) listing every Diablo IV style stat plus a
 * breakdown of the biggest hit on this floor.
 */
#include "menu_int.h"
#include "build.h"
#include "items.h"
#include "stats.h"
#include "../gfx/font.h"
#include "../gfx/sprites.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

#define SHEET_MAX 64
#define SHEET_ROWS 19

static const uint8_t doll[2][5] = {
    { SLOT_HELM, SLOT_CHEST, SLOT_GLOVES, SLOT_PANTS, SLOT_BOOTS },
    { SLOT_AMULET, SLOT_RING1, SLOT_RING2, SLOT_WEAPON, SLOT_OFFHAND },
};

typedef struct { char label[64]; char val[24]; uint16_t c; } SheetLine;
static SheetLine g_sheet[SHEET_MAX];
static int g_lines;

/* --------------------------------------------------------- paper doll */

void hero_tick(Game *g, Input *in)
{
    int *sel = &g->sel[PG_HERO];
    int col = *sel / 5, row = *sel % 5;
    if (in_alt(in)) {
        g->hero_stats = !g->hero_stats;
        g->scroll[PG_HERO] = 0;
        return;
    }
    if (g->hero_stats) {
        int top = g->scroll[PG_HERO];
        if (in_up(in)) top--;
        if (in_down(in)) top++;
        g->scroll[PG_HERO] = CLAMP(top, 0, MAX(0, g_lines - SHEET_ROWS));
        return;
    }
    if (in_up(in))    row = (row + 4) % 5;
    if (in_down(in))  row = (row + 1) % 5;
    if (in_left(in) || in_right(in)) col = 1 - col;
    *sel = col * 5 + row;
}

static void slot_box(const Game *g, int slot, int x, int y, bool on)
{
    const Item *it = &g->p.equip[slot];
    gfx_fill_rect(x, y, 20, 20, RGB565(26, 20, 28));
    gfx_rect(x, y, 20, 20, on ? C_SEL : it->used ? rarity_color((Rarity)it->rarity) : RGB565(60, 50, 56));
    if (it->used) {
        gfx_blit_scaled(spr_icon(slot, it->rarity), x + 2, y + 2, 2);
        if (it->ancestral)
            gfx_pixel(x + 17, y + 2, RGB565(255, 80, 60));
    }
}

static void doll_render(Game *g)
{
    HeroLook l;
    const ClassDef *c = &class_defs[g->p.cls % CLASS_COUNT];
    char buf[128];
    int col, row, sel = g->sel[PG_HERO];
    for (col = 0; col < 2; col++)
        for (row = 0; row < 5; row++)
            slot_box(g, doll[col][row], col ? 124 : 6, 22 + row * 24, sel == col * 5 + row);
    hero_look_from(&l, &g->p);
    gfx_blit_scaled(spr_hero_look(&l, (g->tick >> 5) & 1), 51, 30, 3);
    font_draw(76 - font_text_width(g->p.look.name, 1) / 2, 96, g->p.look.name, C_SEL, 1);
    font_draw(76 - font_text_width(c->name, 1) / 2, 106, c->name, C_TEXT, 1);
    snprintf(buf, sizeof buf, "%s", c->preset[g->p.preset % PRESETS].name);
    font_draw(76 - font_text_width(buf, 1) / 2, 116, buf, RGB565(255, 170, 90), 1);
    if (g->p.level >= LEVEL_CAP)
        snprintf(buf, sizeof buf, T("PARAGON %d"), g->p.paragon_level);
    else
        snprintf(buf, sizeof buf, T("LEVEL %d"), g->p.level);
    font_draw(76 - font_text_width(buf, 1) / 2, 128, buf, RGB565(190, 150, 255), 1);
    {
        int slot = doll[sel / 5][sel % 5];
        font_draw(6, 146, slot_name((Slot)slot), C_DIM, 1);
        if (g->p.equip[slot].used)
            menu_item_card(g, &g->p.equip[slot], 152, 20, 164, NULL);
        else
            font_draw(152, 24, "NOTHING EQUIPPED", C_DIM, 1);
    }
    menu_footer(g, "ARROWS: SLOT  DEL: CHARACTER SHEET  TAB: PAGE");
}

/* ------------------------------------------------------ character sheet */

static void line(const char *label, const char *val, uint16_t c)
{
    if (g_lines >= SHEET_MAX)
        return;
    snprintf(g_sheet[g_lines].label, sizeof g_sheet[0].label, "%s", label);
    snprintf(g_sheet[g_lines].val, sizeof g_sheet[0].val, "%s", val);
    g_sheet[g_lines++].c = c;
}

static void pct(const char *label, double v)
{
    char b[22];
    snprintf(b, sizeof b, "%s%d.%d%%", v < 0 ? "-" : "+", (int)(v < 0 ? -v : v), (int)((v < 0 ? -v : v) * 10) % 10);
    line(label, b, C_TEXT);
}

static void mult(const char *label, double x)
{
    char b[22];
    if (x <= 1.0001)
        return;
    snprintf(b, sizeof b, "[X]%d.%02d", (int)x, (int)(x * 100 + 0.5) % 100);
    line(label, b, C_LEG);
}

static void num(const char *label, double v)
{
    char b[22];
    fmt_num(b, sizeof b, v);
    line(label, b, C_TEXT);
}

static void sheet_offense(const Game *g, const Stats *st)
{
    const BuildRT *b = &st->b;
    int e;
    line("-- OFFENSE --", "", C_SEL);
    num("WEAPON DAMAGE", st->weapon);
    num(stat_name((MainStat)class_defs[g->p.cls % CLASS_COUNT].main_stat), st->mainstat);
    pct("DAMAGE FROM MAIN STAT", (st->stat_mult - 1.0) * 100.0);
    pct("ATTACK SPEED BONUS", b->atk_spd);
    pct("CRITICAL STRIKE CHANCE", st->crit * 100.0);
    pct("CRITICAL STRIKE DAMAGE", 50.0 + st->crit_dmg * 100.0);
    pct("VULNERABLE DAMAGE", 20.0 + st->vuln_dmg * 100.0);
    pct("OVERPOWER CHANCE", st->op_chance * 100.0);
    pct("OVERPOWER DAMAGE", 50.0 + st->op_dmg * 100.0);
    pct("LUCKY HIT CHANCE BONUS", st->lucky * 100.0);
    pct("DAMAGE (ADDITIVE)", b->add[ADD_ALL]);
    pct("CORE SKILL DAMAGE", b->add[ADD_CORE]);
    pct("BASIC SKILL DAMAGE", b->add[ADD_BASIC]);
    pct("DAMAGE TO CLOSE", b->add[ADD_CLOSE]);
    pct("DAMAGE TO DISTANT", b->add[ADD_FAR]);
    pct("DAMAGE TO CROWD CONTROLLED", b->add[ADD_CC]);
    pct("DAMAGE TO ELITES", b->add[ADD_ELITE]);
    pct("DAMAGE OVER TIME", b->add[ADD_DOT]);
    for (e = 0; e < EL_COUNT; e++)
        if (b->add[ADD_ELEM0 + e] > 0) {
            char lab[64];
            snprintf(lab, sizeof lab, T("%s DAMAGE"), T(element_name((Element)e)));
            pct(lab, b->add[ADD_ELEM0 + e]);
        }
    mult("[X] DAMAGE", b->x_all);
    mult("[X] VS VULNERABLE", b->x_vuln);
    mult("[X] CRITICAL DAMAGE", b->x_crit);
    mult("[X] OVERPOWER DAMAGE", b->x_op);
    mult("[X] CORE SKILLS", b->x_core);
    mult("[X] DAMAGE OVER TIME", b->x_dot);
    mult("[X] CROWD CONTROLLED", b->x_cc);
    mult("[X] MINIONS", b->x_minion);
}

static void sheet_defense(const Game *g, const Stats *st)
{
    static const char *const res[EL_COUNT] = { "", "FIRE RESIST", "COLD RESIST", "LIGHTNING RESIST",
                                               "POISON RESIST", "SHADOW RESIST" };
    int e;
    line("-- DEFENSE --", "", C_SEL);
    num("MAXIMUM LIFE", st->max_hp);
    num("ARMOR", st->armor);
    pct("DR FROM ARMOR (THIS FLOOR)", (1.0 - (1.0 - stats_damage_reduction(st, g->p.floor)) / (1.0 - st->dr)) * 100.0);
    pct("DAMAGE REDUCTION", st->dr * 100.0);
    pct("DR FROM CLOSE ENEMIES", st->dr_close * 100.0);
    for (e = 1; e < EL_COUNT; e++)
        pct(res[e], st->res[e] * 100.0);
    num("LIFE PER SECOND", st->regen);
    num("LIFE ON HIT", st->life_hit);
    num("LIFE ON KILL", st->life_kill);
    pct("BARRIER GENERATION", st->barrier_gen * 100.0);
    num("THORNS", st->thorns);
}

static void sheet_utility(const Game *g, const Stats *st)
{
    char lab[64];
    snprintf(lab, sizeof lab, T("MAXIMUM %s"), T(resource_name((ResourceKind)class_defs[g->p.cls % CLASS_COUNT].res_kind)));
    line("-- UTILITY --", "", C_SEL);
    num(lab, st->max_res);
    num("RESOURCE PER SECOND", st->res_regen);
    pct("COOLDOWN REDUCTION", st->cdr);
    pct("RESOURCE COST REDUCTION", st->b.cost);
    pct("MOVEMENT SPEED", st->move_pct);
    pct("CROWD CONTROL DURATION", st->cc_dur * 100.0);
    pct("GOLD FOUND", st->gold_pct);
    pct("EXPERIENCE", st->xp_pct);
    num("HEALING POTIONS", st->potion_max);
}

static void sheet_hits(const Game *g)
{
    const World *w = &g->s.w;
    const HitLog *l = &w->last_big;
    char b[22];
    line("-- BIGGEST HIT THIS FLOOR --", "", C_SEL);
    if (l->total <= 0) {
        line("NO HITS YET", "", C_DIM);
        return;
    }
    num("WEAPON X SKILL", l->base);
    snprintf(b, sizeof b, "X%d.%02d", (int)l->stat, (int)(l->stat * 100) % 100); line("MAIN STAT", b, C_TEXT);
    snprintf(b, sizeof b, "X%d.%02d", (int)l->add, (int)(l->add * 100) % 100); line("ADDITIVE BONUSES", b, C_TEXT);
    snprintf(b, sizeof b, "X%d.%02d", (int)l->mult, (int)(l->mult * 100) % 100); line("[X] MULTIPLIERS", b, C_LEG);
    snprintf(b, sizeof b, "X%d.%02d", (int)l->vuln, (int)(l->vuln * 100) % 100);
    line("VULNERABLE", b, l->is_vuln ? RGB565(200, 110, 255) : C_DIM);
    snprintf(b, sizeof b, "X%d.%02d", (int)l->crit, (int)(l->crit * 100) % 100);
    line("CRITICAL STRIKE", b, l->is_crit ? RGB565(255, 220, 50) : C_DIM);
    snprintf(b, sizeof b, "X%d.%02d", (int)l->op, (int)(l->op * 100) % 100);
    line("OVERPOWER", b, l->is_op ? RGB565(90, 220, 255) : C_DIM);
    num("= TOTAL", l->total);
    line("-- THIS FLOOR --", "", C_SEL);
    num("HITS", w->hs.hits);
    pct("CRITICAL HITS", 100.0 * w->hs.crits / MAX(w->hs.hits, 1.0));
    pct("VULNERABLE HITS", 100.0 * w->hs.vulns / MAX(w->hs.hits, 1.0));
    pct("OVERPOWERED HITS", 100.0 * w->hs.ops / MAX(w->hs.hits, 1.0));
}

static void sheet_render(Game *g)
{
    const Stats *st = &g->s.w.st;
    int i, top;
    g_lines = 0;
    sheet_offense(g, st);
    sheet_defense(g, st);
    sheet_utility(g, st);
    sheet_hits(g);
    top = CLAMP(g->scroll[PG_HERO], 0, MAX(0, g_lines - SHEET_ROWS));
    for (i = 0; i < SHEET_ROWS && top + i < g_lines; i++) {
        const SheetLine *l = &g_sheet[top + i];
        int y = 20 + i * 10;
        font_draw(8, y, l->label, l->val[0] ? C_DIM : l->c, 1);
        font_draw(312 - font_text_width(l->val, 1), y, l->val, l->c, 1);
    }
    menu_footer(g, "UP/DOWN: SCROLL   DEL: PAPER DOLL");
}

void hero_render(Game *g)
{
    if (g->hero_stats)
        sheet_render(g);
    else
        doll_render(g);
}
