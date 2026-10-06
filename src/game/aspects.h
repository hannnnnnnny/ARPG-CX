/*
 * aspects.h - legendary aspects (imprintable powers), unique and mythic
 * unique items, and the Codex of Power that remembers every aspect found.
 *
 * Names and texts are original to Ashen Depths; the systems follow the
 * Diablo IV loot design (aspect categories, codex, uniques, mythics).
 */
#ifndef AD_ASPECTS_H
#define AD_ASPECTS_H

#include "defs.h"
#include "build.h"

typedef enum { ASC_OFFENSE, ASC_DEFENSE, ASC_UTILITY, ASC_RESOURCE, ASC_MOBILITY, ASC_COUNT } AspectCat;

#define ANY_CLASS 0xFF

typedef struct {
    const char *name;
    const char *desc;     /* '#' is replaced by the rolled value */
    uint8_t cls;          /* HeroClass or ANY_CLASS */
    uint8_t cat;          /* AspectCat */
    uint8_t kind, arg;    /* rolled Mod */
    double  lo, hi;
    uint8_t kind2, arg2;  /* optional fixed Mod */
    double  v2;
} AspectDef;

typedef struct {
    const char *name;
    const char *desc;     /* unique power, '#' = rolled value */
    uint8_t slot, base, cls, mythic;
    uint8_t aff[4];       /* fixed affix types */
    uint8_t kind, arg;
    double  lo, hi;
    uint8_t kind2, arg2;
    double  v2;
} UniqueDef;

extern const AspectDef aspect_defs[];
extern const int aspect_count;          /* ids 1..aspect_count */
extern const UniqueDef unique_defs[];
extern const int unique_count;          /* ids 1..unique_count */

const AspectDef *aspect_def(int id);    /* NULL if out of range */
const UniqueDef *unique_def(int id);
/* A build-defining unique (sig.h): the auto equip never takes it off. */
bool item_is_signature(const Item *it);
double aspect_value(const AspectDef *a, int roll);      /* roll 0..1000 */
double unique_value(const UniqueDef *u, int roll);
/* Text with '#' filled in. */
void aspect_text(char *out, size_t cap, const AspectDef *a, int roll);
void unique_text(char *out, size_t cap, const UniqueDef *u, int roll);
const char *aspect_cat_name(AspectCat c);
/* Which aspect categories each slot accepts (bit per AspectCat). */
bool aspect_fits_slot(const AspectDef *a, Slot s);
bool aspect_usable(const AspectDef *a, int cls);

/* Mods granted by an item's legendary aspect or unique power. */
int item_power_mods(const Item *it, int cls, Mod *out, int cap);

/* Codex of Power */
void codex_learn(Profile *p, int aspect, int roll);    /* keeps the best roll */
bool codex_known(const Profile *p, int aspect);
int  codex_roll(const Profile *p, int aspect);          /* -1 if unknown */

#endif
