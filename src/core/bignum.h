/*
 * bignum.h - idle-game number formatting.
 *
 * Gold, damage and HP grow exponentially, so they are stored as double
 * (soft-float on the calculator, but only touched a few times per hit, never
 * per pixel) and displayed with short suffixes: 950, 12.3K, 4.56M ... 1.2e45.
 */
#ifndef AD_BIGNUM_H
#define AD_BIGNUM_H

#include <stddef.h>

void fmt_num(char *out, size_t cap, double v);

#endif
