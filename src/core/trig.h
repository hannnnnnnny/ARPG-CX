/* trig.h - integer sine for bobbing/waves (no FPU on the calculator). */
#ifndef AD_TRIG_H
#define AD_TRIG_H

#include <stdint.h>

/* sin(2*pi*i/64) * 127 */
static const int8_t SINE64[64] = {
    0, 12, 25, 37, 49, 60, 71, 81, 90, 98, 106, 112, 117, 122, 125, 126,
    127, 126, 125, 122, 117, 112, 106, 98, 90, 81, 71, 60, 49, 37, 25, 12,
    0, -12, -25, -37, -49, -60, -71, -81, -90, -98, -106, -112, -117, -122, -125, -126,
    -127, -126, -125, -122, -117, -112, -106, -98, -90, -81, -71, -60, -49, -37, -25, -12
};

/* Phase in 1/64 turns (any int), result in [-127, 127]. */
static inline int isin(int phase) { return SINE64[phase & 63]; }

#endif
