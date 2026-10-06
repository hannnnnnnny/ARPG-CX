/*
 * fixed.h - 24.8 fixed point used for all positions and velocities.
 *
 * The ARM926EJ-S in the TI-Nspire CX has no FPU; soft-float is slow, so the
 * simulation is integer-only. 1 pixel == 256 units.
 *
 * FX() is meant for compile-time constants (e.g. FX(0.25)); compilers fold
 * the float literal so no float math reaches the calculator at runtime.
 * Right-shifting negative values relies on arithmetic shift, which GCC and
 * Clang guarantee on every target we build for.
 */
#ifndef AD_FIXED_H
#define AD_FIXED_H

#include <stdint.h>

typedef int32_t fx;

#define FX_SHIFT 8
#define FX_ONE   256
#define FX(v)        ((fx)((v) * FX_ONE))
#define FX_FROM_INT(i) ((fx)(i) * FX_ONE)
#define FX_TO_INT(v)   ((int)((v) >> FX_SHIFT)) /* floor */

#endif
