#include "i18n.h"
#include <stdio.h>
#include <string.h>

#define SLOTS 8192            /* hash slots, power of two, > 2x the strings */

static int g_lang = LANG_EN;
static uint16_t g_slot[SLOTS]; /* entry index + 1, 0 = empty */
static bool g_built;

static uint32_t fnv(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}

static void build_index(void)
{
    int i;
    memset(g_slot, 0, sizeof g_slot);
    for (i = 0; i < i18n_count && i < 65535; i++) {
        uint32_t h = fnv(i18n_strings[i].en) & (SLOTS - 1);
        while (g_slot[h])                       /* linear probing */
            h = (h + 1) & (SLOTS - 1);
        g_slot[h] = (uint16_t)(i + 1);
    }
    g_built = true;
}

void lang_set(int lang) { g_lang = (lang >= 0 && lang < LANG_COUNT) ? lang : LANG_EN; }
int  lang_get(void) { return g_lang; }

const char *lang_name(int lang)
{
    static const char *const n[LANG_COUNT] = {
        "ENGLISH", "\xE7\xAE\x80\xE4\xBD\x93\xE4\xB8\xAD\xE6\x96\x87", "\xE7\xB9\x81\xE9\xAB\x94\xE4\xB8\xAD\xE6\x96\x87",
        "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E", "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4",
    };
    return n[(lang >= 0 && lang < LANG_COUNT) ? lang : 0];
}

const char *T(const char *en)
{
    uint32_t h;
    if (g_lang == LANG_EN || !en || !en[0])
        return en;
    if (!g_built)
        build_index();
    for (h = fnv(en) & (SLOTS - 1); g_slot[h]; h = (h + 1) & (SLOTS - 1)) {
        const I18nEntry *e = &i18n_strings[g_slot[h] - 1];
        if (!strcmp(e->en, en)) {
            const char *t = e->tr[g_lang - 1];
            return t && t[0] ? t : en;
        }
    }
    return en;
}

bool lang_compact(void) { return g_lang == LANG_ZHS || g_lang == LANG_ZHT || g_lang == LANG_JA; }

void tjoin(char *out, size_t cap, const char *a, const char *b)
{
    const char *ta = T(a), *tb = T(b);
    snprintf(out, cap, "%s%s%s", ta, ta[0] && tb[0] && !lang_compact() ? " " : "", tb);
}
bool lang_modifier_first(void) { return g_lang != LANG_EN; }

static const uint8_t *find_glyph(int lang, uint32_t cp)
{
    const CjkGlyph *g = i18n_glyphs[lang];
    int lo = 0, hi = i18n_glyph_count[lang] - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (g[mid].cp == cp)
            return g[mid].rows;
        if (g[mid].cp < cp)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return NULL;
}

#ifdef GFX_HD
static const uint16_t *find_glyph_hd(int lang, uint32_t cp)
{
    const CjkGlyphHD *g = i18n_glyphs_hd[lang];
    int lo = 0, hi = i18n_glyph_count_hd[lang] - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (g[mid].cp == cp)
            return g[mid].rows;
        if (g[mid].cp < cp)
            lo = mid + 1;
        else
            hi = mid - 1;
    }
    return NULL;
}

const uint16_t *cjk_glyph_hd(uint32_t cp)
{
    const uint16_t *g = find_glyph_hd(g_lang, cp);
    int l;
    for (l = 0; !g && l < LANG_COUNT; l++)
        g = find_glyph_hd(l, cp);
    return g;
}
#endif

const uint8_t *cjk_glyph(uint32_t cp)
{
    const uint8_t *g = find_glyph(g_lang, cp);
    int l;
    for (l = 0; !g && l < LANG_COUNT; l++)     /* e.g. language names on the title */
        g = find_glyph(l, cp);
    return g;
}
