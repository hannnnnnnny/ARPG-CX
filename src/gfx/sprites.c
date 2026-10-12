#include "sprites.h"
#include "../data/art.h"
#include "../data/icons.h"
#include "../game/items.h"
#include <string.h>

#define POOL_PIXELS 40000
#define HERO_CACHE 8
#define ICON_CACHE 72

static uint16_t g_pool[POOL_PIXELS];
static int g_used;
static Sprite g_mon[MON_ART_COUNT][2][2]; /* [type][frame][boss] */
static uint16_t *g_mon_px[MON_ART_COUNT][2][2];
static int g_mon_theme;
static Sprite g_ally[3][2];
static Sprite g_icon[SLOT_ICONS][RAR_COUNT];

typedef struct {
    bool used;
    HeroLook key;
    uint32_t stamp;
    uint16_t px[HERO_FRAMES][HERO_W * HERO_H];
    Sprite spr[HERO_FRAMES];
} HeroCache;
static HeroCache g_hero[HERO_CACHE];

typedef struct { bool used; uint8_t icon; uint16_t color; uint32_t stamp; uint16_t px[256]; Sprite spr; } IconCache;
static IconCache g_icons[ICON_CACHE];
static uint32_t g_stamp;

/* Dark-fantasy palette: muted, hue-shifted ramps lit from the top left.
 * Saturated colours are kept for what the player must notice: glowing eyes,
 * fire, loot. Uppercase/extra letters are the darker or lighter ramp steps. */
uint16_t palette_color(char c)
{
    switch (c) {
    case 'k': return RGB565(20, 12, 16);
    case 'w': return RGB565(232, 224, 206);
    case 's': return RGB565(170, 172, 180);
    case 'S': return RGB565(98, 100, 116);
    case 'T': return RGB565(206, 208, 214);
    case 'd': return RGB565(48, 44, 54);
    case 'D': return RGB565(30, 26, 34);
    case 'r': return RGB565(176, 36, 36);
    case 'R': return RGB565(92, 18, 26);
    case 'j': return RGB565(222, 90, 64);
    case 'y': return RGB565(226, 184, 86);
    case 'Y': return RGB565(206, 104, 34);
    case 'n': return RGB565(112, 74, 48);
    case 'N': return RGB565(58, 38, 28);
    case 'g': return RGB565(128, 146, 92);
    case 'G': return RGB565(72, 92, 62);
    case 'u': return RGB565(186, 196, 120);
    case 'v': return RGB565(40, 56, 40);
    case 'p': return RGB565(122, 74, 156);
    case 'P': return RGB565(60, 34, 86);
    case 'I': return RGB565(170, 120, 204);
    case 'b': return RGB565(66, 98, 162);
    case 'B': return RGB565(30, 38, 74);
    case 'H': return RGB565(244, 238, 214);
    case 'o': return RGB565(214, 200, 166);
    case 'O': return RGB565(150, 134, 104);
    case 'q': return RGB565(92, 78, 64);
    case 'e': return RGB565(255, 92, 40);
    case 'c': return RGB565(110, 206, 214);
    case 'f': return RGB565(204, 158, 124);
    case 'F': return RGB565(156, 106, 84);
    case 'E': return RGB565(96, 56, 46);
    default:  return COLOR_KEY;
    }
}

/* ------------------------------------------------------------ grids */

typedef struct { int w, h; char cell[BOSS_N][BOSS_N + 1]; } Grid;

static void grid_clear(Grid *g, int w, int h)
{
    int y;
    g->w = w;
    g->h = h;
    for (y = 0; y < h; y++) {
        memset(g->cell[y], '.', (size_t)w);
        g->cell[y][w] = '\0';
    }
}

static bool grid_stamp(Grid *g, const char *const *rows, int count, int dy)
{
    int x, y;
    for (y = 0; y < count; y++) {
        if ((int)strlen(rows[y]) != g->w)
            return false;
        if (y + dy < 0 || y + dy >= g->h)
            continue;
        for (x = 0; x < g->w; x++)
            if (rows[y][x] != '.')
                g->cell[y + dy][x] = rows[y][x];
    }
    return true;
}

/* Letter -> colour; 'x' (and per-sprite swaps) via the map, else palette. */
typedef struct { char from[12]; uint16_t to[12]; int n; } ColorMap;

static uint16_t map_color(const ColorMap *m, char c)
{
    int i;
    for (i = 0; m && i < m->n; i++)
        if (m->from[i] == c)
            return m->to[i];
    return palette_color(c);
}

static void grid_paint(const Grid *g, uint16_t *px, int scale, const ColorMap *m)
{
    int w = g->w * scale, h = g->h * scale, x, y;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++)
            px[y * w + x] = map_color(m, g->cell[y / scale][x / scale]);
}

static bool commit(Sprite *s, const Grid *g, int scale, const ColorMap *m)
{
    int w = g->w * scale, h = g->h * scale;
    if (g_used + w * h > POOL_PIXELS)
        return false;
    grid_paint(g, g_pool + g_used, scale, m);
    s->w = (int16_t)w;
    s->h = (int16_t)h;
    s->px = g_pool + g_used;
    g_used += w * h;
    return true;
}

/* ----------------------------------------------------------- static art */

/* ------------------------------------------------------------ bosses */

/* Bosses are drawn at 32x32. The butcher has hand-finished art; the others
 * are their 16px art grown with EPX (Scale2x), which rounds the diagonals,
 * then thinned back to a 1px outline and edge-lit, so they keep the same
 * pixel density as everything around them instead of looking blown up. */
static void grid_epx(const Grid *s, Grid *d)
{
    int x, y;
    grid_clear(d, s->w * 2, s->h * 2);
    for (y = 0; y < s->h; y++)
        for (x = 0; x < s->w; x++) {
            char p = s->cell[y][x];
            char a = y > 0 ? s->cell[y - 1][x] : '.', dn = y + 1 < s->h ? s->cell[y + 1][x] : '.';
            char l = x > 0 ? s->cell[y][x - 1] : '.', r = x + 1 < s->w ? s->cell[y][x + 1] : '.';
            d->cell[2 * y][2 * x] = (l == a && l != dn && a != r) ? a : p;
            d->cell[2 * y][2 * x + 1] = (a == r && a != l && r != dn) ? r : p;
            d->cell[2 * y + 1][2 * x] = (dn == l && dn != r && l != a) ? l : p;
            d->cell[2 * y + 1][2 * x + 1] = (r == dn && r != a && dn != l) ? dn : p;
        }
}

static char cell_at(const Grid *g, int x, int y)
{
    return x >= 0 && y >= 0 && x < g->w && y < g->h ? g->cell[y][x] : '.';
}

/* Outer outline pixels touch transparency (8-neighbourhood). */
static bool outer_k(const Grid *g, int x, int y)
{
    int i, j;
    if (cell_at(g, x, y) != 'k')
        return false;
    for (j = -1; j <= 1; j++)
        for (i = -1; i <= 1; i++)
            if (cell_at(g, x + i, y + j) == '.')
                return true;
    return false;
}

/* Upscaling doubles the outline; the inner layer takes its neighbour's colour. */
static void grid_thin(Grid *g)
{
    static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
    static Grid src;                       /* static: keep 1 KB off the calculator's stack */
    int x, y, k;
    src = *g;
    for (y = 0; y < g->h; y++)
        for (x = 0; x < g->w; x++) {
            char fill = 0;
            bool by_outer = false;
            if (src.cell[y][x] != 'k' || outer_k(&src, x, y))
                continue;
            for (k = 0; k < 4; k++) {
                char c = cell_at(&src, x + dx[k], y + dy[k]);
                by_outer |= outer_k(&src, x + dx[k], y + dy[k]);
                if (c != '.' && c != 'k' && !fill)
                    fill = c;
            }
            if (by_outer && fill)
                g->cell[y][x] = fill;
        }
}

/* One ramp step lighter / darker (see the ramps listed in data/art.c). */
static char ramp_step(char c, bool up)
{
    static const char up_from[] = "oqgGvFErRpPsSdON", up_to[] = "HougGfFjrIpTsSon";
    static const char dn_from[] = "HougGfFjrIpTsSwOn", dn_to[] = "oOgGvFErRpPsSdOqN";
    const char *from = up ? up_from : dn_from, *to = up ? up_to : dn_to;
    const char *hit = strchr(from, c);
    return c && hit ? to[hit - from] : c;
}

/* Light from the top left: exposed top/left edges brighten, bottom/right darken. */
static void grid_bevel(Grid *g)
{
    static Grid src;
    int x, y;
    src = *g;
    for (y = 0; y < g->h; y++)
        for (x = 0; x < g->w; x++) {
            char c = src.cell[y][x], u = cell_at(&src, x, y - 1), l = cell_at(&src, x - 1, y);
            char dn = cell_at(&src, x, y + 1), r = cell_at(&src, x + 1, y);
            bool lit = u == '.' || u == 'k' || l == '.' || l == 'k';
            bool dark = dn == '.' || dn == 'k' || r == '.' || r == 'k';
            if (c == '.' || c == 'k' || lit == dark)
                continue;
            g->cell[y][x] = ramp_step(c, lit);
        }
}

static bool mon_grid(int t, int f, bool boss, Grid *g)
{
    static Grid small;
    if (boss && t == MON_BUTCHER) {
        grid_clear(g, BOSS_N, BOSS_N);
        return grid_stamp(g, art_butcher_big[f], BOSS_N, 0);
    }
    grid_clear(boss ? &small : g, 16, 16);
    if (!grid_stamp(boss ? &small : g, art_mon[t][f], 16, 0))
        return false;
    if (boss) {
        grid_epx(&small, g);
        grid_thin(g);
        grid_bevel(g);
    }
    return true;
}

/* ---------------------------------------------------------- monsters */

#define C(r, g, b) RGB565(r, g, b)
/* The same monsters in each depth's colours: yellowed bone in the catacombs,
 * charred in the hellforge, frost-bitten, void-touched. */
static const ColorMap theme_maps[5] = {
    { { 0 }, { 0 }, 0 },
    { { 'o', 'H', 'O', 'q' }, { C(206, 186, 140), C(232, 218, 170), C(140, 118, 84), C(84, 66, 48) }, 4 },
    { { 'o', 'H', 'O', 'q', 'e', 'g', 'u', 'G', 'v' },
      { C(170, 140, 120), C(214, 182, 150), C(112, 82, 70), C(64, 40, 36), C(255, 196, 64),
        C(150, 120, 84), C(196, 160, 104), C(96, 70, 52), C(56, 36, 30) }, 9 },
    { { 'o', 'H', 'O', 'q', 'e', 'g', 'u', 'G', 'v' },
      { C(190, 208, 222), C(228, 240, 248), C(122, 148, 172), C(64, 82, 108), C(120, 220, 255),
        C(112, 150, 154), C(168, 212, 214), C(64, 98, 108), C(32, 54, 66) }, 9 },
    { { 'o', 'H', 'O', 'q', 'e', 'g', 'u', 'G', 'v' },
      { C(186, 168, 208), C(226, 212, 244), C(124, 102, 152), C(72, 54, 98), C(226, 120, 255),
        C(118, 112, 152), C(170, 160, 206), C(70, 62, 104), C(40, 32, 64) }, 9 },
};
#undef C

static bool build_monsters(void)
{
    int t, f, b;
    for (t = 0; t < MON_ART_COUNT; t++)
        for (f = 0; f < 2; f++)
            for (b = 0; b < 2; b++) {
                Grid g;
                g_mon_px[t][f][b] = g_pool + g_used;
                if (!mon_grid(t, f, b, &g) || !commit(&g_mon[t][f][b], &g, 1, NULL))
                    return false;
            }
    g_mon_theme = 0;
    return true;
}

void sprites_set_theme(int theme)
{
    int t, f, b;
    theme = ((theme % 5) + 5) % 5;
    if (theme == g_mon_theme)
        return;
    g_mon_theme = theme;
    for (t = 0; t < MON_ART_COUNT; t++)
        for (f = 0; f < 2; f++)
            for (b = 0; b < 2; b++) {
                Grid g;
                if (mon_grid(t, f, b, &g))
                    grid_paint(&g, g_mon_px[t][f][b], 1, &theme_maps[theme]);
            }
}

static bool build_allies(void)
{
    /* Raised skeletons have green eyes; mages are spectral purple. */
    ColorMap warrior = { { 'e' }, { RGB565(90, 255, 140) }, 1 };
    ColorMap mage = { { 'e', 'o', 'O', 'H', 'q' }, { RGB565(120, 255, 220), RGB565(190, 150, 240), RGB565(120, 80, 180),
                                                 RGB565(226, 200, 250), RGB565(76, 46, 120) }, 5 };
    int f;
    for (f = 0; f < 2; f++) {
        Grid g;
        grid_clear(&g, 16, 16);
        if (!grid_stamp(&g, art_mon[0][f], 16, 0) || !commit(&g_ally[0][f], &g, 1, &warrior)
            || !commit(&g_ally[1][f], &g, 1, &mage))
            return false;
        grid_clear(&g, 16, 16);
        if (!grid_stamp(&g, art_wolf[f], 16, 0) || !commit(&g_ally[2][f], &g, 1, NULL))
            return false;
    }
    return true;
}

static bool build_icons(void)
{
    int s, r;
    for (s = 0; s < SLOT_ICONS; s++)
        for (r = 0; r < RAR_COUNT; r++) {
            Grid g;
            ColorMap m = { { 'x' }, { rarity_color((Rarity)r) }, 1 };
            grid_clear(&g, 8, 8);
            if (!grid_stamp(&g, art_icons[s], 8, 0) || !commit(&g_icon[s][r], &g, 1, &m))
                return false;
        }
    return true;
}

static bool check_hero_art(void)
{
    Grid g;
    int i, j;
    grid_clear(&g, 16, 20);
    for (i = 0; i < 6; i++)
        if (!grid_stamp(&g, art_body[i], 7, 7))
            return false;
    for (i = 0; i < HAIR_STYLES; i++)
        if (!grid_stamp(&g, art_hair[i], 7, 0))
            return false;
    for (i = 0; i < 2; i++)
        for (j = 0; j < 3; j++)
            if (!grid_stamp(&g, art_legs[i][j], 6, 14))
                return false;
    return grid_stamp(&g, art_hero_head, 7, 0);
}

bool sprites_init(void)
{
    g_used = 0;
    memset(g_hero, 0, sizeof g_hero);
    memset(g_icons, 0, sizeof g_icons);
    return build_monsters() && build_allies() && build_icons() && check_hero_art();
}

const Sprite *spr_mon(int type, int frame, bool boss) { return &g_mon[type % MON_ART_COUNT][frame & 1][boss ? 1 : 0]; }
const Sprite *spr_ally(int kind, int frame) { return &g_ally[CLAMP(kind, 0, 2)][frame & 1]; }

const Sprite *spr_icon(int slot, int rarity)
{
    static const uint8_t map[SLOT_COUNT] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 8 };
    return &g_icon[map[slot % SLOT_COUNT]][rarity % RAR_COUNT];
}

/* ----------------------------------------------------------- hero look */

#define C(r, g, b) RGB565(r, g, b)
static const uint16_t skin_tab[SKIN_TONES][2] = {
    { C(250, 214, 186), C(214, 170, 140) }, { C(232, 190, 150), C(190, 140, 105) },
    { C(205, 150, 105), C(160, 110, 75) },  { C(160, 105, 70), C(115, 72, 48) },
    { C(110, 72, 50), C(75, 48, 34) },      { C(170, 170, 178), C(120, 120, 130) },
};
static const uint16_t hair_tab[HAIR_COLORS][2] = {
    { C(44, 40, 44), C(22, 20, 24) },     { C(110, 70, 40), C(70, 44, 26) },  { C(165, 72, 40), C(110, 45, 25) },
    { C(232, 200, 110), C(180, 150, 70) }, { C(232, 232, 238), C(170, 170, 182) }, { C(205, 52, 42), C(130, 30, 25) },
    { C(140, 140, 146), C(95, 95, 102) },  { C(64, 74, 150), C(36, 42, 92) },
};
static const uint16_t eye_tab[EYE_COLORS] = { C(30, 30, 42), C(90, 160, 255), C(90, 220, 120), C(255, 190, 60) };
static const uint16_t cloth_tab[CLOTH_COLORS][2] = {
    { C(170, 40, 40), C(110, 25, 28) },  { C(50, 80, 170), C(30, 48, 110) },  { C(60, 120, 60), C(36, 78, 38) },
    { C(110, 60, 150), C(70, 36, 100) }, { C(58, 54, 66), C(32, 30, 38) },    { C(215, 210, 200), C(160, 155, 148) },
    { C(130, 90, 55), C(88, 60, 36) },   { C(40, 140, 140), C(24, 92, 92) },
};
/* Gear materials: main, shade, trim. Index 0 = nothing equipped. */
static const uint16_t mat_tab[RAR_COUNT + 1][3] = {
    { C(150, 140, 125), C(100, 92, 80), C(170, 160, 140) },
    { C(140, 95, 55), C(95, 62, 35), C(175, 165, 150) },
    { C(110, 130, 190), C(60, 75, 130), C(205, 215, 235) },
    { C(185, 190, 200), C(110, 115, 130), C(250, 212, 80) },
    { C(95, 90, 105), C(55, 50, 62), C(255, 140, 30) },
    { C(215, 180, 90), C(150, 110, 50), C(255, 245, 200) },
    { C(90, 50, 140), C(50, 25, 85), C(215, 150, 255) },
};
#undef C

uint16_t material_color(int mat) { return mat_tab[CLAMP(mat, 0, RAR_COUNT)][0]; }
uint16_t skin_color(int i) { return skin_tab[((i % SKIN_TONES) + SKIN_TONES) % SKIN_TONES][0]; }
uint16_t hair_color(int i) { return hair_tab[((i % HAIR_COLORS) + HAIR_COLORS) % HAIR_COLORS][0]; }
uint16_t cloth_color(int i) { return cloth_tab[((i % CLOTH_COLORS) + CLOTH_COLORS) % CLOTH_COLORS][0]; }

static int mat_of(const Item *it) { return it->used ? it->rarity + 1 : 0; }

void hero_look_from(HeroLook *l, const Profile *p)
{
    const Item *helm = &p->equip[SLOT_HELM];
    memset(l, 0, sizeof *l);
    l->cls = p->cls;
    l->skin = p->look.skin % SKIN_TONES;
    l->hair = p->look.hair % HAIR_STYLES;
    l->hair_color = p->look.hair_color % HAIR_COLORS;
    l->face = p->look.face % FACE_STYLES;
    l->eyes = p->look.eyes % EYE_COLORS;
    l->cloth = p->look.cloth % CLOTH_COLORS;
    l->helm = (uint8_t)(helm->used && p->look.show_helm ? (helm->base & 3) + 1 : 0);
    l->helm_mat = (uint8_t)mat_of(helm);
    l->chest_mat = (uint8_t)mat_of(&p->equip[SLOT_CHEST]);
    l->chest_tier = p->equip[SLOT_CHEST].used ? p->equip[SLOT_CHEST].base & 3 : 0;
    l->glove_mat = (uint8_t)mat_of(&p->equip[SLOT_GLOVES]);
    l->pants_mat = (uint8_t)mat_of(&p->equip[SLOT_PANTS]);
    l->boot_mat = (uint8_t)mat_of(&p->equip[SLOT_BOOTS]);
}

static void hero_colors(const HeroLook *l, ColorMap *m)
{
    static const char role[8] = { '1', '2', '3', '4', '5', 'c', 'C', 't' };
    const uint16_t *chest = mat_tab[l->chest_mat];
    int i;
    for (i = 0; i < 8; i++)
        m->from[i] = role[i];
    m->to[0] = skin_tab[l->skin][0];
    m->to[1] = skin_tab[l->skin][1];
    m->to[2] = hair_tab[l->hair_color][0];
    m->to[3] = hair_tab[l->hair_color][1];
    m->to[4] = eye_tab[l->eyes];
    m->to[5] = cloth_tab[l->cloth][0];
    m->to[6] = cloth_tab[l->cloth][1];
    m->to[7] = chest[2];
    m->n = 8;
}

/* Gear letters, resolved before the colour map (it holds only 8 entries). */
static char gear_role(const HeroLook *l, char c, uint16_t *out)
{
    const uint16_t *chest = l->chest_mat ? mat_tab[l->chest_mat] : cloth_tab[l->cloth];
    const uint16_t *helm = mat_tab[l->helm_mat];
    switch (c) {
    case 'a': *out = chest[0]; return 1;
    case 'A': *out = chest[1]; return 1;
    case 'g': *out = l->glove_mat ? mat_tab[l->glove_mat][0] : skin_tab[l->skin][0]; return 1;
    case 'l': *out = l->pants_mat ? mat_tab[l->pants_mat][0] : RGB565(92, 72, 56); return 1;
    case 'L': *out = l->pants_mat ? mat_tab[l->pants_mat][1] : RGB565(62, 48, 38); return 1;
    case 'b': *out = l->boot_mat ? mat_tab[l->boot_mat][1] : RGB565(70, 45, 30); return 1;
    case 'm': *out = helm[0]; return 1;
    case 'M': *out = helm[1]; return 1;
    case 'h': *out = helm[2]; return 1;
    default:  return 0;
    }
}

static void compose_hero(const HeroLook *l, int frame, Grid *g)
{
    static const int legs[HERO_FRAMES] = { 0, 0, 1, 2 };
    int bob = frame == 1 ? 1 : 0, robe = l->cls == CLASS_SORCERER || l->cls == CLASS_NECRO;
    grid_clear(g, HERO_W, HERO_H);
    grid_stamp(g, art_legs[robe][legs[frame]], 6, 14);
    grid_stamp(g, art_body[l->cls % 6], 7, 7 + bob);
    if (l->chest_mat && l->chest_tier >= 2)
        grid_stamp(g, art_pauldrons[l->chest_tier >= 3], 2, 7 + bob);
    grid_stamp(g, art_hero_head, 7, bob);
    grid_stamp(g, art_face[l->face % FACE_STYLES], 7, bob);
    if (l->helm == 0 || l->helm == 2)            /* hair shows without a helm and under a skullcap */
        grid_stamp(g, art_hair[l->hair % HAIR_STYLES], 7, bob);
    if (l->helm)
        grid_stamp(g, art_helm[(l->helm - 1) & 3], 7, bob);
}

/* Scale a colour by num/256 and add 'add' (in 8-bit units), clamped. */
static uint16_t tone(uint16_t c, int num, int add)
{
    int r = ((c >> 11) & 31) * 8 * num / 256 + add, g = ((c >> 5) & 63) * 4 * num / 256 + add;
    int b = (c & 31) * 8 * num / 256 + add;
    return RGB565(CLAMP(r, 0, 255), CLAMP(g, 0, 255), CLAMP(b, 0, 255));
}

static bool open_cell(const Grid *g, int x, int y)
{
    char c = cell_at(g, x, y);
    return c == '.' || c == 'k';
}

/* The same top-left light the monsters are drawn with: edges facing the
 * light brighten, edges facing away darken. Hero parts are recoloured per
 * gear, so this works on the final colours instead of ramp letters. */
static void light_hero(const Grid *g, uint16_t *px)
{
    int x, y;
    for (y = 0; y < HERO_H; y++)
        for (x = 0; x < HERO_W; x++) {
            char c = g->cell[y][x];
            bool lit, dark;
            if (c == '.' || c == 'k' || c == '5')       /* outline and eyes stay as drawn */
                continue;
            lit = open_cell(g, x, y - 1) || open_cell(g, x - 1, y);
            dark = open_cell(g, x, y + 1) || open_cell(g, x + 1, y);
            if (lit != dark)
                px[y * HERO_W + x] = lit ? tone(px[y * HERO_W + x], 307, 12) : tone(px[y * HERO_W + x], 180, 0);
        }
}

static void paint_hero(const HeroLook *l, const Grid *g, uint16_t *px)
{
    ColorMap m;
    int x, y;
    hero_colors(l, &m);
    for (y = 0; y < HERO_H; y++)
        for (x = 0; x < HERO_W; x++) {
            char c = g->cell[y][x];
            uint16_t col;
            px[y * HERO_W + x] = gear_role(l, c, &col) ? col : map_color(&m, c);
        }
    light_hero(g, px);
}

const Sprite *spr_hero_look(const HeroLook *l, int frame)
{
    int i, slot = 0, f;
    frame = ((frame % HERO_FRAMES) + HERO_FRAMES) % HERO_FRAMES;
    for (i = 0; i < HERO_CACHE; i++)
        if (g_hero[i].used && !memcmp(&g_hero[i].key, l, sizeof *l)) {
            g_hero[i].stamp = ++g_stamp;
            return &g_hero[i].spr[frame];
        }
    for (i = 1; i < HERO_CACHE; i++)            /* evict the least recently used */
        if (!g_hero[i].used || g_hero[i].stamp < g_hero[slot].stamp)
            slot = i;
    g_hero[slot].used = true;
    g_hero[slot].key = *l;
    g_hero[slot].stamp = ++g_stamp;
    for (f = 0; f < HERO_FRAMES; f++) {
        Grid g;
        compose_hero(l, f, &g);
        paint_hero(l, &g, g_hero[slot].px[f]);
        g_hero[slot].spr[f].w = HERO_W;
        g_hero[slot].spr[f].h = HERO_H;
        g_hero[slot].spr[f].px = g_hero[slot].px[f];
    }
    return &g_hero[slot].spr[frame];
}

/* ------------------------------------------------------------ skill icons */

static uint16_t shade(uint16_t c, int num, int den)
{
    int r = ((c >> 11) & 31) * num / den, g = ((c >> 5) & 63) * num / den, b = (c & 31) * num / den;
    return (uint16_t)((MIN(r, 31) << 11) | (MIN(g, 63) << 5) | MIN(b, 31));
}

static void paint_icon(IconCache *ic)
{
    ColorMap m = { { 'x', 'z' }, { ic->color, shade(ic->color, 1, 2) }, 2 };
    int x, y;
    for (y = 0; y < 16; y++)       /* dark gradient frame tinted with the skill colour */
        for (x = 0; x < 16; x++) {
            bool edge = x == 0 || y == 0 || x == 15 || y == 15;
            ic->px[y * 16 + x] = edge ? shade(ic->color, 2, 3) : shade(ic->color, 2 + (15 - y) / 6, 16);
        }
    for (y = 0; y < ICON_N; y++)
        for (x = 0; x < ICON_N; x++) {
            char c = art_skill_icons[ic->icon][y][x];
            if (c != '.')
                ic->px[(y + 2) * 16 + x + 2] = map_color(&m, c);
        }
}

const Sprite *spr_skill_icon(int icon, uint16_t color)
{
    int i, slot = 0;
    icon = CLAMP(icon, 0, IC_COUNT - 1);
    for (i = 0; i < ICON_CACHE; i++)
        if (g_icons[i].used && g_icons[i].icon == icon && g_icons[i].color == color) {
            g_icons[i].stamp = ++g_stamp;
            return &g_icons[i].spr;
        }
    for (i = 1; i < ICON_CACHE; i++)
        if (!g_icons[i].used || g_icons[i].stamp < g_icons[slot].stamp)
            slot = i;
    g_icons[slot].used = true;
    g_icons[slot].icon = (uint8_t)icon;
    g_icons[slot].color = color;
    g_icons[slot].stamp = ++g_stamp;
    paint_icon(&g_icons[slot]);
    g_icons[slot].spr.w = 16;
    g_icons[slot].spr.h = 16;
    g_icons[slot].spr.px = g_icons[slot].px;
    return &g_icons[slot].spr;
}
