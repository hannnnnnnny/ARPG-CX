/* save_io.h - little-endian field readers / writers shared by save.c and
 * save_legacy.c (internal). Readers flag errors instead of reading past
 * the end, so a truncated file can never crash the loader. */
#ifndef AD_SAVE_IO_H
#define AD_SAVE_IO_H

#include "../core/common.h"
#include <string.h>

typedef struct { uint8_t *p; size_t n, cap; bool ok; } Writer;
typedef struct { const uint8_t *p; size_t n, len; bool ok; int version; } Reader;

static inline void w8(Writer *w, uint8_t v)
{
    if (w->n >= w->cap) { w->ok = false; return; }
    w->p[w->n++] = v;
}
static inline void w16(Writer *w, uint16_t v) { w8(w, (uint8_t)v); w8(w, (uint8_t)(v >> 8)); }
static inline void w32(Writer *w, uint32_t v) { int i; for (i = 0; i < 4; i++) w8(w, (uint8_t)(v >> (8 * i))); }
static inline void wf(Writer *w, double v)
{
    uint64_t u;
    int i;
    memcpy(&u, &v, 8); /* IEEE-754 double on every supported target */
    for (i = 0; i < 8; i++) w8(w, (uint8_t)(u >> (8 * i)));
}

static inline uint8_t r8(Reader *r)
{
    if (r->n >= r->len) { r->ok = false; return 0; }
    return r->p[r->n++];
}
static inline uint16_t r16(Reader *r) { uint16_t v = r8(r); return (uint16_t)(v | (uint16_t)(r8(r) << 8)); }
static inline uint32_t r32(Reader *r)
{
    uint32_t v = 0;
    int i;
    for (i = 0; i < 4; i++) v |= (uint32_t)r8(r) << (8 * i);
    return v;
}
static inline double rf(Reader *r)
{
    uint64_t u = 0;
    double v;
    int i;
    for (i = 0; i < 8; i++) u |= (uint64_t)r8(r) << (8 * i);
    memcpy(&v, &u, 8);
    if (v != v || v > 1e300 || v < -1e300) { r->ok = false; return 0; } /* NaN / inf */
    return v;
}

#endif
