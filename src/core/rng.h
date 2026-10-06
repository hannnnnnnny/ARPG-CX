/* rng.h - tiny deterministic PRNG (xorshift32). Deterministic so desktop
 * replays and host tests behave identically to the calculator build. */
#ifndef AD_RNG_H
#define AD_RNG_H

#include <stdint.h>

typedef struct { uint32_t s; } Rng;

static inline void rng_seed(Rng *r, uint32_t seed) { r->s = seed ? seed : 0x9E3779B9u; }

static inline uint32_t rng_next(Rng *r)
{
    uint32_t x = r->s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->s = x;
    return x;
}

/* Uniform-ish integer in [lo, hi]. */
static inline int rng_range(Rng *r, int lo, int hi)
{
    return lo + (int)(rng_next(r) % (uint32_t)(hi - lo + 1));
}

#endif
