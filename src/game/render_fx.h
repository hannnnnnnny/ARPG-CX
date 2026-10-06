/*
 * render_fx.h - per-skill visual effects, projectiles and weapons.
 *
 * Every class skill has its own look (a VS_* style picked from its VX id),
 * tinted by the element it currently deals - so an upgrade that turns
 * Leap Slam into fire also turns its effect into fire. All effects are a
 * few dozen pixel / line draws, cheap enough for the calculator's ARM9.
 */
#ifndef AD_RENDER_FX_H
#define AD_RENDER_FX_H

#include "world.h"

/* Visual styles; each class skill picks one (VX id -> style). */
typedef enum {
    VS_DEFAULT, VS_FLAY, VS_SLASH, VS_WHIRL, VS_HAMMER, VS_CLAW, VS_WARCRY, VS_BARRIER, VS_LEAP, VS_QUAKE, VS_ULT,
    VS_SPARK, VS_FROSTBOLT, VS_FIREBALL, VS_SHARD, VS_FROSTNOVA, VS_METEOR, VS_STORM, VS_TEMPEST,
    VS_DAGGER, VS_ARROW, VS_POWERSHOT, VS_TRAP, VS_SHADOWDASH, VS_IMBUE, VS_RAIN,
    VS_SCYTHE, VS_BONE, VS_BONESPEAR, VS_BLIGHT, VS_BLOODSURGE, VS_PRISON, VS_MIST, VS_RAISE, VS_CORPSE, VS_LEGION,
    VS_MAUL, VS_TORNADO, VS_LANDSLIDE, VS_STONE, VS_CYCLONE, VS_BOULDER,
    VS_THRUST, VS_FIST, VS_QUILL, VS_PALM, VS_STINGER, VS_CENTIPEDE, VS_SOAR, VS_JAGUAR,
    VS_BURN, VS_POOL
} VStyle;

int  fx_style_of(int vfx);
void fx_draw_effect(const Effect *e, int cam_x, int cam_y);
void fx_draw_ground(const Ground *g, int cam_x, int cam_y);
/* x, y: projectile position on screen. */
void fx_draw_proj(const Proj *pj, int x, int y);
/* kind: WK_* weapon / OK_* off-hand, -1 = none; mat: gear material 0..6. */
void fx_draw_weapon(int kind, int mat, int x, int y, int face, int attack_t, uint16_t accent, int tick);
void fx_draw_offhand(int kind, int mat, int x, int y, int face, int tick);

/* render_sig.c: signature builds (effects, aura, giant spear, shake). */
void fx_draw_sig(const Effect *e, int x, int y);
void render_sig_aura(const World *w, int x, int y);
void render_sig_giant(const Proj *pj, int x, int y);
int  render_sig_shake(const World *w, int axis);
void render_sig_flash(const World *w, int view_h);

/* render_event.c: floor event objects, goblin sack, champion plates. */
void render_event_object(const World *w, int cam_x, int cam_y);
void render_monster_extras(const World *w, const Monster *m, int x, int top);

/* Shared drawing helpers (render_fx.c). */
int  fx_hash(int a, int b);
int  fx_cx(int x, int r, int phase);
int  fx_cy(int y, int r, int phase);
void fx_arc(int x, int y, int r, int from, int span, uint16_t c);
void fx_ellipse(int x, int y, int r, uint16_t c);
void fx_jagged(int x0, int y0, int x1, int y1, int seed, uint16_t c);

#endif
