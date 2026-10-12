/*
 * codex.c - unique sources and the codex order (see codex.h).
 */
#include "codex.h"
#include "aspects.h"
#include "balance.h"

int codex_guardian(int unique)
{
    const UniqueDef *u = unique_def(unique);
    if (!u || u->mythic)
        return -1;
    return unique % GUARDIANS;
}

/* Guardian floors of a theme: 10 * (theme + 1), then every 50 floors. */
int codex_guardian_floor_below(int theme, int floor)
{
    int first = 10 * (theme + 1), cycle = 10 * GUARDIANS;
    if (theme < 0 || floor < first)
        return 0;
    return first + (floor - first) / cycle * cycle;
}

int codex_guardian_floor_from(int theme, int floor)
{
    int first = 10 * (theme + 1), cycle = 10 * GUARDIANS;
    if (floor <= first)
        return first;
    return first + (floor - first + cycle - 1) / cycle * cycle;
}

int codex_pick(int theme, int cls, Rng *r)
{
    int ids[UNIQUE_BITS], n = 0, i;
    for (i = 1; i <= unique_count && n < UNIQUE_BITS; i++) {
        const UniqueDef *u = unique_def(i);
        if (codex_guardian(i) == theme && (u->cls == ANY_CLASS || u->cls == cls))
            ids[n++] = i;
    }
    return n ? ids[rng_range(r, 0, n - 1)] : 0;
}

int codex_entries(int *out, int cap)
{
    int n = 0, g, i;
    for (i = 1; i <= unique_count && n < cap; i++)
        if (codex_guardian(i) < 0)
            out[n++] = i;
    for (g = 0; g < GUARDIANS; g++)
        for (i = 1; i <= unique_count && n < cap; i++)
            if (codex_guardian(i) == g)
                out[n++] = i;
    return n;
}
