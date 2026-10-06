#include "build.h"
#include "../gfx/gfx.h"
#include <string.h>

/* Preset field order: name, desc, bar skills, upgrade per bar skill (0 = none), passive priority,
 * key passive, main element, aspects to imprint, glyph. Aspect ids: see aspects.c. */
const ClassDef class_defs[CLASS_COUNT] = {
    [CLASS_BARBARIAN] = { "BARBARIAN", "A WARRIOR OF THE HIGH PEAKS WHO FIGHTS WITH RAGE AND STEEL.", "MELEE - TOUGH",
        RES_FURY, STAT_STR, 1.30, 20, RGB565(220, 40, 40), {
        { "WHIRLWIND BLEED", "SPIN THROUGH THE HORDE AND BLEED IT DRY.", { 0, 2, 4, 5, 6, 9 }, { 1, 1, 1, 2, 1, 2 },
          { 0, 2, 3, 1, 5, 4 }, 2, EL_PHYS, { 19, 20, 2, 9 }, 1 },
        { "EARTHQUAKE", "LEAP AND SLAM: THE GROUND DOES THE REST.", { 0, 3, 7, 8, 5, 9 }, { 2, 1, 2, 2, 1, 1 },
          { 4, 0, 1, 3, 5, 2 }, 3, EL_PHYS, { 22, 21, 4, 9 }, 0 },
        { "BERSERKER", "FAST AXES AND ENDLESS BERSERKING.", { 1, 3, 4, 5, 7, 9 }, { 1, 2, 2, 1, 1, 1 },
          { 3, 0, 4, 5, 1, 2 }, 1, EL_PHYS, { 23, 24, 3, 1 }, 0 } } },
    [CLASS_SORCERER] = { "SORCERER", "WIELDS FIRE, FROST AND LIGHTNING FROM AFAR.", "RANGED - FRAGILE",
        RES_MANA, STAT_INT, 0.85, 100, RGB565(60, 110, 255), {
        { "PYROMANCER", "FIREBALLS THAT EXPLODE TWICE AND RAIN METEORS.", { 1, 2, 5, 6, 7, 9 },
          { 1, 1, 1, 1, 2, 1 }, { 0, 2, 4, 3, 1, 5 }, 1, EL_FIRE, { 59, 60, 25, 26 }, 1 },
        { "CRYOMANCER", "FREEZE EVERYTHING, THEN SHATTER IT.", { 1, 4, 5, 6, 7, 9 }, { 1, 1, 1, 2, 1, 2 },
          { 4, 2, 0, 3, 5, 1 }, 2, EL_COLD, { 28, 27, 5, 10 }, 0 },
        { "STORMCALLER", "CHAIN LIGHTNING THAT NEVER STOPS.", { 0, 3, 8, 5, 6, 9 }, { 2, 1, 1, 1, 1, 0 },
          { 3, 2, 0, 5, 1, 4 }, 3, EL_LIGHT, { 29, 30, 7, 9 }, 0 } } },
    [CLASS_ROGUE] = { "ROGUE", "A SWIFT HUNTER OF BOWS, BLADES, TRAPS AND VENOM.", "RANGED - SWIFT",
        RES_ENERGY, STAT_DEX, 1.00, 110, RGB565(250, 210, 60), {
        { "MARKSMAN", "PIERCING SHOTS THAT CRIT THE HEALTHY.", { 1, 2, 6, 8, 5, 9 }, { 1, 1, 2, 2, 1, 2 },
          { 0, 1, 3, 4, 5, 2 }, 1, EL_PHYS, { 31, 32, 3, 2 }, 1 },
        { "VENOM", "RAPID FIRE THAT DROWNS FOES IN POISON.", { 0, 3, 7, 5, 6, 9 }, { 2, 2, 1, 2, 2, 0 },
          { 2, 1, 0, 3, 4, 5 }, 2, EL_POISON, { 33, 36, 6, 10 }, 0 },
        { "BARRAGE", "ARROWS IN EVERY DIRECTION.", { 0, 4, 6, 8, 5, 9 }, { 1, 1, 2, 2, 1, 2 },
          { 1, 0, 3, 4, 5, 2 }, 3, EL_PHYS, { 34, 35, 1, 7 }, 1 } } },
    [CLASS_NECRO] = { "NECROMANCER", "COMMANDS BONE, BLOOD AND AN ARMY OF THE DEAD.", "RANGED - MINIONS",
        RES_ESSENCE, STAT_INT, 0.95, 90, RGB565(80, 220, 180), {
        { "BONE SPEAR", "LANCES OF BONE THAT BURST INTO STORMS OF SHARDS.", { 1, 2, 5, 8, 6, 9 },
          { 1, 2, 1, 2, 1, 2 }, { 4, 5, 0, 1, 2, 3 }, 1, EL_PHYS, { 57, 58, 37, 3 }, 1 },
        { "SUMMONER", "AN ARMY OF SKELETONS AND ROTTING BLIGHT.", { 0, 3, 7, 8, 6, 9 }, { 1, 1, 1, 1, 2, 1 },
          { 3, 1, 0, 2, 5, 4 }, 2, EL_SHADOW, { 40, 41, 38, 10 }, 0 },
        { "BLOOD SURGE", "DRAIN THE HORDE TO FEED YOUR OWN LIFE.", { 0, 4, 5, 6, 8, 9 }, { 2, 1, 1, 1, 2, 1 },
          { 5, 1, 4, 0, 2, 3 }, 3, EL_PHYS, { 39, 42, 4, 11 }, 1 } } },
    [CLASS_DRUID] = { "DRUID", "A SHAPESHIFTER WHO CALLS THE STORM AND THE EARTH.", "MELEE - STURDY",
        RES_SPIRIT, STAT_WIL, 1.20, 24, RGB565(110, 220, 255), {
        { "STORM", "TORNADOES AND LIGHTNING ALL AROUND.", { 0, 2, 6, 5, 8, 9 }, { 2, 1, 1, 2, 1, 1 },
          { 4, 0, 2, 1, 5, 3 }, 2, EL_LIGHT, { 43, 46, 1, 9 }, 1 },
        { "EARTH", "LANDSLIDES AND BOULDERS THAT OVERPOWER.", { 1, 3, 8, 5, 6, 9 }, { 1, 1, 2, 1, 1, 1 },
          { 3, 0, 1, 5, 2, 4 }, 1, EL_PHYS, { 44, 46, 4, 9 }, 0 },
        { "STORM WEREWOLF", "SHRED CALLS DOWN THE SKY. NEVER STOPS, NEVER FALLS.", { 0, 4, 6, 7, 5, 9 },
          { 1, 1, 1, 2, 2, 1 }, { 0, 2, 3, 1, 5, 4 }, 3, EL_LIGHT, { 55, 56, 47, 3 }, 0 } } },
    [CLASS_SPIRITBORN] = { "SPIRITBORN", "A JUNGLE WARRIOR BOUND TO FOUR SPIRIT GUARDIANS.", "MELEE - QUICK",
        RES_VIGOR, STAT_DEX, 1.10, 22, RGB565(230, 200, 90), {
        { "EAGLE QUILLS", "VOLLEYS OF QUILLS FROM THE SKY.", { 0, 2, 7, 6, 8, 9 }, { 1, 1, 1, 2, 1, 1 },
          { 3, 1, 2, 0, 5, 4 }, 2, EL_PHYS, { 49, 54, 2, 9 }, 0 },
        { "JAGUAR", "CRUSHING PALMS AND A RISING FEROCITY.", { 0, 3, 8, 6, 7, 9 }, { 1, 1, 1, 2, 1, 1 },
          { 1, 3, 5, 2, 0, 4 }, 1, EL_PHYS, { 50, 53, 4, 9 }, 0 },
        { "CENTIPEDE", "STINGERS AND SWARMS OF VENOM.", { 1, 4, 5, 6, 8, 9 }, { 1, 1, 1, 1, 1, 1 },
          { 4, 0, 2, 1, 3, 5 }, 3, EL_POISON, { 51, 52, 6, 10 }, 1 } } },
};

const char *resource_name(ResourceKind r)
{
    static const char *const n[RES_COUNT] = { "FURY", "MANA", "ENERGY", "ESSENCE", "SPIRIT", "VIGOR" };
    return n[r % RES_COUNT];
}

const char *stat_name(MainStat s)
{
    static const char *const n[STAT_COUNT] = { "STRENGTH", "INTELLIGENCE", "DEXTERITY", "WILLPOWER" };
    return n[s % STAT_COUNT];
}

/* -------------------------------------------------------------- mods */

void build_clear(BuildRT *b)
{
    int i;
    memset(b, 0, sizeof *b);
    b->x_all = b->x_vuln = b->x_crit = b->x_op = b->x_cc = b->x_dot = b->x_core = b->x_minion = b->x_life = 1.0;
    for (i = 0; i < EL_COUNT; i++) b->x_elem[i] = 1.0;
    for (i = 0; i < CLASS_SKILLS; i++) b->x_skill[i] = 1.0;
    for (i = 0; i < TAG_COUNT; i++) b->x_tag[i] = 1.0;
}

static void mult(double *x, double pct) { *x *= 1.0 + pct / 100.0; }

/* A build-defining unique: remember the strongest; resolve_build decides
 * whether the build actually uses it. */
static void signature_mod(BuildRT *b, int sig, double power)
{
    if (sig <= SIG_NONE || sig >= SIG_COUNT || power <= b->sig_power)
        return;
    b->sig = (uint8_t)sig;
    b->sig_power = power;
}

static int signature_skill(int sig)
{
    return sig == SIG_STORMWOLF ? SIG_SKILL_SHRED : sig == SIG_BONESPEAR ? SIG_SKILL_BONESPEAR : SIG_SKILL_FIREBALL;
}

/* The signature only wakes with its skill on the bar; then it reshapes
 * that skill (Stormhowl Pelt: Shred becomes free lightning) and makes the
 * hero tanky enough to stand in the middle of it. */
static void apply_signature(const Profile *p, BuildRT *b)
{
    if (!b->sig)
        return;
    if (!skill_on_bar(p, signature_skill(b->sig))) {
        b->sig = SIG_NONE;
        b->sig_power = 0;
        return;
    }
    b->x_life *= 8.0;                        /* [x]: paragon's additive life can't drown it */
    build_add_mod(b, MOD_DR, 0, 40);
    if (b->sig == SIG_STORMWOLF) {
        b->sk_elem[SIG_SKILL_SHRED] = EL_LIGHT + 1;
        b->sk_cost[SIG_SKILL_SHRED] = -100;
    }
}

static void add_skill_mod(BuildRT *b, int kind, int sk, double v)
{
    sk %= CLASS_SKILLS;
    switch (kind) {
    case MOD_SK_COUNT:  b->sk_count[sk] += (int)v; break;
    case MOD_SK_RADIUS: b->sk_radius[sk] += v; break;
    case MOD_SK_CD:     b->sk_cd[sk] += v; break;
    case MOD_SK_COST:   b->sk_cost[sk] += v; break;
    case MOD_SK_DUR:    b->sk_dur[sk] += (int)v; break;
    case MOD_SK_FLAGS:  b->sk_flags[sk] |= (uint16_t)v; break;
    case MOD_SK_STATUS: b->sk_status[sk] = (uint8_t)v; break;
    case MOD_SK_ELEM:   b->sk_elem[sk] = (uint8_t)(v + 1); break;
    case MOD_SK_RANKS:  b->ranks[sk] += (int)v; break;
    default: break;
    }
}

static void add_multiplier(BuildRT *b, int kind, int arg, double v)
{
    switch (kind) {
    case MOD_X_ALL:    mult(&b->x_all, v); break;
    case MOD_X_ELEM:   mult(&b->x_elem[arg % EL_COUNT], v); break;
    case MOD_X_VULN:   mult(&b->x_vuln, v); break;
    case MOD_X_CRIT:   mult(&b->x_crit, v); break;
    case MOD_X_OP:     mult(&b->x_op, v); break;
    case MOD_X_CC:     mult(&b->x_cc, v); break;
    case MOD_X_DOT:    mult(&b->x_dot, v); break;
    case MOD_X_CORE:   mult(&b->x_core, v); break;
    case MOD_X_SKILL:  mult(&b->x_skill[arg % CLASS_SKILLS], v); break;
    case MOD_X_TAG:    mult(&b->x_tag[arg % TAG_COUNT], v); break;
    case MOD_X_MINION: mult(&b->x_minion, v); break;
    default: break;
    }
}

static void add_offence(BuildRT *b, int kind, int arg, double v)
{
    switch (kind) {
    case MOD_ADD_DMG:   b->add[ADD_ALL] += v; break;
    case MOD_ADD_ELEM:  b->add[ADD_ELEM0 + arg % EL_COUNT] += v; break;
    case MOD_ADD_CLOSE: b->add[ADD_CLOSE] += v; break;
    case MOD_ADD_FAR:   b->add[ADD_FAR] += v; break;
    case MOD_ADD_CC:    b->add[ADD_CC] += v; break;
    case MOD_ADD_ELITE: b->add[ADD_ELITE] += v; break;
    case MOD_ADD_DOT:   b->add[ADD_DOT] += v; break;
    case MOD_ADD_CORE:  b->add[ADD_CORE] += v; break;
    case MOD_ADD_BASIC: b->add[ADD_BASIC] += v; break;
    case MOD_ADD_TAG:   b->add[ADD_TAG0 + arg % TAG_COUNT] += v; break;
    case MOD_MAINSTAT:  b->mainstat_flat += v; break;
    case MOD_STAT:      b->stat[arg % STAT_COUNT] += v; break;
    case MOD_CRIT:      b->crit += v; break;
    case MOD_CRIT_DMG:  b->crit_dmg += v; break;
    case MOD_VULN_DMG:  b->vuln_dmg += v; break;
    case MOD_OP_CHANCE: b->op_chance += v; break;
    case MOD_OP_DMG:    b->op_dmg += v; break;
    case MOD_ATK_SPD:   b->atk_spd += v; break;
    case MOD_LUCKY:     b->lucky += v; break;
    case MOD_LH_VULN:   b->lh_vuln += v; break;
    case MOD_LH_STUN:   b->lh_stun += v; break;
    case MOD_LH_RES:    b->lh_res += v; break;
    default: add_multiplier(b, kind, arg, v); break;
    }
}

static void add_defence(BuildRT *b, int kind, int arg, double v)
{
    switch (kind) {
    case MOD_LIFE:        b->life += v; break;
    case MOD_LIFE_PCT:    b->life_pct += v; break;
    case MOD_ARMOR:       b->armor += v; break;
    case MOD_ARMOR_PCT:   b->armor_pct += v; break;
    case MOD_DR:          b->dr = 100.0 - (100.0 - b->dr) * (1.0 - v / 100.0); break; /* stacks multiplicatively */
    case MOD_DR_CLOSE:    b->dr_close += v; break;
    case MOD_RES_ALL:     b->res_all += v; break;
    case MOD_RES_ELEM:    b->res[arg % EL_COUNT] += v; break;
    case MOD_LIFE_HIT:    b->life_hit += v; break;
    case MOD_LIFE_KILL:   b->life_kill += v; break;
    case MOD_HEAL_KILL:   b->heal_kill += v; break;
    case MOD_REGEN:       b->regen += v; break;
    case MOD_BARRIER_GEN: b->barrier_gen += v; break;
    case MOD_THORNS:      b->thorns += v; break;
    case MOD_THORNS_PCT:  b->thorns_pct += v; break;
    case MOD_BARRIER_CD:  b->barrier_cd += v; break;
    case MOD_CDR:         b->cdr += v; break;
    case MOD_COST:        b->cost += v; break;
    case MOD_RES_GEN:     b->res_gen += v; break;
    case MOD_MAX_RES:     b->max_res += v; break;
    case MOD_RES_KILL:    b->res_kill += v; break;
    case MOD_MOVE:        b->move += v; break;
    case MOD_CC_DUR:      b->cc_dur += v; break;
    case MOD_GOLD:        b->gold += v; break;
    case MOD_XP:          b->xp += v; break;
    case MOD_MINION_COUNT: b->minion_add += (int)v; break;
    case MOD_SIGNATURE:    signature_mod(b, arg, v); break;
    case MOD_SIG_FACET:    if (arg < SF_COUNT) b->facet[arg] = MAX(b->facet[arg], v); break;
    case MOD_ALL_RANKS: {
        int i;
        for (i = 0; i < CLASS_SKILLS; i++)
            b->ranks[i] += (int)v;
        break;
    }
    default: add_skill_mod(b, kind, arg, v); break;
    }
}

void build_add_mod(BuildRT *b, int kind, int arg, double value)
{
    if (kind <= MOD_NONE || kind >= MOD_COUNT)
        return;
    if (kind < MOD_LIFE)
        add_offence(b, kind, arg, value);
    else
        add_defence(b, kind, arg, value);
}

/* ------------------------------------------------------------ resolution */

static void apply_rune(SkillRT *s, const RuneDef *r)
{
    s->count += r->d_count;
    s->radius = s->radius * (100 + r->d_radius) / 100;
    s->coef *= 1.0 + r->d_coef / 100.0;
    s->cd_ticks = s->cd_ticks * (100 + r->d_cd) / 100;
    s->duration += r->d_dur;
    if (r->status)
        s->status = (uint8_t)r->status;
    if (r->element >= 0)
        s->element = (uint8_t)r->element;
    s->flags |= r->flags;
    s->cost *= 1.0 + r->d_cost / 100.0;
    s->crit_add += r->d_crit / 100.0;
    s->op_add += r->d_op / 100.0;
}

static void apply_gear_reshape(const BuildRT *b, int i, SkillRT *s)
{
    s->count += b->sk_count[i];
    s->radius = (int)(s->radius * (1.0 + b->sk_radius[i] / 100.0));
    s->cd_ticks = (int)(s->cd_ticks * (1.0 + b->sk_cd[i] / 100.0));
    s->duration += b->sk_dur[i];
    s->flags |= b->sk_flags[i];
    if (b->sk_status[i])
        s->status = b->sk_status[i];
    if (b->sk_elem[i])
        s->element = (uint8_t)(b->sk_elem[i] - 1);
    if (s->cost > 0)
        s->cost *= 1.0 + b->sk_cost[i] / 100.0;
}

/* Key passives that reshape skills (the rest act in combat). */
static void apply_key_reshape(const BuildRT *b, SkillRT *s)
{
    if (has_key(b, KP_HAIL) && s->behavior == SB_PROJ)
        s->count += 2;
    if (has_key(b, KP_STORMCALLER) && s->tag == TAG_STORM && (s->behavior == SB_CHAIN || s->behavior == SB_PROJ))
        s->count += 1;
    if (has_key(b, KP_EAGLE) && s->tag == TAG_EAGLE)
        s->flags |= RF_VULN;
    if (has_key(b, KP_MARROW) && s->tag == TAG_BONE)
        s->crit_add += 0.10;
    if (has_key(b, KP_EARTHSPIRIT) && s->tag == TAG_EARTH)
        s->op_add += 0.15;
}

static void resolve_skill(const Profile *p, const BuildRT *b, int i, SkillRT *s)
{
    const SkillDef *d = skill_def(p->cls, i);
    int rank = p->skill_rank[i] > 0 ? p->skill_rank[i] + b->ranks[i] : 0;
    memset(s, 0, sizeof *s);
    s->usable = p->skill_rank[i] > 0 && skill_on_bar(p, i);
    s->rank = rank;
    s->cat = d->cat;
    s->behavior = d->behavior;
    s->element = d->element;
    s->status = d->status;
    s->buff = d->buff;
    s->tag = d->tag;
    s->flags = d->flags;
    /* Each rank past the first: +10% damage, or +5% strength for buffs. */
    s->coef = d->coef * (1.0 + (d->behavior == SB_BUFF ? 0.05 : 0.10) * (rank > 0 ? rank - 1 : 0));
    s->cost = d->cost;
    s->cd_ticks = (int)(d->cooldown * TICK_HZ);
    s->radius = d->radius;
    s->range = d->range;
    s->count = d->count;
    s->duration = d->duration;
    s->lucky = d->lucky / 100.0;
    s->color = d->color;
    s->vfx = VX_CLASS(p->cls % CLASS_COUNT, i);
    if (p->skill_enh[i])
        apply_rune(s, &d->enh);
    if (p->skill_enh[i] && p->skill_upg[i] >= 1 && p->skill_upg[i] <= 2)
        apply_rune(s, &d->upg[p->skill_upg[i] - 1]);
    apply_gear_reshape(b, i, s);
    apply_key_reshape(b, s);
    if (s->behavior == SB_SUMMON)
        s->count += b->minion_add + (has_key(b, KP_COMMANDER) ? 2 : 0);
    s->cd_ticks = MAX(d->cooldown > 0 ? TICK_HZ / 2 : 0, (int)(s->cd_ticks * (1.0 - MIN(b->cdr, 60.0) / 100.0)));
    if (s->cost > 0)
        s->cost *= 1.0 - MIN(b->cost, 50.0) / 100.0;
    else
        s->cost *= 1.0 + b->res_gen / 100.0;
    s->count = MAX(1, s->count);
}

static void apply_passives(const Profile *p, BuildRT *b)
{
    int i, cls = p->cls % CLASS_COUNT;
    for (i = 0; i < CLASS_PASSIVES; i++)
        if (p->passive[i])
            build_add_mod(b, passive_defs[cls][i].mod, passive_defs[cls][i].arg,
                          passive_defs[cls][i].per_rank * p->passive[i]);
    b->key = p->key_passive >= 1 && p->key_passive <= CLASS_KEYS
           ? (uint8_t)(KP_BERSERKER + cls * CLASS_KEYS + p->key_passive - 1) : KP_NONE;
    /* Flat key passive numbers live here; conditional ones are in combat. */
    switch (b->key) {
    case KP_TITAN:      b->op_chance += 10; mult(&b->x_op, 40); break;
    case KP_DEADEYE:    mult(&b->x_crit, 20); break;
    case KP_MARROW:     mult(&b->x_tag[TAG_BONE], 30); break;
    case KP_COMMANDER:  mult(&b->x_minion, 60); break;
    case KP_EARTHSPIRIT: mult(&b->x_op, 30); break;
    case KP_STORMCALLER: mult(&b->x_tag[TAG_STORM], 25); break;
    case KP_PRIMAL:     mult(&b->x_tag[TAG_BEAST], 30); b->atk_spd += 15; break;
    case KP_EAGLE:      mult(&b->x_tag[TAG_EAGLE], 25); break;
    default: break;
    }
}

void resolve_build(const Profile *p, BuildRT *b)
{
    int s;
    apply_signature(p, b);
    apply_passives(p, b);
    for (s = 0; s < CLASS_SKILLS; s++)
        resolve_skill(p, b, s, &b->skill[s]);
}

/* ------------------------------------------------------- auto planner */

static const BuildPreset *preset_of(const Profile *p)
{
    return &class_defs[p->cls % CLASS_COUNT].preset[p->preset % PRESETS];
}

/* One point along the preset's plan; false when nothing more can be bought. */
static bool plan_step(Profile *p, const BuildPreset *b)
{
    int i;
    for (i = 0; i < BAR_SLOTS; i++)            /* every bar skill at rank 1, then its enhancement */
        if ((p->skill_rank[b->bar[i]] == 0 && skill_rank_up(p, b->bar[i])) || skill_enhance(p, b->bar[i]))
            return true;
    for (i = 0; i < BAR_SLOTS; i++)
        if (b->upg[i] && !p->skill_upg[b->bar[i]] && skill_set_upgrade(p, b->bar[i], b->upg[i]))
            return true;
    if (!p->key_passive && key_passive_set(p, b->key))
        return true;
    if (skill_rank_up(p, b->bar[1]))           /* the core skill carries the build */
        return true;
    for (i = 0; i < 2; i++)
        if (passive_rank_up(p, b->passives[i]))
            return true;
    for (i = 0; i < BAR_SLOTS; i++)
        if (skill_rank_up(p, b->bar[i]))
            return true;
    for (i = 0; i < CLASS_PASSIVES; i++)
        if (b->passives[i] < CLASS_PASSIVES && passive_rank_up(p, b->passives[i]))
            return true;
    return false;
}

void build_auto_spend(Profile *p)
{
    const BuildPreset *b = preset_of(p);
    int i, guard = 0;
    while (p->skill_points > 0 && guard++ < 400 && plan_step(p, b))
        ;
    for (i = 0; i < BAR_SLOTS; i++)            /* the bar follows the preset's order */
        if (p->skill_rank[b->bar[i]] > 0)
            skill_bar_set(p, i, b->bar[i]);
}

void build_apply_preset(Profile *p, int preset)
{
    skill_refund_all(p);
    p->preset = (uint8_t)(preset % PRESETS);
    build_auto_spend(p);
}
