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

#endif
