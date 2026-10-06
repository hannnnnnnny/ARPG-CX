/* world_int.h - internals shared by world*.c and dungeon.c (not public API). */
#ifndef AD_WORLD_INT_H
#define AD_WORLD_INT_H

#include "world.h"

#define HERO_HALF 5   /* collision half-size in px */
#define MON_HALF  5
#define CLOSE_PX  40  /* "close enemies" for damage / damage reduction */

void dungeon_build(World *w, const Profile *p);
void bfs_field(World *w, int16_t field[MAP_H][MAP_W], int cx, int cy);
bool line_of_sight(const World *w, fx x0, fx y0, fx x1, fx y1);
bool body_line_clear(const World *w, fx x0, fx y0, fx x1, fx y1, int half);
void move_body(const World *w, fx *x, fx *y, fx dx, fx dy, int half);
int  dist_px(fx ax, fx ay, fx bx, fx by);
/* Unit step (scaled to 'speed') from (x,y) toward the target point. */
void step_toward(fx x, fx y, fx tx, fx ty, fx speed, fx *dx, fx *dy);
/* Next waypoint following a distance field downhill from the body's cell. */
bool field_waypoint(const World *w, const int16_t field[MAP_H][MAP_W], fx x, fx y, fx *wx, fx *wy);
/* field_waypoint plus look-ahead along the path while the line is clear. */
bool smooth_waypoint(const World *w, const int16_t field[MAP_H][MAP_W], fx x, fx y, int half, fx *wx, fx *wy);

void floater(World *w, int x, int y, const char *text, uint16_t color);
void floater_kind(World *w, int x, int y, const char *text, uint16_t color, FloatKind k);
/* Returns the new effect (or NULL if the pool is full) so callers can set its vfx. */
Effect *effect(World *w, FxKind k, int x, int y, int x2, int y2, int r, int dur, uint16_t color);

/* world_hero.c / world_cast.c */
void hero_update(World *w, Profile *p);
int  count_near(const World *w, int x, int y, int r);
int  densest_pack(const World *w, int range, int r, int *count);
/* A hit carrying skill i's numbers (mult scales the damage). */
Hit  skill_hit(const World *w, int i, double mult);
bool cast_skill(World *w, Profile *p, int i, const SkillRT *s, const Monster *tgt);
void update_channel_strike_dash(World *w, Profile *p);
void hero_add_barrier(World *w, double frac_of_life);
void hero_buff(World *w, int kind, double val, int ticks);
static inline bool hero_has(const World *w, int buff) { return w->h.buff_t[buff] > 0; }
void hero_gain_res(World *w, double amount);

/* world_dmg.c: the single entry point for damaging a monster. */
void deal_damage(World *w, Profile *p, int i, const Hit *h);
void hurt_hero(World *w, Profile *p, double raw, int element, int attacker);
void kill_rewards(World *w, Profile *p, Monster *m);
/* A loot drop near (x, y); NULL when the floor's drop pool is full. */
Drop *world_drop_item(World *w, Profile *p, fx x, fx y, Rarity min, int luck);
/* Warded champions shrug off most damage 1.2 s out of every 4 s. */
static inline bool champion_warded(const World *w, const Monster *m) { return (w->tick + (int)(m - w->mon) * 37) % 120 < 36; }
void tick_dots(World *w, Profile *p, int i);

/* world_mon.c */
void monsters_update(World *w, Profile *p);
void projectiles_update(World *w, Profile *p);
void grounds_update(World *w, Profile *p);
void world_spawn_ground(World *w, int x, int y, int r, int dur, const Hit *h, bool follow, int vfx);
void spawn_monster(World *w, int type, int cx, int cy, bool elite, bool boss);
Proj *spawn_proj(World *w);

/* world_ally.c: minions, corpses, health orbs */
void allies_update(World *w, Profile *p);
int  allies_alive(const World *w);
bool ally_summon(World *w, int kind, int element);
void corpse_add(World *w, fx x, fx y);
int  corpse_best(const World *w, int range, int r);
void orbs_update(World *w);
void orb_drop(World *w, fx x, fx y);

#endif
