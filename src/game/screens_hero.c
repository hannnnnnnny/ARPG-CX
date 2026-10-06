/*
 * screens_hero.c - choosing a class and creating the hero's look (skin,
 * hair style and colour, face, eyes, clothing colour, name). The same
 * creator is reachable from OPTIONS > APPEARANCE to change the look later.
 */
#include "game.h"
#include "build.h"
#include "skills.h"
#include "../data/art.h"
#include "../gfx/font.h"
#include "../gfx/sprites.h"
#include "../core/bignum.h"
#include "../i18n/i18n.h"
#include <stdio.h>
#include <string.h>

#define C_TEXT  RGB565(230, 220, 200)
#define C_DIM   RGB565(130, 118, 104)
#define C_SEL   RGB565(255, 200, 90)
#define C_TITLE RGB565(255, 120, 50)
#define C_SHADE RGB565(30, 8, 4)
#define C_HIL   RGB565(60, 30, 20)

enum { CR_SKIN, CR_HAIR, CR_HAIR_COLOR, CR_FACE, CR_EYES, CR_CLOTH, CR_NAME, CR_RANDOM, CR_BEGIN, CR_COUNT };

static const char *const skin_names[SKIN_TONES] = { "PALE", "LIGHT", "TAN", "BROWN", "DARK", "ASHEN" };
static const char *const hair_names[HAIR_STYLES] = { "SHORT", "LONG", "MOHAWK", "BALD", "BRAIDS", "PONYTAIL" };
static const char *const hcol_names[HAIR_COLORS] = { "BLACK", "BROWN", "AUBURN", "BLOND", "WHITE", "RED", "GREY",
                                                      "BLUE-BLACK" };
static const char *const face_names[FACE_STYLES] = { "CLEAN", "BEARD", "SCAR", "WAR PAINT", "TATTOO" };
static const char *const eye_names[EYE_COLORS] = { "DARK", "BLUE", "GREEN", "AMBER" };
static const char *const cloth_names[CLOTH_COLORS] = { "CRIMSON", "BLUE", "GREEN", "PURPLE", "BLACK", "WHITE",
                                                        "BROWN", "TEAL" };

static void preview_look(HeroLook *l, int cls, const Look *look)
{
    memset(l, 0, sizeof *l);
    l->cls = (uint8_t)cls;
    l->skin = look->skin % SKIN_TONES;
    l->hair = look->hair % HAIR_STYLES;
    l->hair_color = look->hair_color % HAIR_COLORS;
    l->face = look->face % FACE_STYLES;
    l->eyes = look->eyes % EYE_COLORS;
    l->cloth = look->cloth % CLOTH_COLORS;
    l->chest_mat = 1;                       /* the starting leathers */
}

/* ------------------------------------------------------------ class select */

void class_select_tick(Game *g, Input *in, uint32_t now)
{
    if (in_up(in) || in_left(in))
        g->class_sel = (g->class_sel + CLASS_COUNT - 1) % CLASS_COUNT;
    if (in_down(in) || in_right(in))
        g->class_sel = (g->class_sel + 1) % CLASS_COUNT;
    if (in_back(in)) {
        g->state = g->class_for_rebirth ? GS_MENU : GS_TITLE;
        input_block_held(in);
        return;
    }
    if (!in_ok(in))
        return;
    input_block_held(in);
    if (g->class_for_rebirth) {
        char buf[128], n[16];
        fmt_num(n, sizeof n, prog_rebirth(&g->p, g->class_sel));
        session_start(&g->s, &g->p);
        snprintf(buf, sizeof buf, T("REBORN AS %s! +%s EMBERS"), T(class_defs[g->class_sel].name), n);
        game_toast(g, buf, RGB565(255, 120, 60));
        g->state = GS_BATTLE;
        game_save(g, now);
        return;
    }
    create_open(g, false);
}

static void class_row(const Game *g, int i)
{
    HeroLook l;
    Look look;
    int y = 34 + i * 26;
    bool on = i == g->class_sel;
    prog_default_look(&look, i, 77u + (uint32_t)i * 13u);
    preview_look(&l, i, &look);
    game_ui_rect(8, y - 3, 112, 24, on);
    if (on)
        gfx_fill_rect(8, y - 3, 112, 24, C_HIL);
    gfx_rect(8, y - 3, 112, 24, on ? C_SEL : RGB565(80, 60, 50));
    gfx_blit(spr_hero_look(&l, on ? 2 + ((g->tick >> 3) & 1) : 0), 12, y - 1, 0);
    font_draw(32, y + 6, class_defs[i].name, on ? C_SEL : C_TEXT, 1);
}

void class_select_render(Game *g)
{
    const ClassDef *c = &class_defs[g->class_sel];
    HeroLook l;
    Look look;
    char buf[128];
    int i, y;
    title_backdrop(g->tick);
    font_draw_centered(8, g->class_for_rebirth ? "CHOOSE YOUR NEXT LIFE" : "CHOOSE YOUR CLASS", C_SEL, C_SHADE, 2);
    for (i = 0; i < CLASS_COUNT; i++)
        class_row(g, i);
    gfx_rect(128, 30, 186, 160, RGB565(80, 60, 50));
    font_draw(136, 36, c->name, C_TITLE, 2);
    prog_default_look(&look, g->class_sel, 77u + (uint32_t)g->class_sel * 13u);
    preview_look(&l, g->class_sel, &look);
    gfx_blit_scaled(spr_hero_look(&l, (g->tick >> 4) & 1), 264, 52, 3);
    y = font_draw_wrapped(136, 56, c->desc, 120, C_TEXT);
    snprintf(buf, sizeof buf, "%s, %s", T(resource_name((ResourceKind)c->res_kind)), T(stat_name((MainStat)c->main_stat)));
    font_draw(136, y + 4, c->style, C_DIM, 1);
    font_draw(136, y + 14, buf, RGB565(160, 200, 255), 1);
    font_draw(136, 126, "BUILDS:", C_DIM, 1);
    for (i = 0; i < PRESETS; i++)
        font_draw(144, 138 + i * 10, c->preset[i].name, RGB565(255, 170, 90), 1);
    snprintf(buf, sizeof buf, T("STARTS WITH %s"), T(skill_def(g->class_sel, c->preset[0].bar[0])->name));
    font_draw(136, 174, buf, C_DIM, 1);
    font_draw_centered(198, "UP/DOWN: CHOOSE    ENTER: NEXT    ESC: BACK", C_DIM, C_SHADE, 1);
}

/* ------------------------------------------------------------- creation */

void create_open(Game *g, bool edit_only)
{
    g->state = GS_CREATE;
    g->create_edit_only = edit_only;
    g->create_sel = 0;
    g->name_edit = false;
    if (edit_only) {
        g->create_look = g->p.look;
        g->class_sel = g->p.cls;
    } else {
        prog_default_look(&g->create_look, g->class_sel, (uint32_t)g->tick * 2654435761u);
    }
}

static uint8_t cycle(uint8_t v, int d, int n) { return (uint8_t)(((int)v + d + n) % n); }

static void change_option(Look *l, int row, int d)
{
    switch (row) {
    case CR_SKIN:       l->skin = cycle(l->skin, d, SKIN_TONES); break;
    case CR_HAIR:       l->hair = cycle(l->hair, d, HAIR_STYLES); break;
    case CR_HAIR_COLOR: l->hair_color = cycle(l->hair_color, d, HAIR_COLORS); break;
    case CR_FACE:       l->face = cycle(l->face, d, FACE_STYLES); break;
    case CR_EYES:       l->eyes = cycle(l->eyes, d, EYE_COLORS); break;
    case CR_CLOTH:      l->cloth = cycle(l->cloth, d, CLOTH_COLORS); break;
    default: break;
    }
}

static const char name_chars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ-'";

static void name_char_step(char *c, int d)
{
    const char *p = strchr(name_chars, *c ? *c : ' ');
    int n = (int)sizeof name_chars - 1, i = p ? (int)(p - name_chars) : 0;
    *c = name_chars[(i + d + n) % n];
}

static void finish_name(Look *l, int cls, uint32_t seed)
{
    int n = NAME_LEN;
    l->name[NAME_LEN] = '\0';
    while (n > 0 && (l->name[n - 1] == ' ' || l->name[n - 1] == '\0'))
        l->name[--n] = '\0';
    if (n == 0) {                           /* an empty name gets a random one */
        Look tmp;
        prog_default_look(&tmp, cls, seed);
        memcpy(l->name, tmp.name, sizeof l->name);
    }
}

static void name_tick(Game *g, Input *in)
{
    char *nm = g->create_look.name;
    int len = (int)strlen(nm);
    while (len < NAME_LEN)                  /* edit over a space-padded field */
        nm[len++] = ' ';
    nm[NAME_LEN] = '\0';
    if (in_left(in))  g->name_cursor = MAX(0, g->name_cursor - 1);
    if (in_right(in)) g->name_cursor = MIN(NAME_LEN - 1, g->name_cursor + 1);
    if (in_up(in))    name_char_step(&nm[g->name_cursor], 1);
    if (in_down(in))  name_char_step(&nm[g->name_cursor], -1);
    if (in_ok(in) || in_back(in)) {
        g->name_edit = false;
        finish_name(&g->create_look, g->class_sel, (uint32_t)g->tick);
        input_block_held(in);
    }
}

static void create_done(Game *g, uint32_t now)
{
    finish_name(&g->create_look, g->class_sel, now);
    if (g->create_edit_only) {
        g->p.look = g->create_look;
        g->state = GS_MENU;
        game_toast(g, "APPEARANCE CHANGED", RGB565(110, 230, 110));
        return;
    }
    prog_new(&g->p, now ^ 0xA5u, g->class_sel);
    g->p.look = g->create_look;
    begin_battle(g, now, true);
    g->state = GS_BATTLE;
}

void create_tick(Game *g, Input *in, uint32_t now)
{
    if (g->name_edit) {
        name_tick(g, in);
        return;
    }
    if (in_up(in))    g->create_sel = (g->create_sel + CR_COUNT - 1) % CR_COUNT;
    if (in_down(in))  g->create_sel = (g->create_sel + 1) % CR_COUNT;
    if (in_left(in))  change_option(&g->create_look, g->create_sel, -1);
    if (in_right(in)) change_option(&g->create_look, g->create_sel, 1);
    if (in_back(in)) {
        g->state = g->create_edit_only ? GS_MENU : GS_CLASS_SELECT;
        input_block_held(in);
        return;
    }
    if (in_alt(in) && g->create_sel == CR_NAME) {
        Look tmp;
        prog_default_look(&tmp, g->class_sel, (uint32_t)g->tick * 7919u);
        memcpy(g->create_look.name, tmp.name, sizeof tmp.name);
    }
    if (!in_ok(in))
        return;
    input_block_held(in);
    if (g->create_sel == CR_NAME) {
        g->name_edit = true;
        g->name_cursor = 0;
    } else if (g->create_sel == CR_RANDOM) {
        Look keep = g->create_look;
        prog_default_look(&g->create_look, g->class_sel, (uint32_t)g->tick * 2246822519u);
        memcpy(g->create_look.name, keep.name, sizeof keep.name);
    } else if (g->create_sel == CR_BEGIN) {
        create_done(g, now);
    }
}

static void option_text(const Look *l, int row, char *buf, size_t cap, uint16_t *swatch)
{
    *swatch = 0;
    switch (row) {
    case CR_SKIN:       snprintf(buf, cap, "%s", skin_names[l->skin % SKIN_TONES]); *swatch = skin_color(l->skin); break;
    case CR_HAIR:       snprintf(buf, cap, "%s", hair_names[l->hair % HAIR_STYLES]); break;
    case CR_HAIR_COLOR: snprintf(buf, cap, "%s", hcol_names[l->hair_color % HAIR_COLORS]);
                        *swatch = hair_color(l->hair_color); break;
    case CR_FACE:       snprintf(buf, cap, "%s", face_names[l->face % FACE_STYLES]); break;
    case CR_EYES:       snprintf(buf, cap, "%s", eye_names[l->eyes % EYE_COLORS]); break;
    case CR_CLOTH:      snprintf(buf, cap, "%s", cloth_names[l->cloth % CLOTH_COLORS]);
                        *swatch = cloth_color(l->cloth); break;
    case CR_NAME:       snprintf(buf, cap, "%s", l->name); break;
    default:            buf[0] = '\0'; break;
    }
}

static void create_rows(const Game *g)
{
    static const char *const labels[CR_COUNT] = { "SKIN", "HAIR", "HAIR COLOUR", "FACE", "EYES", "CLOTHING",
                                                  "NAME", "[ RANDOMIZE ]", "[ BEGIN ]" };
    char buf[128];
    int i;
    for (i = 0; i < CR_COUNT; i++) {
        int y = 40 + i * 16;
        uint16_t sw;
        bool on = i == g->create_sel;
        game_ui_rect(118, y - 3, 196, 14, on);
        if (on)
            gfx_fill_rect(118, y - 3, 196, 14, C_HIL);
        font_draw(124, y, i == CR_BEGIN && g->create_edit_only ? "[ DONE ]" : labels[i], on ? C_SEL : C_TEXT, 1);
        option_text(&g->create_look, i, buf, sizeof buf, &sw);
        if (buf[0])
            font_draw(206, y, buf, C_SEL, 1);
        if (sw)
            gfx_fill_rect(298, y - 1, 9, 9, sw);
        if (on && i < CR_NAME)
            font_draw(194, y, "<", C_DIM, 1);
    }
    if (g->name_edit) {
        int cx = 206 + g->name_cursor * 6;
        gfx_hline(cx, 40 + CR_NAME * 16 + 8, 5, (g->tick >> 3) & 1 ? C_SEL : C_TEXT);
    }
}

void create_render(Game *g)
{
    HeroLook l;
    int frame = (g->tick >> 5) % 4 < 2 ? (g->tick >> 3) & 1 : 2 + ((g->tick >> 3) & 1);
    title_backdrop(g->tick);
    font_draw_centered(8, g->create_edit_only ? "APPEARANCE" : "CREATE YOUR HERO", C_SEL, C_SHADE, 2);
    preview_look(&l, g->class_sel, &g->create_look);
    gfx_fill_rect(14, 36, 96, 120, RGB565(20, 12, 14));
    gfx_rect(14, 36, 96, 120, RGB565(80, 60, 50));
    gfx_blit_scaled(spr_hero_look(&l, frame), 30, 50, 4);
    font_draw(62 - font_text_width(class_defs[g->class_sel].name, 1) / 2, 140, class_defs[g->class_sel].name,
              C_TITLE, 1);
    create_rows(g);
    if (g->name_edit)
        font_draw_centered(190, "LEFT/RIGHT: LETTER  UP/DOWN: CHANGE  ENTER: DONE", C_DIM, C_SHADE, 1);
    else
        font_draw_centered(190, "UP/DOWN: ROW  LEFT/RIGHT: CHANGE  ENTER: SELECT", C_DIM, C_SHADE, 1);
    if (!g->name_edit && g->create_sel == CR_NAME)
        font_draw_centered(176, "ENTER: EDIT NAME   DEL: RANDOM NAME", C_DIM, C_SHADE, 1);
}
