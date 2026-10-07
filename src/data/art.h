/* art.h - original pixel art grids (art.c monsters, art_hero.c hero parts,
 * companions and loot icons, art_icons.c skill icons). */
#ifndef AD_ART_H
#define AD_ART_H

#define MON_ART_COUNT 8   /* the 7 monster kinds and the butcher */
#define HAIR_STYLES 6
#define FACE_STYLES 5
#define SLOT_ICONS 9   /* weapon, off-hand, helm, chest, gloves, pants, boots, amulet, ring */

extern const char *const art_hero_head[7];
extern const char *const art_hair[HAIR_STYLES][7];
extern const char *const art_face[FACE_STYLES][7];
extern const char *const art_helm[4][7];
extern const char *const art_body[6][7];
extern const char *const art_pauldrons[2][2];
extern const char *const art_legs[2][3][6];
extern const char *const art_wolf[2][16];
extern const char *const art_mon[MON_ART_COUNT][2][16];
extern const char *const art_icons[SLOT_ICONS][8];

#endif
