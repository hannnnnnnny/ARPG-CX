/*
 * i18n.h - languages: English, Simplified Chinese, Traditional Chinese,
 * Japanese, Korean.
 *
 * Game code keeps its English strings. T(english) returns the string in
 * the current language (or the English text if there is no translation).
 * font_draw() translates whole strings automatically, so only composed
 * text (snprintf formats and their pieces) needs explicit T() calls.
 * Translations and the CJK glyph subset are generated into i18n_data.c by
 * tools/i18n/gen_i18n.py from the TSV tables in tools/i18n and the Fusion Pixel Font.
 */
#ifndef AD_I18N_H
#define AD_I18N_H

#include "../core/common.h"

typedef enum { LANG_EN, LANG_ZHS, LANG_ZHT, LANG_JA, LANG_KO, LANG_COUNT } Lang;

typedef struct {
    const char *en;
    const char *tr[LANG_COUNT - 1];   /* zh-Hans, zh-Hant, ja, ko (UTF-8) */
} I18nEntry;

typedef struct {
    uint32_t cp;
    uint8_t  rows[8];                  /* 8x8, bit 7 = leftmost pixel */
} CjkGlyph;

extern const I18nEntry i18n_strings[];
extern const int i18n_count;
extern const CjkGlyph *const i18n_glyphs[LANG_COUNT];
extern const int i18n_glyph_count[LANG_COUNT];

void lang_set(int lang);
int  lang_get(void);
const char *lang_name(int lang);      /* in its own language */
const char *T(const char *en);
/* Composing names: CJK puts modifiers before the noun without spaces;
 * Korean keeps spaces. */
bool lang_compact(void);               /* no spaces between words (zh, ja) */
bool lang_modifier_first(void);        /* "OF THE BEAR BOOTS" order (zh, ja, ko) */
const uint8_t *cjk_glyph(uint32_t cp); /* NULL if not in the current font subset */
/* T(a) and T(b) joined: "A B" in English and Korean, "AB" in zh / ja. */
void tjoin(char *out, size_t cap, const char *a, const char *b);

#endif
