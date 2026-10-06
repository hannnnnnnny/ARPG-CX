/* menu_int.h - shared look and helpers of the menu pages (internal). */
#ifndef AD_MENU_INT_H
#define AD_MENU_INT_H

#include "game.h"
#include "../gfx/gfx.h"

#define C_BG    RGB565(14, 10, 16)
#define C_TEXT  RGB565(230, 220, 200)
#define C_DIM   RGB565(130, 118, 104)
#define C_SEL   RGB565(255, 200, 90)
#define C_GOOD  RGB565(110, 230, 110)
#define C_BAD   RGB565(240, 90, 80)
#define C_EDGE  RGB565(120, 90, 60)
#define C_HIL   RGB565(48, 34, 30)
#define C_LEG   RGB565(255, 140, 30)
#define C_AFFIX RGB565(150, 180, 255)
#define C_GREAT RGB565(255, 170, 60)
#define C_TEMP  RGB565(110, 220, 220)
#define MENU_TOP 18
#define FOOTER_Y 216

void menu_footer(const Game *g, const char *keys);
void menu_row_highlight(int y, int h, bool on);
/* Diablo IV style item tooltip. compare: equipped item to compare against
 * (or NULL). Returns the y below the card. */
int  menu_item_card(const Game *g, const Item *it, int x, int y, int w, const Item *compare);
void menu_craft_toast(Game *g, CraftResult r, const char *ok);
/* Move a selection up/down with wrap-around; returns true if it moved. */
bool menu_move(const Input *in, int *sel, int rows);
int  menu_scroll(int sel, int rows, int visible);

void hero_tick(Game *g, Input *in);
void hero_render(Game *g);
void bag_tick(Game *g, Input *in);
void bag_render(Game *g);
void skills_tick(Game *g, Input *in);
void skills_render(Game *g);
void paragon_tick(Game *g, Input *in);
void paragon_render(Game *g);
void town_tick(Game *g, Input *in);
void town_render(Game *g);
void embers_tick(Game *g, Input *in);
void embers_render(Game *g);
void options_tick(Game *g, Input *in, uint32_t now);
void options_render(Game *g);
/* Pages that use LEFT/RIGHT themselves (not for switching pages). */
bool menu_page_uses_lr(const Game *g);

#endif
