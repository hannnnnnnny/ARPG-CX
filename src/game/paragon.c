#include "paragon.h"
#include "../i18n/i18n.h"
#include "../core/rng.h"
#include <stdio.h>
#include <string.h>

#define N BOARD_N
#define MID (BOARD_N / 2)

const GlyphDef glyph_defs[GLYPH_COUNT] = {
    { "MIGHT", "+#% DAMAGE TO CLOSE ENEMIES PER 5 STAT", MOD_ADD_CLOSE, 0, 1.5, MOD_X_ALL, 0, 6, "[x]6% DAMAGE" },
    { "TERROR", "+#% DAMAGE OVER TIME PER 5 STAT", MOD_ADD_DOT, 0, 2.0, MOD_X_DOT, 0, 10, "[x]10% DAMAGE OVER TIME" },
    { "DESTRUCTION", "+#% CORE SKILL DAMAGE PER 5 STAT", MOD_ADD_CORE, 0, 1.5, MOD_X_CORE, 0, 8, "[x]8% CORE DAMAGE" },
    { "ELEMENTALIST", "+#% DAMAGE PER 5 STAT", MOD_ADD_DMG, 0, 1.2, MOD_X_CRIT, 0, 10, "[x]10% CRIT DAMAGE" },
    { "EXPLOIT", "+#% VULNERABLE DAMAGE PER 5 STAT", MOD_VULN_DMG, 0, 2.0, MOD_X_VULN, 0, 8, "[x]8% VS VULNERABLE" },
    { "RANGER", "+#% DAMAGE TO DISTANT PER 5 STAT", MOD_ADD_FAR, 0, 1.5, MOD_X_ALL, 0, 6, "[x]6% DAMAGE" },
    { "UNDYING", "+#% MINION DAMAGE PER 5 STAT", MOD_ADD_TAG, TAG_MINION, 2.5, MOD_X_MINION, 0, 15,
      "[x]15% MINION DAMAGE" },
    { "DOMINANCE", "+#% CORE SKILL DAMAGE PER 5 STAT", MOD_ADD_CORE, 0, 1.5, MOD_X_CORE, 0, 8, "[x]8% CORE DAMAGE" },
    { "EARTH AND SKY", "+#% OVERPOWER DAMAGE PER 5 STAT", MOD_OP_DMG, 0, 3.0, MOD_X_OP, 0, 12,
      "[x]12% OVERPOWER DAMAGE" },
    { "TEMPEST", "+#% STORM SKILL DAMAGE PER 5 STAT", MOD_ADD_TAG, TAG_STORM, 2.0, MOD_X_TAG, TAG_STORM, 10,
      "[x]10% STORM DAMAGE" },
    { "APEX", "+#% DAMAGE PER 5 STAT", MOD_ADD_DMG, 0, 1.2, MOD_X_ALL, 0, 6, "[x]6% DAMAGE" },
    { "VENOM", "+#% POISON DAMAGE PER 5 STAT", MOD_ADD_ELEM, EL_POISON, 2.0, MOD_X_DOT, 0, 10,
      "[x]10% DAMAGE OVER TIME" },
};

const ParaLegend para_legends[CLASS_COUNT][PARAGON_BOARDS - 1] = {
    { { "BLOODBATH", "[x]30% DAMAGE OVER TIME", MOD_X_DOT, 0, 30 },
      { "DECIMATOR", "[x]15% DAMAGE", MOD_X_ALL, 0, 15 },
      { "BONE BREAKER", "[x]40% OVERPOWER DAMAGE", MOD_X_OP, 0, 40 } },
    { { "ARCANE FURNACE", "[x]25% FIRE DAMAGE", MOD_X_ELEM, EL_FIRE, 25 },
      { "FROZEN MIRROR", "[x]25% DAMAGE TO CROWD CONTROLLED", MOD_X_CC, 0, 25 },
      { "STATIC SURGE", "[x]25% LIGHTNING DAMAGE", MOD_X_ELEM, EL_LIGHT, 25 } },
    { { "EXPLOITER", "[x]25% DAMAGE TO VULNERABLE", MOD_X_VULN, 0, 25 },
      { "NO WITNESSES", "[x]15% DAMAGE", MOD_X_ALL, 0, 15 },
      { "TOXIN", "[x]25% POISON DAMAGE", MOD_X_ELEM, EL_POISON, 25 } },
    { { "BLOODBORNE", "[x]25% BLOOD SKILL DAMAGE", MOD_X_TAG, TAG_BLOOD, 25 },
      { "BONE GRAFT", "[x]25% BONE SKILL DAMAGE", MOD_X_TAG, TAG_BONE, 25 },
      { "FLESH EATER", "[x]40% MINION DAMAGE", MOD_X_MINION, 0, 40 } },
    { { "THUNDERSTRUCK", "[x]25% STORM SKILL DAMAGE", MOD_X_TAG, TAG_STORM, 25 },
      { "EARTHEN DEVASTATION", "[x]25% EARTH SKILL DAMAGE", MOD_X_TAG, TAG_EARTH, 25 },
      { "ANCESTRAL GUIDANCE", "[x]15% DAMAGE", MOD_X_ALL, 0, 15 } },
    { { "SPIRIT FURY", "[x]25% JAGUAR SKILL DAMAGE", MOD_X_TAG, TAG_JAGUAR, 25 },
      { "EAGLE'S DOMINION", "[x]25% EAGLE SKILL DAMAGE", MOD_X_TAG, TAG_EAGLE, 25 },
      { "PLAGUE CARRIER", "[x]30% DAMAGE OVER TIME", MOD_X_DOT, 0, 30 } },
};

/* ------------------------------------------------------- board layout */

static ParaNode g_board[PARAGON_BOARDS][N][N];
static int g_cls = -1;

static const struct { uint8_t mod; double v; } magic_nodes[] = {
    { MOD_ADD_DMG, 2.5 }, { MOD_LIFE_PCT, 1.5 }, { MOD_ARMOR_PCT, 2.0 }, { MOD_RES_ALL, 1.5 },
    { MOD_ADD_CORE, 3.0 }, { MOD_CRIT_DMG, 3.0 }, { MOD_VULN_DMG, 3.0 }, { MOD_OP_DMG, 4.0 },
};
static const struct { uint8_t mod; double v; uint8_t mod2; double v2; } rare_nodes[4] = {
    { MOD_ADD_DMG, 8, MOD_MAINSTAT, 10 }, { MOD_CRIT_DMG, 10, MOD_LIFE_PCT, 3 },
    { MOD_VULN_DMG, 10, MOD_DR, 2 }, { MOD_OP_DMG, 12, MOD_ARMOR_PCT, 4 },
};

static void set_node(ParaNode *n, int type, int mod, int arg, double v)
{
    if (n->type != PN_NONE && type == PN_NORMAL)
        return;                         /* never overwrite a special node */
    memset(n, 0, sizeof *n);
    n->type = (uint8_t)type;
    n->mod = (uint8_t)mod;
    n->arg = (uint8_t)arg;
    n->value = v;
}

static void carve_line(ParaNode b[N][N], int x0, int y0, int x1, int y1)
{
    int x = x0, y = y0;
    for (;;) {
        set_node(&b[y][x], PN_NORMAL, MOD_MAINSTAT, 0, 5);
        if (x == x1 && y == y1)
            break;
        x += SIGN(x1 - x);
        y += SIGN(y1 - y);
    }
}

static bool next_to_node(ParaNode b[N][N], int x, int y)
{
    return (x > 0 && b[y][x - 1].type) || (x < N - 1 && b[y][x + 1].type)
        || (y > 0 && b[y - 1][x].type) || (y < N - 1 && b[y + 1][x].type);
}

/* Normal nodes become Magic nodes now and then, and blobs grow off the paths. */
static void decorate(ParaNode b[N][N], Rng *r)
{
    int i, x, y;
    for (i = 0; i < 60; i++) {
        x = rng_range(r, 1, N - 2);
        y = rng_range(r, 1, N - 2);
        if (!b[y][x].type && next_to_node(b, x, y))
            set_node(&b[y][x], PN_NORMAL, MOD_MAINSTAT, 0, 5);
    }
    for (y = 0; y < N; y++)
        for (x = 0; x < N; x++)
            if (b[y][x].type == PN_NORMAL && rng_range(r, 0, 99) < 30) {
                int k = rng_range(r, 0, (int)(sizeof magic_nodes / sizeof magic_nodes[0]) - 1);
                b[y][x].type = PN_MAGIC;
                b[y][x].mod = magic_nodes[k].mod;
                b[y][x].value = magic_nodes[k].v;
            }
}

static void place_specials(ParaNode b[N][N], int cls, int board)
{
    static const int rx[4] = { 3, 11, 3, 11 }, ry[4] = { 3, 3, 11, 11 };
    int i;
    for (i = 0; i < 4; i++) {
        ParaNode *n = &b[ry[i]][rx[i]];
        set_node(n, PN_RARE, rare_nodes[i].mod, 0, rare_nodes[i].v);
        n->mod2 = rare_nodes[i].mod2;
        n->value2 = rare_nodes[i].v2;
    }
    set_node(&b[MID][MID], PN_GLYPH, 0, 0, 0);
    set_node(&b[N - 1][MID], PN_START, MOD_MAINSTAT, 0, 5);
    if (board < PARAGON_BOARDS - 1)
        set_node(&b[0][MID], PN_GATE, MOD_MAINSTAT, 0, 5);
    if (board > 0) {
        const ParaLegend *l = &para_legends[cls][board - 1];
        set_node(&b[4][MID], PN_LEGEND, l->mod, l->arg, l->value);
    }
}

static void build_board(int cls, int board)
{
    ParaNode (*b)[N] = g_board[board];
    Rng r;
    memset(g_board[board], 0, sizeof g_board[board]);
    rng_seed(&r, 0xB0A2Du + (uint32_t)(cls * 16 + board) * 7919u);
    carve_line(b, MID, N - 1, MID, board < PARAGON_BOARDS - 1 ? 0 : 1);   /* spine */
    carve_line(b, 2, MID, N - 3, MID);                                    /* cross */
    carve_line(b, 3, 3, 11, 3);                                           /* ring */
    carve_line(b, 3, 11, 11, 11);
    carve_line(b, 3, 3, 3, 11);
    carve_line(b, 11, 3, 11, 11);
    decorate(b, &r);
    place_specials(b, cls, board);
}

static void ensure_boards(int cls)
{
    int b;
    cls = CLAMP(cls, 0, CLASS_COUNT - 1);
    if (g_cls == cls)
        return;
    for (b = 0; b < PARAGON_BOARDS; b++)
        build_board(cls, b);
    g_cls = cls;
}

const ParaNode *paragon_node(int cls, int board, int x, int y)
{
    static const ParaNode none;
    if (board < 0 || board >= PARAGON_BOARDS || x < 0 || y < 0 || x >= N || y >= N)
        return &none;
    ensure_boards(cls);
    return &g_board[board][y][x];
}

/* ---------------------------------------------------------- ownership */

bool paragon_owned(const Profile *p, int board, int x, int y)
{
    int i = y * N + x;
    if (board < 0 || board >= PARAGON_BOARDS || x < 0 || y < 0 || x >= N || y >= N)
        return false;
    return (p->para[board][i >> 3] >> (i & 7)) & 1u;
}

bool paragon_board_open(const Profile *p, int board)
{
    return board == 0 || (board > 0 && board < PARAGON_BOARDS && paragon_owned(p, board - 1, MID, 0));
}

int paragon_spent(const Profile *p)
{
    int b, i, n = 0;
    for (b = 0; b < PARAGON_BOARDS; b++)
        for (i = 0; i < BOARD_BYTES; i++) {
            uint8_t v = p->para[b][i];
            while (v) { n += v & 1u; v >>= 1; }
        }
    return n;
}

int paragon_points_for(int level, int paragon_level)
{
    int lv = MIN(level, LEVEL_CAP);
    return (lv >= PARAGON_START ? (lv - PARAGON_START + 1) * 4 : 0) + MAX(paragon_level, 0);
}

static int available(const Profile *p)
{
    return paragon_points_for(p->level, p->paragon_level) - paragon_spent(p);
}

bool paragon_can_buy(const Profile *p, int board, int x, int y)
{
    const ParaNode *n = paragon_node(p->cls, board, x, y);
    if (n->type == PN_NONE || available(p) <= 0 || !paragon_board_open(p, board) || paragon_owned(p, board, x, y))
        return false;
    return n->type == PN_START || paragon_owned(p, board, x - 1, y) || paragon_owned(p, board, x + 1, y)
        || paragon_owned(p, board, x, y - 1) || paragon_owned(p, board, x, y + 1);
}

bool paragon_buy(Profile *p, int board, int x, int y)
{
    int i = y * N + x;
    if (!paragon_can_buy(p, board, x, y))
        return false;
    p->para[board][i >> 3] |= (uint8_t)(1u << (i & 7));
    return true;
}

void paragon_refund(Profile *p)
{
    memset(p->para, 0, sizeof p->para);
    memset(p->glyph, 0, sizeof p->glyph);
}

/* -------------------------------------------------------------- glyphs */

int glyph_radius(int level) { return level >= 46 ? 5 : level >= 15 ? 4 : 3; }

int glyph_stat_in_radius(const Profile *p, int board)
{
    int g = p->glyph[board], lvl, r, x, y, stat = 0;
    if (!g)
        return 0;
    lvl = MAX(1, p->glyph_lvl[(g - 1) % GLYPH_COUNT]);
    r = glyph_radius(lvl);
    for (y = MID - r; y <= MID + r; y++)
        for (x = MID - r; x <= MID + r; x++) {
            const ParaNode *n = paragon_node(p->cls, board, x, y);
            if (paragon_owned(p, board, x, y) && n->mod == MOD_MAINSTAT)
                stat += (int)n->value;
            if (paragon_owned(p, board, x, y) && n->mod2 == MOD_MAINSTAT)
                stat += (int)n->value2;
        }
    return stat;
}

bool glyph_socket(Profile *p, int board, int glyph)
{
    int id = (p->cls % CLASS_COUNT) * 2 + (glyph & 1), b;
    if (board < 0 || board >= PARAGON_BOARDS || !paragon_owned(p, board, MID, MID))
        return false;
    for (b = 0; b < PARAGON_BOARDS; b++)
        if (p->glyph[b] == id + 1)
            p->glyph[b] = 0;              /* a glyph sits in one socket at a time */
    p->glyph[board] = (uint8_t)(id + 1);
    return true;
}

static void glyph_mods(const Profile *p, int board, BuildRT *b)
{
    int g = p->glyph[board], lvl, stat;
    const GlyphDef *d;
    if (!g)
        return;
    d = &glyph_defs[(g - 1) % GLYPH_COUNT];
    lvl = MAX(1, p->glyph_lvl[(g - 1) % GLYPH_COUNT]);
    stat = glyph_stat_in_radius(p, board);
    build_add_mod(b, d->mod, d->arg, d->per5 * (stat / 5) * (1.0 + 0.1 * (lvl - 1)));
    if (stat >= 25)
        build_add_mod(b, d->bonus, d->bonus_arg, d->bonus_v);
}

void paragon_mods(const Profile *p, BuildRT *b)
{
    int bd, x, y;
    for (bd = 0; bd < PARAGON_BOARDS; bd++) {
        for (y = 0; y < N; y++)
            for (x = 0; x < N; x++)
                if (paragon_owned(p, bd, x, y)) {
                    const ParaNode *n = paragon_node(p->cls, bd, x, y);
                    build_add_mod(b, n->mod, n->arg, n->value);
                    build_add_mod(b, n->mod2, n->arg2, n->value2);
                }
        glyph_mods(p, bd, b);
    }
}

/* ------------------------------------------------------------ autoplan */

/* Buy along the shortest path of nodes from what we own to (tx, ty). */
static bool buy_toward(Profile *p, int board, int tx, int ty)
{
    int16_t prev[N * N];
    int queue[N * N], head = 0, tail = 0, i, cur = -1;
    if (paragon_owned(p, board, tx, ty) || !paragon_board_open(p, board))
        return false;
    for (i = 0; i < N * N; i++) {
        prev[i] = -2;
        if (paragon_owned(p, board, i % N, i / N) || (i == (N - 1) * N + MID && !paragon_owned(p, board, MID, N - 1)))
            prev[i] = -1, queue[tail++] = i;
    }
    while (head < tail) {
        static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
        int c = queue[head++], d;
        if (c == ty * N + tx) { cur = c; break; }
        for (d = 0; d < 4; d++) {
            int nx = c % N + dx[d], ny = c / N + dy[d];
            if (nx < 0 || ny < 0 || nx >= N || ny >= N || prev[ny * N + nx] != -2
                || paragon_node(p->cls, board, nx, ny)->type == PN_NONE)
                continue;
            prev[ny * N + nx] = (int16_t)c;
            queue[tail++] = ny * N + nx;
        }
    }
    if (cur < 0)
        return false;
    /* Walk back to the owned frontier, then buy forward. */
    {
        int path[N * N], n = 0;
        for (i = cur; i >= 0 && !paragon_owned(p, board, i % N, i / N); i = prev[i])
            path[n++] = i;
        while (n > 0 && paragon_buy(p, board, path[n - 1] % N, path[n - 1] / N))
            n--;
        return n == 0;
    }
}

static void auto_glyphs(Profile *p)
{
    const BuildPreset *pr = &class_defs[p->cls % CLASS_COUNT].preset[p->preset % PRESETS];
    int b, used = 0;
    for (b = 0; b < PARAGON_BOARDS; b++)
        if (p->glyph[b]) used |= 1 << ((p->glyph[b] - 1) & 1);
    for (b = 0; b < PARAGON_BOARDS; b++) {
        int want = (used >> pr->glyph) & 1 ? !pr->glyph : pr->glyph;
        if (!p->glyph[b] && paragon_owned(p, b, MID, MID) && !((used >> want) & 1)) {
            glyph_socket(p, b, want);
            used |= 1 << want;
        }
    }
}

void paragon_auto(Profile *p)
{
    static const int rx[4] = { 3, 11, 3, 11 }, ry[4] = { 11, 11, 3, 3 };
    int b, k, pass;
    for (b = 0; b < PARAGON_BOARDS && available(p) > 0; b++) {
        buy_toward(p, b, MID, MID);
        if (b > 0)
            buy_toward(p, b, MID, 4);
        for (k = 0; k < 4 && available(p) > 0; k++)
            buy_toward(p, b, rx[k], ry[k]);
        if (b < PARAGON_BOARDS - 1)
            buy_toward(p, b, MID, 0);
    }
    /* Leftover points: fill buyable nodes, nearest boards first, pass after
     * pass (each bought node can open its neighbours). */
    for (pass = 0; pass < N * 2 && available(p) > 0; pass++) {
        bool bought = false;
        for (b = 0; b < PARAGON_BOARDS && available(p) > 0; b++) {
            int x, y;
            for (y = N - 1; y >= 0; y--)
                for (x = 0; x < N; x++)
                    bought |= paragon_buy(p, b, x, y);
        }
        if (!bought)
            break;
    }
    auto_glyphs(p);
}

/* ---------------------------------------------------------------- text */

static const char *mod_label(int mod)
{
    switch (mod) {
    case MOD_MAINSTAT:  return "MAIN STAT";
    case MOD_ADD_DMG:   return "% DAMAGE";
    case MOD_LIFE_PCT:  return "% MAXIMUM LIFE";
    case MOD_ARMOR_PCT: return "% ARMOR";
    case MOD_RES_ALL:   return "% ALL RESISTANCE";
    case MOD_ADD_CORE:  return "% CORE SKILL DAMAGE";
    case MOD_CRIT_DMG:  return "% CRIT DAMAGE";
    case MOD_VULN_DMG:  return "% VULNERABLE DAMAGE";
    case MOD_OP_DMG:    return "% OVERPOWER DAMAGE";
    case MOD_DR:        return "% DAMAGE REDUCTION";
    default:            return "";
    }
}

void paragon_node_text(char *out, size_t cap, const ParaNode *n, int cls)
{
    const char *stat = stat_name((MainStat)class_defs[cls % CLASS_COUNT].main_stat);
    switch (n->type) {
    case PN_NONE:   snprintf(out, cap, "-"); break;
    case PN_GLYPH:  snprintf(out, cap, "%s", T("GLYPH SOCKET")); break;
    case PN_LEGEND: snprintf(out, cap, "%s", T("LEGENDARY NODE")); break;
    case PN_RARE:
        snprintf(out, cap, T("RARE: +%d%s, +%d%s"), (int)n->value, T(mod_label(n->mod)), (int)n->value2,
                 T(n->mod2 == MOD_MAINSTAT ? " MAIN STAT" : mod_label(n->mod2)));
        break;
    case PN_MAGIC:
        snprintf(out, cap, T("MAGIC: +%d.%d%s"), (int)n->value, (int)(n->value * 10) % 10, T(mod_label(n->mod)));
        break;
    default:
        snprintf(out, cap, "%s: +%d %s", T(n->type == PN_GATE ? "GATE" : n->type == PN_START ? "START" : "NORMAL"),
                 (int)n->value, T(stat));
        break;
    }
}

void glyph_text(char *out, size_t cap, int glyph, int level)
{
    const GlyphDef *d = &glyph_defs[glyph % GLYPH_COUNT];
    double v = d->per5 * (1.0 + 0.1 * (MAX(level, 1) - 1));
    char num[12];
    const char *s;
    size_t n = 0;
    snprintf(num, sizeof num, "%d.%d", (int)v, (int)(v * 10) % 10);
    for (s = T(d->desc); *s && n + 1 < cap; s++) {
        if (*s == '#') {
            const char *q = num;
            while (*q && n + 1 < cap)
                out[n++] = *q++;
        } else {
            out[n++] = *s;
        }
    }
    out[n] = '\0';
}
