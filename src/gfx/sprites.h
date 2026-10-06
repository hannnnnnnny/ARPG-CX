/* sprites.h - converts the art grids (data/art*.c) into RGB565 sprites:
 * monsters (plus 2x boss versions), minions, loot icons and skill icons
 * once at startup; the hero on demand from its look and equipped gear. */
#ifndef AD_SPRITES_H
#define AD_SPRITES_H

#include "gfx.h"
#include "../game/defs.h"

#define HERO_FRAMES 4 /* idle, idle-bob, walk A, walk B */
#define HERO_W 16
#define HERO_H 20

/* Everything that changes how the hero looks. Materials: 0 = nothing
 * equipped there, else item rarity + 1 (leather, blue steel, steel,
 * dark iron, gold, violet). */
typedef struct {
    uint8_t cls, skin, hair, hair_color, face, eyes, cloth;
    uint8_t helm;             /* 0 = none / hidden, else helm base + 1 */
    uint8_t helm_mat, chest_mat, glove_mat, pants_mat, boot_mat;
    uint8_t chest_tier;       /* chest base 0..3 (pauldrons from 2) */
} HeroLook;

#define SKIN_TONES 6
#define HAIR_COLORS 8
#define EYE_COLORS 4
#define CLOTH_COLORS 8

bool sprites_init(void);
void hero_look_from(HeroLook *l, const Profile *p);
/* Built and cached on first use (a handful of looks stay cached). */
const Sprite *spr_hero_look(const HeroLook *l, int frame);
const Sprite *spr_mon(int type, int frame, bool boss);
const Sprite *spr_ally(int kind, int frame);
const Sprite *spr_icon(int slot, int rarity);
/* 16x16 skill icon: glyph on a background tinted with 'color'. */
const Sprite *spr_skill_icon(int icon, uint16_t color);
uint16_t palette_color(char c);
uint16_t material_color(int mat);   /* main colour of a gear material */
uint16_t skin_color(int i);
uint16_t hair_color(int i);
uint16_t cloth_color(int i);

#endif
