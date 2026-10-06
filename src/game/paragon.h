/*
 * paragon.h - the Paragon board (Diablo IV style).
 *
 * From level 50 the hero earns paragon points. Each class has four 15x15
 * boards, generated deterministically from a fixed seed so every hero of a
 * class sees the same layout. Nodes are bought next to nodes you already
 * own: Normal (+5 main stat), Magic (+%), Rare (bigger mixed bonus),
 * Legendary (a build-defining [x] multiplier, boards 2-4), a Glyph socket
 * in the centre of each board, and a Gate on top that opens the next one.
 *
 * Glyphs grow stronger with the main stat allocated inside their radius,
 * and level up when Torment guardians fall (radius grows at level 15).
 */
#ifndef AD_PARAGON_H
#define AD_PARAGON_H

#include "defs.h"
#include "build.h"

typedef enum { PN_NONE, PN_START, PN_NORMAL, PN_MAGIC, PN_RARE, PN_LEGEND, PN_GLYPH, PN_GATE } NodeType;

typedef struct {
    uint8_t type;      /* NodeType */
    uint8_t mod, arg;  /* main bonus (Normal: main stat) */
    double  value;
    uint8_t mod2, arg2;
    double  value2;
} ParaNode;

typedef struct {
    const char *name;
    const char *desc;  /* bonus per 5 main stat in radius */
    uint8_t mod, arg;
    double  per5;      /* at level 1 */
    uint8_t bonus, bonus_arg;   /* extra once 25 main stat is in radius */
    double  bonus_v;
    const char *bonus_desc;
} GlyphDef;

typedef struct {
    const char *name;
    const char *desc;
    uint8_t mod, arg;
    double  value;
} ParaLegend;

extern const GlyphDef glyph_defs[GLYPH_COUNT];   /* class c owns 2c and 2c+1 */
extern const ParaLegend para_legends[CLASS_COUNT][PARAGON_BOARDS - 1];

const ParaNode *paragon_node(int cls, int board, int x, int y);
bool paragon_owned(const Profile *p, int board, int x, int y);
bool paragon_board_open(const Profile *p, int board);
bool paragon_can_buy(const Profile *p, int board, int x, int y);
bool paragon_buy(Profile *p, int board, int x, int y);
int  paragon_spent(const Profile *p);
void paragon_refund(Profile *p);
/* Spend all points toward glyph sockets, rares, legendaries and gates. */
void paragon_auto(Profile *p);
/* Add every allocated node, glyph and legendary to the build. */
void paragon_mods(const Profile *p, BuildRT *b);
int  glyph_radius(int level);
/* Main stat allocated within the glyph radius on a board. */
int  glyph_stat_in_radius(const Profile *p, int board);
bool glyph_socket(Profile *p, int board, int glyph);   /* glyph = id (class-local 0/1) */
void paragon_node_text(char *out, size_t cap, const ParaNode *n, int cls);
void glyph_text(char *out, size_t cap, int glyph, int level);
/* Points available at a level / paragon level. */
int  paragon_points_for(int level, int paragon_level);

#endif
