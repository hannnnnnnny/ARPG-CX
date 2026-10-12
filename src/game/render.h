/* render.h - battle view and HUD drawing. */
#ifndef AD_RENDER_H
#define AD_RENDER_H

#include "world.h"

void render_world(const World *w, const Profile *p);
/* Top-left world pixel of the last drawn battle view (mouse to world). */
void render_camera(int *x, int *y);
/* Fraction (0..256) of a tick elapsed since the last logic update. */
void render_set_alpha(int alpha);
void render_hud(const World *w, const Profile *p);
/* Desktop side panels (map, build, loot, event) into the bound FB_W x FB_H
 * buffer: left in logical columns 0..79, right in 80..159. NULL world or
 * profile draws empty frames (title screen, menus without a run). */
void render_side(const World *w, const Profile *p);

#endif
