/* font.h - text drawing: the original 5x7 pixel font for ASCII (lowercase
 * folds to upper, 'x' is a small multiplication sign) plus 8x8 CJK glyphs
 * (Fusion Pixel Font subset) for Chinese, Japanese and Korean. Strings are
 * UTF-8 and pass through T() first, so every call draws the current
 * language (see i18n/i18n.h). */
#ifndef AD_FONT_H
#define AD_FONT_H

#include "../core/common.h"

#define FONT_W 5
#define FONT_H 7
#define FONT_ADVANCE 6

int  font_text_width(const char *s, int scale);
int  font_draw(int x, int y, const char *s, uint16_t color, int scale);
/* Draw at most max_w pixels of text (the rest is cut off cleanly). */
int  font_draw_fit(int x, int y, const char *s, int max_w, uint16_t color);
void font_draw_shadow(int x, int y, const char *s, uint16_t color, uint16_t shadow, int scale);
void font_draw_centered(int y, const char *s, uint16_t color, uint16_t shadow, int scale);
/* Wrapped text within max_w pixels (spaces, or between CJK characters);
 * returns the y after the last line. */
int  font_draw_wrapped(int x, int y, const char *s, int max_w, uint16_t color);
/* Same, but lines that would end below max_y are skipped. */
int  font_draw_wrapped_clip(int x, int y, const char *s, int max_w, uint16_t color, int max_y);

#endif
