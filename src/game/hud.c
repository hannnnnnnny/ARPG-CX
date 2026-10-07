/*
 * hud.c - Diablo IV style HUD: life globe (left), resource globe (right),
 * potion and six skill icons in between, buff tags, the top status line,
 * boss bar and loot banners.
 */
#include "render.h"
#include "balance.h"
#include "skills.h"
#include "build.h"
#include "progress.h"
#include "events.h"
#include "../data/icons.h"
#include "../gfx/font.h"
#include "../gfx/tiles.h"
#include "../gfx/sprites.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define PANEL_Y 200
#define C_PANEL  RGB565(18, 14, 22)
#define C_EDGE   RGB565(120, 90, 60)
#define C_TEXT   RGB565(230, 220, 200)
#define C_DIM    RGB565(120, 110, 100)
#define C_GOLD   RGB565(250, 212, 80)
#define GLOBE_R  18

/* A globe filled from the bottom; row by row so it stays a circle. */
static void globe(int cx, int cy, double frac, uint16_t fill, uint16_t dark)
{
    int y, level = cy + GLOBE_R - (int)(2 * GLOBE_R * CLAMP(frac, 0.0, 1.0));
    for (y = -GLOBE_R; y <= GLOBE_R; y++) {
        int half = 0;
        while ((half + 1) * (half + 1) + y * y <= GLOBE_R * GLOBE_R)
            half++;
        gfx_hline(cx - half, cy + y, half * 2 + 1, cy + y >= level ? fill : dark);
    }
    gfx_circle(cx, cy, GLOBE_R, C_EDGE);
    gfx_circle(cx, cy, GLOBE_R + 1, RGB565(50, 36, 26));
    gfx_pixel(cx - 7, cy - 9, RGB565(255, 255, 255));      /* glass highlight */
    gfx_hline(cx - 8, cy - 8, 3, RGB565(220, 220, 230));
}

static void globe_text(int cx, int cy, double v)
{
    char buf[16];
    fmt_num(buf, sizeof buf, v);
    font_draw(cx - font_text_width(buf, 1) / 2 + 1, cy - 2, buf, 0, 1);
    font_draw(cx - font_text_width(buf, 1) / 2, cy - 3, buf, C_TEXT, 1);
}

static void draw_globes(const World *w, const Profile *p)
{
    const ClassDef *c = &class_defs[p->cls % CLASS_COUNT];
    uint16_t rc = c->res_color;
    globe(21, 220, w->h.hp / w->st.max_hp, RGB565(190, 20, 30), RGB565(40, 6, 10));
    if (w->h.barrier > 0)                  /* barrier wraps the life globe in white */
        gfx_circle(21, 220, GLOBE_R - 1, RGB565(230, 235, 255));
    globe_text(21, 220, w->h.hp);
    globe(SCREEN_W - 22, 220, w->h.res / world_max_res(w), rc, (uint16_t)((rc >> 2) & 0x39E7));
    globe_text(SCREEN_W - 22, 220, w->h.res);
}

static void draw_potion(const World *w)
{
    char n[8];
    const Sprite *s = spr_skill_icon(IC_POTION, RGB565(90, 20, 30));
    gfx_blit(s, 42, 203, 0);
    if (w->h.potions == 0)
        gfx_dim_rect(43, 204, 14, 14);
    snprintf(n, sizeof n, "%d", w->h.potions);
    font_draw(50 - font_text_width(n, 1) / 2, 222, n, w->h.potions ? C_TEXT : C_DIM, 1);
}

static void skill_slot(const World *w, const Profile *p, int slot, int x, int y)
{
    int i = p->bar[slot];
    const SkillDef *d;
    const SkillRT *rt;
    gfx_rect(x - 1, y - 1, 18, 18, C_EDGE);
    if (i == NO_SKILL) {
        gfx_fill_rect(x, y, 16, 16, RGB565(26, 22, 30));
        return;
    }
    d = skill_def(p->cls, i);
    rt = &w->st.b.skill[i];
    gfx_blit(spr_skill_icon(d->icon, d->color), x, y, 0);
    if (rt->cost > 0 && w->h.res < rt->cost)            /* not enough resource: blue wash */
        gfx_blend_rect(x, y, 16, 16, RGB565(20, 40, 120));
    if (w->h.skill_cd[i] > 0 && rt->cd_ticks > 0) {
        int hgt = MIN(16, 16 * w->h.skill_cd[i] / rt->cd_ticks);
        char s[12];
        gfx_dim_rect(x, y + 16 - hgt, 16, hgt);
        if (w->h.skill_cd[i] >= TICK_HZ) {
            snprintf(s, sizeof s, "%d", w->h.skill_cd[i] / TICK_HZ + 1);
            font_draw(x + 8 - font_text_width(s, 1) / 2, y + 5, s, C_TEXT, 1);
        }
    }
}

static void draw_skills(const World *w, const Profile *p)
{
    int s;
    for (s = 0; s < BAR_SLOTS; s++)
        skill_slot(w, p, s, 62 + s * 20, 203);
}

static void draw_info(const World *w, const Profile *p)
{
    char a[16], buf[40];
    double need = prog_xp_needed(p);
    if (p->level >= LEVEL_CAP)
        snprintf(buf, sizeof buf, T("P %d"), p->paragon_level);
    else
        snprintf(buf, sizeof buf, T("LV %d"), p->level);
    font_draw(184, 204, buf, RGB565(190, 150, 255), 1);
    fmt_num(a, sizeof a, p->gold);
    snprintf(buf, sizeof buf, "G %s", a);
    font_draw(184, 214, buf, C_GOLD, 1);
    if (p->skill_points > 0 && !p->auto_skills)
        font_draw(232, 204, "SKILL+", RGB565(255, 220, 100), 1);
    gfx_fill_rect(62, 225, 214, 3, RGB565(30, 16, 50));
    gfx_fill_rect(62, 225, (int)(214 * CLAMP(p->xp / need, 0.0, 1.0)), 3, RGB565(170, 110, 255));
    snprintf(buf, sizeof buf, "%s", p->look.name);
    font_draw(62, 230, buf, C_DIM, 1);
    (void)w;
}

/* --------------------------------------------------------------- top */

static int buff_tag(int x, const char *text, uint16_t c)
{
    int tw = font_text_width(text, 1);
    gfx_dim_rect(x - 1, 12, tw + 2, 9);
    font_draw(x, 13, text, c, 1);
    return x + tw + 4;
}

static void draw_buffs(const World *w)
{
    const HeroRT *h = &w->h;
    char buf[48];
    int x = 4;
    if (w->shrine_t > 0) {
        snprintf(buf, sizeof buf, "%s %d", T(shrine_tag(w->shrine)), w->shrine_t / TICK_HZ);
        x = buff_tag(x, buf, RGB565(140, 220, 255));
    }
    if (h->buff_t[BUFF_ULT]) x = buff_tag(x, "ULTIMATE", w->accent);
    if (h->buff_t[BUFF_BERSERK]) x = buff_tag(x, "BERSERKING", RGB565(255, 80, 70));
    if (h->buff_t[BUFF_UNSTOP]) x = buff_tag(x, "UNSTOPPABLE", RGB565(255, 170, 60));
    if (h->barrier > 0) {
        char n[12];
        fmt_num(n, sizeof n, h->barrier);
        snprintf(buf, sizeof buf, T("BARRIER %s"), n);
        x = buff_tag(x, buf, RGB565(220, 230, 255));
    }
    if (h->buff_t[BUFF_IMBUE]) x = buff_tag(x, "IMBUED", element_color((Element)h->imbue_el));
    if (h->buff_t[BUFF_SPEED] || h->haste_t) x = buff_tag(x, "HASTE", RGB565(120, 230, 160));
    if (h->buff_t[BUFF_CRIT]) x = buff_tag(x, "FOCUS", RGB565(255, 220, 60));
    if (w->myth.stop_t > 0) x = buff_tag(x, "TIME STOP", RGB565(150, 210, 255));
    if (w->sig.res > 1.5) {
        char n[12];
        fmt_num(n, sizeof n, w->sig.res);
        snprintf(buf, sizeof buf, T("RESONANCE x%s"), n);
        x = buff_tag(x, buf, RGB565(255, 170, 60));
    }
    if (w->myth.streak > 0) {
        snprintf(buf, sizeof buf, T("SLAUGHTER x%d%%"), 100 + w->myth.streak);
        x = buff_tag(x, buf, RGB565(255, 70, 70));
    }
    if (h->stacks > 0) {
        snprintf(buf, sizeof buf, T("FEROCITY %d"), h->stacks);
        buff_tag(x, buf, RGB565(255, 200, 80));
    }
}

static void draw_status(const World *w, const Profile *p)
{
    char buf[96];
    if (w->boss_floor)
        snprintf(buf, sizeof buf, T("FLOOR %d  GUARDIAN"), w->floor);
    else
        snprintf(buf, sizeof buf, T("FLOOR %d  %d/%d"), w->floor, MIN(w->kills, w->quota), w->quota);
    gfx_dim_rect(0, 0, SCREEN_W, 11);
    font_draw(4, 2, buf, C_TEXT, 1);
    {
        char tier[24] = "", roman[8];
        if (w->floor >= TORMENT_FLOOR) {
            torment_name(roman, sizeof roman, w->floor);
            snprintf(tier, sizeof tier, T("TORMENT %s"), roman);
        }
        snprintf(buf, sizeof buf, "%s%s%s  %s", tier, tier[0] ? "  " : "", T(theme_name(w->theme)),
                 T(p->mode == MODE_FARM ? "FARM" : "PUSH"));
    }
    font_draw(SCREEN_W - 4 - font_text_width(buf, 1), 2, buf, p->mode == MODE_FARM ? RGB565(120, 200, 255) : C_DIM, 1);
    draw_buffs(w);
    if (w->msg_t > 0) {
        int tw = font_text_width(w->msg, 1);
        gfx_dim_rect((SCREEN_W - tw) / 2 - 3, 24, tw + 6, 11);
        font_draw((SCREEN_W - tw) / 2, 26, w->msg, w->msg_color, 1);
    }
}

/* The bounty nearest completion, under the status line on the right. */
static void draw_bounty(const Profile *p)
{
    const Bounty *best = NULL;
    char t[64], buf[96];
    int i, tw;
    for (i = 0; i < BOUNTY_SLOTS; i++) {
        const Bounty *b = &p->bounty[i];
        if (b->kind != BT_NONE && b->need > 0 && (!best || b->have * best->need > best->have * b->need))
            best = b;
    }
    if (!best)
        return;
    bounty_text(t, sizeof t, best);
    snprintf(buf, sizeof buf, "%s %d/%d", t, best->have, best->need);
    tw = MIN(font_text_width(buf, 1), 150);
    gfx_dim_rect(SCREEN_W - tw - 6, 12, tw + 3, 9);
    font_draw_fit(SCREEN_W - tw - 4, 13, buf, tw, RGB565(230, 200, 120));
}

/* The running event's objective, under the bounty line. */
static void draw_event(const World *w)
{
    char buf[96];
    int tw;
    events_d4_status(w, buf, sizeof buf);
    if (!buf[0])
        return;
    tw = MIN(font_text_width(buf, 1), 150);
    gfx_dim_rect(SCREEN_W - tw - 6, 22, tw + 3, 9);
    font_draw_fit(SCREEN_W - tw - 4, 23, buf, tw, (w->tick >> 3) & 1 ? RGB565(255, 110, 90) : RGB565(255, 170, 120));
}

/* Aldric (through the torch) or the hero, just above the subtitles. */
static void draw_bark(const World *w, const Profile *p)
{
    const BarkLine *l = w->bark.line;
    char buf[160];
    int tw;
    if (w->bark.t <= 0 || !l)
        return;
    snprintf(buf, sizeof buf, T("%s: %s"), l->speaker == SPK_ALDRIC ? T("ALDRIC") : p->look.name, T(l->text));
    tw = MIN(font_text_width(buf, 1), SCREEN_W - 12);
    gfx_dim_rect((SCREEN_W - tw) / 2 - 3, 148, tw + 6, 11);
    font_draw_fit((SCREEN_W - tw) / 2, 150, buf, tw,
                  l->speaker == SPK_ALDRIC ? RGB565(255, 190, 120) : RGB565(170, 215, 255));
}

static void draw_banner(const World *w)
{
    int tw, y = 60;
    if (w->banner_t <= 0)
        return;
    tw = font_text_width(w->banner, 2);
    gfx_dim_rect((SCREEN_W - tw) / 2 - 6, y - 4, tw + 12, 22);
    gfx_hline((SCREEN_W - tw) / 2 - 6, y - 4, tw + 12, w->banner_color);
    gfx_hline((SCREEN_W - tw) / 2 - 6, y + 17, tw + 12, w->banner_color);
    font_draw_shadow((SCREEN_W - tw) / 2, y, w->banner, w->banner_color, 0, 2);
}

static void draw_boss_bar(const World *w)
{
    int i;
    for (i = 0; i < w->nmon; i++) {
        const Monster *m = &w->mon[i];
        if (!m->boss || !m->alive || !m->aggro)
            continue;
        gfx_fill_rect(60, 36, 200, 14, RGB565(20, 6, 8));
        gfx_fill_rect(62, 44, (int)(196 * CLAMP(m->hp / m->max_hp, 0.0, 1.0)), 4,
                      m->vuln ? RGB565(200, 110, 255) : RGB565(200, 40, 30));
        gfx_rect(60, 36, 200, 14, RGB565(160, 110, 60));
        font_draw(160 - font_text_width(w->boss_name, 1) / 2, 37, w->boss_name, RGB565(255, 200, 120), 1);
        return;
    }
}

void render_hud(const World *w, const Profile *p)
{
    gfx_fill_rect(0, PANEL_Y, SCREEN_W, SCREEN_H - PANEL_Y, C_PANEL);
    gfx_hline(0, PANEL_Y, SCREEN_W, C_EDGE);
    draw_globes(w, p);
    draw_potion(w);
    draw_skills(w, p);
    draw_info(w, p);
    draw_status(w, p);
    draw_bounty(p);
    draw_event(w);
    draw_bark(w, p);
    if (w->boss_floor)
        draw_boss_bar(w);
    draw_banner(w);
}
