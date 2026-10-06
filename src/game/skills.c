#include "skills.h"
#include "build.h"
#include "../data/icons.h"
#include "../gfx/gfx.h"
#include <string.h>

#define C_PHYS  RGB565(230, 230, 230)
#define C_FIRE  RGB565(255, 130, 40)
#define C_COLD  RGB565(120, 200, 255)
#define C_LIGHT RGB565(200, 190, 255)
#define C_POIS  RGB565(130, 230, 80)
#define C_SHAD  RGB565(180, 100, 240)
#define C_GOLD  RGB565(255, 220, 120)
#define C_BLOOD RGB565(230, 50, 60)
#define C_EARTH RGB565(200, 150, 90)
#define C_BONE  RGB565(235, 228, 205)

/* Enhancement / upgrade shorthands. Field order of RuneDef:
 * name, desc, +count, +radius%, +dmg%, +cd%, +ticks, status, element, flags, +cost%, +crit%, +overpower% */
#define U(n, d, cnt, rad, coef, cd, dur, st, el, fl, cost, crit, op) \
    { n, d, cnt, rad, coef, cd, dur, st, el, fl, cost, crit, op }
#define UF(n, d, fl)     U(n, d, 0, 0, 0, 0, 0, 0, -1, fl, 0, 0, 0)
#define UC(n, d, v)      U(n, d, 0, 0, v, 0, 0, 0, -1, 0, 0, 0, 0)
#define UN(n, d, v)      U(n, d, v, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0)
#define UR(n, d, v)      U(n, d, 0, v, 0, 0, 0, 0, -1, 0, 0, 0, 0)
#define UD(n, d, v)      U(n, d, 0, 0, 0, 0, v, 0, -1, 0, 0, 0, 0)
#define UE(n, d, st, el) U(n, d, 0, 0, 0, 0, 0, st, el, 0, 0, 0, 0)
#define UK(n, d, v)      U(n, d, 0, 0, 0, v, 0, 0, -1, 0, 0, 0, 0)
#define UX(n, d, v)      U(n, d, 0, 0, 0, 0, 0, 0, -1, 0, 0, v, 0)
#define UO(n, d, v)      U(n, d, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, v)
#define UG(n, d, v)      U(n, d, 0, 0, 0, 0, 0, 0, -1, 0, v, 0, 0)

/* SkillDef field order: name, desc, cluster, behaviour, element, status, buff, tag, icon,
 * damage (x weapon), cost (<0 generates), cooldown s, radius, range, count, duration ticks,
 * lucky hit %, base flags, colour, enhancement, { upgrade A, upgrade B }.
 * For buffs the damage field is the buff strength in %. */
const SkillDef skill_defs[CLASS_COUNT][CLASS_SKILLS] = {
    [CLASS_BARBARIAN] = {
        { "FLAY", "SLASH AND MAKE FOES BLEED. +10 FURY", CAT_BASIC, SB_ARC, EL_PHYS, ST_BLEED, 0, TAG_NONE, IC_SWORD,
          0.45, -10, 0, 20, 0, 1, 0, 50, 0, C_PHYS,
          UF("ENHANCED FLAY", "HITS MAKE ENEMIES VULNERABLE", RF_VULN),
          { UF("COMBAT FLAY", "HITS GRANT BERSERKING", RF_BERSERK), UO("BRUTAL FLAY", "+15% OVERPOWER CHANCE", 15) } },
        { "FRENZY", "FAST BLOWS, +20% ATTACK SPEED. +6 FURY", CAT_BASIC, SB_ARC, EL_PHYS, ST_NONE, 0, TAG_NONE, IC_AXE,
          0.35, -6, 0, 18, 0, 1, 0, 30, RF_HASTE, C_PHYS,
          UX("ENHANCED FRENZY", "+20% CRIT CHANCE", 20),
          { UG("FURIOUS FRENZY", "+60% FURY GENERATED", 60), UF("BATTLE FRENZY", "HITS MAKE VULNERABLE", RF_VULN) } },
        { "WHIRLWIND", "SPIN, SHREDDING ALL AROUND. 6 FURY/PULSE", CAT_CORE, SB_CHANNEL, EL_PHYS, ST_NONE, 0, TAG_NONE,
          IC_WHIRL, 0.8, 5, 0, 30, 0, 1, 90, 20, 0, C_GOLD,
          UR("ENHANCED WHIRLWIND", "+25% RADIUS", 25),
          { UE("BLOOD WHIRL", "CUTS CAUSE BLEEDING", ST_BLEED, -1), UC("VIOLENT WHIRL", "+40% DAMAGE", 40) } },
        { "GROUND SLAM", "HEAVY BLOW THAT STUNS. 30 FURY", CAT_CORE, SB_ARC, EL_PHYS, ST_STUN, 0, TAG_EARTH, IC_HAMMER,
          1.5, 30, 0, 28, 0, 1, 15, 40, 0, C_EARTH,
          UO("ENHANCED SLAM", "+20% OVERPOWER CHANCE", 20),
          { UF("SEISMIC SLAM", "LEAVES AN EARTHQUAKE", RF_GROUND), U("CRUSHING SLAM", "MAKES VULNERABLE, +20% DMG",
            0, 0, 20, 0, 0, 0, -1, RF_VULN, 0, 0, 0) } },
        { "REND", "DEEP CUTS: HEAVY BLEED FOR 5S. 25 FURY", CAT_CORE, SB_ARC, EL_PHYS, ST_BLEED, 0, TAG_NONE, IC_CLAWS,
          0.8, 25, 0, 26, 0, 1, 150, 33, 0, C_BLOOD,
          UF("ENHANCED REND", "HITS MAKE VULNERABLE", RF_VULN),
          { UC("DEEP WOUNDS", "+50% DAMAGE", 50), UR("SPREADING REND", "+50% RADIUS", 50) } },
        { "WAR CRY", "BERSERKING: [x]25% DAMAGE FOR 6S", CAT_DEFENSIVE, SB_BUFF, EL_PHYS, ST_NONE, BUFF_BERSERK,
          TAG_SHOUT, IC_SHOUT, 25, 0, 18, 0, 0, 1, 180, 0, 0, C_GOLD,
          UF("ENHANCED WAR CRY", "ALSO GRANTS UNSTOPPABLE", RF_UNSTOP),
          { UC("MIGHTY CRY", "+40% STRENGTH", 40), U("RALLYING CRY", "HEALS 15%, +2S", 0, 0, 0, 0, 60, 0, -1, RF_HEAL,
            0, 0, 0) } },
        { "IRON SKIN", "BARRIER OF 40% LIFE FOR 5S", CAT_DEFENSIVE, SB_BUFF, EL_PHYS, ST_NONE, BUFF_BARRIER, TAG_NONE,
          IC_SHIELD, 40, 0, 16, 0, 0, 1, 150, 0, 0, RGB565(170, 180, 200),
          UC("ENHANCED IRON SKIN", "+25% BARRIER", 25),
          { UF("STRENGTHENED SKIN", "ALSO HEALS 15%", RF_HEAL), UF("UNYIELDING SKIN", "GRANTS UNSTOPPABLE", RF_UNSTOP) } },
        { "LEAP SLAM", "CRASH DOWN ON A PACK AND STUN", CAT_MASTERY, SB_STRIKE, EL_PHYS, ST_STUN, 0, TAG_EARTH, IC_LEAP,
          2.6, 0, 9, 36, 140, 1, 20, 50, 0, C_GOLD,
          UN("ENHANCED LEAP", "+1 AFTERSHOCK", 1),
          { U("MAGMA LEAP", "FIRE DAMAGE, BURNING GROUND", 0, 0, 0, 0, 0, ST_BURN, EL_FIRE, RF_GROUND, 0, 0, 0),
            U("SHATTERING LEAP", "MAKES VULNERABLE, +30% SIZE", 0, 30, 0, 0, 0, 0, -1, RF_VULN, 0, 0, 0) } },
        { "EARTHQUAKE", "THE GROUND SHATTERS AROUND YOU", CAT_MASTERY, SB_GROUND, EL_PHYS, ST_STUN, 0, TAG_EARTH,
          IC_QUAKE, 0.5, 0, 12, 52, 0, 1, 120, 25, 0, C_EARTH,
          UD("ENHANCED QUAKE", "+2 SECONDS", 60),
          { UE("MOLTEN QUAKE", "FIRE DAMAGE THAT BURNS", ST_BURN, EL_FIRE),
            U("RUPTURE", "MAKES VULNERABLE, +30% DMG", 0, 0, 30, 0, 0, 0, -1, RF_VULN, 0, 0, 0) } },
        { "BLOOD FRENZY", "ULTIMATE: [x]40% DMG, SPEED, UNSTOPPABLE", CAT_ULTIMATE, SB_BUFF, EL_PHYS, ST_NONE, BUFF_ULT,
          TAG_SHOUT, IC_RAGE, 40, 0, 45, 0, 0, 1, 240, 0, 0, C_BLOOD,
          UD("ENHANCED FRENZY", "+3 SECONDS", 90),
          { UC("SAVAGE FRENZY", "+50% STRENGTH", 50), UF("UNDYING FRENZY", "ALSO HEALS 15%", RF_HEAL) } },
    },
    [CLASS_SORCERER] = {
        { "SPARK", "LIGHTNING THAT ARCS TO A FOE. +8 MANA", CAT_BASIC, SB_CHAIN, EL_LIGHT, ST_NONE, 0, TAG_STORM,
          IC_SPARK, 0.4, -8, 0, 40, 110, 1, 0, 25, 0, C_LIGHT,
          UN("ENHANCED SPARK", "+1 CHAIN", 1),
          { UX("FLICKERING SPARK", "+15% CRIT CHANCE", 15), UF("SURGING SPARK", "HITS MAKE VULNERABLE", RF_VULN) } },
        { "FROST BOLT", "A BOLT THAT CHILLS. +8 MANA", CAT_BASIC, SB_PROJ, EL_COLD, ST_CHILL, 0, TAG_NONE, IC_SPEAR,
          0.4, -8, 0, 0, 130, 1, 60, 30, 0, C_COLD,
          UF("ENHANCED FROST BOLT", "PIERCES", RF_PIERCE),
          { UC("GLINTING BOLT", "+30% DAMAGE", 30), UF("SHATTERING BOLT", "HITS MAKE VULNERABLE", RF_VULN) } },
        { "FIREBALL", "EXPLODING BOLT OF FLAME. 40 MANA", CAT_CORE, SB_PROJ, EL_FIRE, ST_BURN, 0, TAG_NONE, IC_FIREBALL,
          1.4, 40, 0, 22, 140, 1, 0, 33, 0, C_FIRE,
          UR("ENHANCED FIREBALL", "+30% RADIUS", 30),
          { U("DESTRUCTIVE FIREBALL", "+25% DMG, MAKES VULNERABLE", 0, 0, 25, 0, 0, 0, -1, RF_VULN, 0, 0, 0),
            U("SPLIT FIREBALL", "FIRES 3 BOLTS", 2, 0, -25, 0, 0, 0, -1, 0, 0, 0, 0) } },
        { "CHAIN LIGHTNING", "ARCS BETWEEN FOES. 35 MANA", CAT_CORE, SB_CHAIN, EL_LIGHT, ST_NONE, 0, TAG_STORM, IC_CHAIN,
          1.2, 35, 0, 60, 120, 4, 0, 25, 0, C_LIGHT,
          UN("ENHANCED CHAIN", "+2 CHAINS", 2),
          { U("STATIC CHAIN", "STUNS EVERY TARGET", 0, 0, 0, 0, 15, ST_STUN, -1, 0, 0, 0, 0),
            UX("CHARGED CHAIN", "+15% CRIT CHANCE", 15) } },
        { "ICE SHARDS", "A FAN OF CHILLING SHARDS. 30 MANA", CAT_CORE, SB_PROJ, EL_COLD, ST_CHILL, 0, TAG_NONE, IC_SHARDS,
          0.55, 30, 0, 0, 130, 5, 60, 20, 0, C_COLD,
          UF("ENHANCED SHARDS", "SHARDS PIERCE", RF_PIERCE),
          { UE("SHATTERING SHARDS", "SHARDS FREEZE", ST_FREEZE, -1), UN("VOLLEY OF SHARDS", "+2 SHARDS", 2) } },
        { "FROST NOVA", "BURST THAT FREEZES ALL AROUND", CAT_DEFENSIVE, SB_NOVA, EL_COLD, ST_FREEZE, 0, TAG_NONE, IC_SNOW,
          0.6, 0, 10, 48, 0, 1, 45, 50, 0, C_COLD,
          UF("ENHANCED NOVA", "MAKES VULNERABLE", RF_VULN),
          { UD("DEEP FREEZE", "+1 SECOND FREEZE", 30), UR("WIDE NOVA", "+50% RADIUS", 50) } },
        { "ICE ARMOR", "BARRIER OF 35% LIFE FOR 6S", CAT_DEFENSIVE, SB_BUFF, EL_COLD, ST_NONE, BUFF_BARRIER, TAG_NONE,
          IC_SHIELD, 35, 0, 18, 0, 0, 1, 180, 0, 0, C_COLD,
          UC("ENHANCED ARMOR", "+25% BARRIER", 25),
          { UF("MYSTICAL ARMOR", "ALSO HEALS 15%", RF_HEAL), UF("FROZEN ARMOR", "GRANTS UNSTOPPABLE", RF_UNSTOP) } },
        { "METEOR", "CRUSH THE DENSEST PACK", CAT_MASTERY, SB_STRIKE, EL_FIRE, ST_BURN, 0, TAG_NONE, IC_METEOR,
          3.0, 0, 10, 40, 160, 1, 18, 40, 0, C_FIRE,
          UF("ENHANCED METEOR", "LEAVES BURNING GROUND", RF_GROUND),
          { UE("COMET", "COLD DAMAGE THAT FREEZES", ST_FREEZE, EL_COLD), UC("MOLTEN METEOR", "+40% DAMAGE", 40) } },
        { "THUNDERSTORM", "LIGHTNING RAINS AROUND YOU", CAT_MASTERY, SB_GROUND, EL_LIGHT, ST_NONE, 0, TAG_STORM, IC_STORM,
          0.65, 0, 13, 60, 0, 1, 150, 30, 0, C_LIGHT,
          UD("ENHANCED STORM", "+2 SECONDS", 60),
          { UF("EYE OF THE STORM", "FOLLOWS YOU", RF_FOLLOW), U("STATIC STORM", "STUNS", 0, 0, 0, 0, 0, ST_STUN, -1, 0,
            0, 0, 0) } },
        { "ELEMENTAL TEMPEST", "ULTIMATE: A HUGE STORM AROUND YOU", CAT_ULTIMATE, SB_GROUND, EL_LIGHT, ST_NONE, 0,
          TAG_STORM, IC_TORNADO, 1.1, 0, 40, 64, 0, 1, 210, 25, RF_FOLLOW, C_SHAD,
          UR("ENHANCED TEMPEST", "+30% RADIUS", 30),
          { UE("INFERNO", "BECOMES FIRE THAT BURNS", ST_BURN, EL_FIRE),
            UE("BLIZZARD", "BECOMES COLD THAT CHILLS", ST_CHILL, EL_COLD) } },
    },
    [CLASS_ROGUE] = {
        { "PUNCTURE", "THROW 3 BLADES. +8 ENERGY", CAT_BASIC, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_NONE, IC_DAGGER,
          0.3, -8, 0, 0, 120, 3, 0, 33, 0, C_PHYS,
          UF("ENHANCED PUNCTURE", "HITS MAKE VULNERABLE", RF_VULN),
          { UG("PRIMARY PUNCTURE", "+50% ENERGY GENERATED", 50), UN("FUNDAMENTAL PUNCTURE", "+2 BLADES", 2) } },
        { "HEARTSEEKER", "AIMED ARROW. +6 ENERGY", CAT_BASIC, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_NONE, IC_EYE,
          0.55, -6, 0, 0, 140, 1, 0, 40, 0, C_PHYS,
          UX("ENHANCED HEARTSEEKER", "+15% CRIT CHANCE", 15),
          { UF("ACCELERATING SEEKER", "GRANTS +20% ATTACK SPEED", RF_HASTE), UF("BLADED SEEKER", "PIERCES", RF_PIERCE) } },
        { "PENETRATING SHOT", "ARROW THAT PIERCES ALL. 35 ENERGY", CAT_CORE, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_NONE,
          IC_ARROW, 1.5, 35, 0, 0, 160, 1, 0, 50, RF_PIERCE, C_PHYS,
          U("ENHANCED SHOT", "+20% DMG, MAKES VULNERABLE", 0, 0, 20, 0, 0, 0, -1, RF_VULN, 0, 0, 0),
          { UX("ADVANCED SHOT", "+20% CRIT CHANCE", 20), UC("IMPROVED SHOT", "+40% DAMAGE", 40) } },
        { "RAPID FIRE", "5 ARROWS AT ONE TARGET. 25 ENERGY", CAT_CORE, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_NONE, IC_BOW,
          0.5, 25, 0, 0, 140, 5, 0, 20, RF_BURST, C_PHYS,
          UN("ENHANCED RAPID FIRE", "+2 ARROWS", 2),
          { UX("PRIMARY RAPID FIRE", "+15% CRIT CHANCE", 15), UF("ADVANCED RAPID FIRE", "MAKES VULNERABLE", RF_VULN) } },
        { "BARRAGE", "A WIDE VOLLEY OF ARROWS. 30 ENERGY", CAT_CORE, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_NONE, IC_ARROWS,
          0.55, 30, 0, 0, 140, 6, 0, 17, 0, C_PHYS,
          UN("ENHANCED BARRAGE", "+2 ARROWS", 2),
          { UF("IMPROVED BARRAGE", "ARROWS PIERCE", RF_PIERCE), UC("ADVANCED BARRAGE", "+30% DAMAGE", 30) } },
        { "SPIKE TRAP", "TRAP THAT SHREDS AND SLOWS", CAT_DEFENSIVE, SB_GROUND, EL_PHYS, ST_CHILL, 0, TAG_NONE, IC_TRAP,
          0.8, 0, 9, 28, 120, 1, 180, 30, 0, C_PHYS,
          UF("ENHANCED TRAP", "MAKES VULNERABLE", RF_VULN),
          { UE("FIRE TRAP", "BURNS INSTEAD", ST_BURN, EL_FIRE), UE("VENOM TRAP", "POISONS INSTEAD", ST_POISON, EL_POISON) } },
        { "SHADOW DASH", "DASH THROUGH A PACK, CUTTING", CAT_DEFENSIVE, SB_DASH, EL_SHADOW, ST_NONE, 0, TAG_NONE, IC_DASH,
          1.2, 0, 8, 18, 90, 1, 0, 50, 0, C_SHAD,
          UF("ENHANCED DASH", "GRANTS UNSTOPPABLE", RF_UNSTOP),
          { UK("DISCIPLINED DASH", "-30% COOLDOWN", -30), U("ADVANCED DASH", "+40% DMG, MAKES VULNERABLE", 0, 0, 40,
            0, 0, 0, -1, RF_VULN, 0, 0, 0) } },
        { "POISON IMBUEMENT", "HITS POISON AND DEAL [x]40% FOR 5S", CAT_MASTERY, SB_BUFF, EL_POISON, ST_POISON,
          BUFF_IMBUE, TAG_IMBUE, IC_VIAL, 40, 0, 13, 0, 0, 1, 150, 0, 0, C_POIS,
          UD("ENHANCED IMBUEMENT", "+2 SECONDS", 60),
          { UC("BLENDED IMBUEMENT", "+40% STRENGTH", 40), UF("MIXED IMBUEMENT", "HITS MAKE VULNERABLE", RF_VULN) } },
        { "COLD IMBUEMENT", "HITS CHILL AND DEAL [x]40% FOR 5S", CAT_MASTERY, SB_BUFF, EL_COLD, ST_CHILL, BUFF_IMBUE,
          TAG_IMBUE, IC_VIAL, 40, 0, 13, 0, 0, 1, 150, 0, 0, C_COLD,
          UD("ENHANCED IMBUEMENT", "+2 SECONDS", 60),
          { UE("MIXED IMBUEMENT", "HITS FREEZE", ST_FREEZE, -1), UF("BLENDED IMBUEMENT", "HITS MAKE VULNERABLE",
            RF_VULN) } },
        { "RAIN OF ARROWS", "ULTIMATE: ARROWS FALL ON A WIDE AREA", CAT_ULTIMATE, SB_GROUND, EL_PHYS, ST_NONE, 0,
          TAG_NONE, IC_RAIN, 1.0, 0, 40, 64, 160, 1, 150, 20, 0, C_PHYS,
          UD("ENHANCED RAIN", "+2 SECONDS", 60),
          { UE("FIRE ARROWS", "FIRE DAMAGE THAT BURNS", ST_BURN, EL_FIRE),
            U("HAIL OF ARROWS", "+30% DMG, MAKES VULNERABLE", 0, 0, 30, 0, 0, 0, -1, RF_VULN, 0, 0, 0) } },
    },
    [CLASS_NECRO] = {
        { "REAP", "SCYTHE SWEEP OF SHADOW. +10 ESSENCE", CAT_BASIC, SB_ARC, EL_SHADOW, ST_NONE, 0, TAG_NONE, IC_SCYTHE,
          0.45, -10, 0, 26, 0, 1, 0, 40, 0, C_SHAD,
          UF("ENHANCED REAP", "GRANTS A SMALL BARRIER", RF_BARRIER),
          { UC("ACOLYTE'S REAP", "+30% DAMAGE", 30), UF("INITIATE'S REAP", "HITS MAKE VULNERABLE", RF_VULN) } },
        { "BONE SPLINTERS", "3 BONE SHARDS. +8 ESSENCE", CAT_BASIC, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_BONE, IC_BONE,
          0.3, -8, 0, 0, 120, 3, 0, 33, 0, C_BONE,
          UN("ENHANCED SPLINTERS", "+1 SHARD", 1),
          { UG("ACOLYTE'S SPLINTERS", "+50% ESSENCE GENERATED", 50), UF("INITIATE'S SPLINTERS", "HITS MAKE VULNERABLE",
            RF_VULN) } },
        { "BONE SPEAR", "A PIERCING LANCE OF BONE. 25 ESSENCE", CAT_CORE, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_BONE,
          IC_SPEAR, 1.5, 25, 0, 0, 160, 1, 0, 50, RF_PIERCE, C_BONE,
          UX("ENHANCED SPEAR", "+10% CRIT CHANCE", 10),
          { UF("SUPREME SPEAR", "SHATTERS AT THE END", RF_EXPLODE), UF("PAINFUL SPEAR", "MAKES VULNERABLE", RF_VULN) } },
        { "BLIGHT", "A POOL OF SHADOW THAT ROTS. 25 ESSENCE", CAT_CORE, SB_GROUND, EL_SHADOW, ST_SHADOW, 0, TAG_NONE,
          IC_CLOUD, 0.5, 25, 0, 30, 130, 1, 120, 20, 0, C_SHAD,
          UR("ENHANCED BLIGHT", "+30% RADIUS", 30),
          { UC("SUPERNATURAL BLIGHT", "+40% DAMAGE", 40), UE("PARANORMAL BLIGHT", "SLOWS (CHILL)", ST_CHILL, -1) } },
        { "BLOOD SURGE", "DRAIN BLOOD IN A NOVA. 30 ESSENCE", CAT_CORE, SB_NOVA, EL_PHYS, ST_NONE, 0, TAG_BLOOD, IC_BLOOD,
          1.1, 30, 0, 44, 0, 1, 0, 25, 0, C_BLOOD,
          UR("ENHANCED SURGE", "+25% RADIUS", 25),
          { UF("PARANORMAL SURGE", "MAKES VULNERABLE", RF_VULN), UC("SUPERNATURAL SURGE", "+40% DAMAGE", 40) } },
        { "BONE PRISON", "A RING OF BONE THAT TRAPS FOES", CAT_DEFENSIVE, SB_NOVA, EL_PHYS, ST_IMMOBILIZE, 0, TAG_BONE,
          IC_PRISON, 0.4, 0, 12, 40, 0, 1, 60, 30, 0, C_BONE,
          UF("ENHANCED PRISON", "MAKES VULNERABLE", RF_VULN),
          { UD("GHASTLY PRISON", "+1 SECOND", 30), UE("DREADFUL PRISON", "STUNS INSTEAD", ST_STUN, -1) } },
        { "BLOOD MIST", "UNSTOPPABLE, 30% LESS DAMAGE FOR 3S", CAT_DEFENSIVE, SB_BUFF, EL_PHYS, ST_NONE, BUFF_UNSTOP,
          TAG_BLOOD, IC_SHIELD, 30, 0, 18, 0, 0, 1, 90, 0, 0, C_BLOOD,
          UF("ENHANCED MIST", "ALSO HEALS 15%", RF_HEAL),
          { UD("GHASTLY MIST", "+2 SECONDS", 60), UK("DREADFUL MIST", "-25% COOLDOWN", -25) } },
        { "RAISE DEAD", "4 SKELETON WARRIORS FIGHT FOR YOU", CAT_MASTERY, SB_SUMMON, EL_PHYS, ST_NONE, 0, TAG_MINION,
          IC_SKULL, 0.6, 0, 4, 0, 0, 4, 0, 0, 0, C_BONE,
          UN("ENHANCED DEAD", "+1 SKELETON", 1),
          { U("SKELETAL MAGES", "RANGED SHADOW CASTERS", 0, 0, 0, 0, 0, 0, EL_SHADOW, RF_ALT, 0, 0, 0),
            UC("REAPERS", "+50% MINION DAMAGE", 50) } },
        { "CORPSE EXPLOSION", "A CORPSE BURSTS AMONG FOES", CAT_MASTERY, SB_CORPSE, EL_PHYS, ST_NONE, 0, TAG_NONE,
          IC_BURST, 1.6, 0, 1.5, 30, 140, 1, 0, 40, 0, C_BONE,
          UR("ENHANCED EXPLOSION", "+25% RADIUS", 25),
          { UE("BLIGHTED EXPLOSION", "SHADOW THAT ROTS", ST_SHADOW, EL_SHADOW), UF("PLAGUED EXPLOSION",
            "MAKES VULNERABLE", RF_VULN) } },
        { "LEGION OF BONE", "ULTIMATE: BONE ERUPTS ALL AROUND YOU", CAT_ULTIMATE, SB_GROUND, EL_PHYS, ST_STUN, 0, TAG_BONE,
          IC_HAND, 1.2, 0, 45, 60, 0, 1, 210, 25, 0, C_BONE,
          UF("ENHANCED LEGION", "FOLLOWS YOU", RF_FOLLOW),
          { UC("PRIME LEGION", "+40% DAMAGE", 40), UF("SUPREME LEGION", "MAKES VULNERABLE", RF_VULN) } },
    },
    [CLASS_DRUID] = {
        { "STORM STRIKE", "LIGHTNING FIST THAT ARCS. +10 SPIRIT", CAT_BASIC, SB_CHAIN, EL_LIGHT, ST_NONE, 0, TAG_STORM,
          IC_CHAIN, 0.4, -10, 0, 30, 24, 1, 0, 30, 0, RGB565(120, 230, 255),
          UF("ENHANCED STRIKE", "HITS MAKE VULNERABLE", RF_VULN),
          { UC("FIERCE STRIKE", "+30% DAMAGE", 30), UN("WILD STRIKE", "+2 CHAINS", 2) } },
        { "MAUL", "WEREBEAR SWIPE. +12 SPIRIT", CAT_BASIC, SB_ARC, EL_PHYS, ST_NONE, 0, TAG_BEAST, IC_FIST,
          0.5, -12, 0, 26, 0, 1, 0, 40, 0, C_EARTH,
          UO("ENHANCED MAUL", "+15% OVERPOWER CHANCE", 15),
          { UF("FIERCE MAUL", "HITS MAKE VULNERABLE", RF_VULN), UR("WILD MAUL", "+40% RADIUS", 40) } },
        { "TORNADO", "A WANDERING TWISTER. 40 SPIRIT", CAT_CORE, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_STORM, IC_TORNADO,
          0.8, 40, 0, 0, 140, 1, 0, 20, RF_PIERCE | RF_WANDER, RGB565(190, 210, 200),
          UN("ENHANCED TORNADO", "+1 TORNADO", 1),
          { UC("RAGING TORNADO", "+40% DAMAGE", 40), UF("PRIMAL TORNADO", "MAKES VULNERABLE", RF_VULN) } },
        { "LANDSLIDE", "PILLARS OF EARTH CRUSH FOES. 30 SPIRIT", CAT_CORE, SB_STRIKE, EL_PHYS, ST_NONE, 0, TAG_EARTH,
          IC_ROCKS, 1.5, 30, 0, 26, 120, 1, 0, 30, 0, C_EARTH,
          UO("ENHANCED LANDSLIDE", "+15% OVERPOWER CHANCE", 15),
          { UN("RAGING LANDSLIDE", "+1 PILLAR", 1), U("PRIMAL LANDSLIDE", "STUNS", 0, 0, 0, 0, 20, ST_STUN, -1, 0,
            0, 0, 0) } },
        { "SHRED", "WEREWOLF COMBO THAT BLEEDS. 25 SPIRIT", CAT_CORE, SB_ARC, EL_PHYS, ST_BLEED, 0, TAG_BEAST, IC_CLAWS,
          1.2, 25, 0, 24, 0, 1, 0, 30, 0, C_PHYS,
          UF("ENHANCED SHRED", "GRANTS +20% ATTACK SPEED", RF_HASTE),
          { UC("RAGING SHRED", "+40% DAMAGE", 40), UE("PRIMAL SHRED", "POISON THAT LINGERS", ST_POISON, EL_POISON) } },
        { "EARTHEN BULWARK", "BARRIER OF 40% LIFE FOR 5S", CAT_DEFENSIVE, SB_BUFF, EL_PHYS, ST_NONE, BUFF_BARRIER,
          TAG_EARTH, IC_SHIELD, 40, 0, 16, 0, 0, 1, 150, 0, 0, C_EARTH,
          UF("ENHANCED BULWARK", "GRANTS UNSTOPPABLE", RF_UNSTOP),
          { UC("PRESERVING BULWARK", "+30% BARRIER", 30), UF("INNATE BULWARK", "ALSO HEALS 15%", RF_HEAL) } },
        { "CYCLONE ARMOR", "WIND BURST THAT STUNS, GRANTS BARRIER", CAT_DEFENSIVE, SB_NOVA, EL_PHYS, ST_STUN, 0,
          TAG_STORM, IC_WHIRL, 0.6, 0, 14, 44, 0, 1, 15, 30, RF_BARRIER, RGB565(160, 230, 170),
          UF("ENHANCED CYCLONE", "MAKES VULNERABLE", RF_VULN),
          { UF("PRESERVING CYCLONE", "GRANTS UNSTOPPABLE", RF_UNSTOP), UC("INNATE CYCLONE", "+60% DAMAGE", 60) } },
        { "WOLF PACK", "2 SPIRIT WOLVES HUNT WITH YOU", CAT_MASTERY, SB_SUMMON, EL_PHYS, ST_NONE, 0, TAG_MINION, IC_PAW,
          0.7, 0, 4, 0, 0, 2, 0, 0, 0, RGB565(200, 200, 210),
          UN("ENHANCED PACK", "+1 WOLF", 1),
          { UC("FERAL PACK", "+50% WOLF DAMAGE", 50), U("STORM WOLVES", "LIGHTNING BITES", 0, 0, 0, 0, 0, 0, EL_LIGHT,
            0, 0, 0, 0) } },
        { "BOULDER", "ROLL A CRUSHING BOULDER", CAT_MASTERY, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_EARTH, IC_BOULDER,
          1.6, 0, 8, 0, 140, 1, 0, 40, 0, C_EARTH,
          UN("ENHANCED BOULDER", "+1 BOULDER", 1),
          { UF("RAGING BOULDER", "PIERCES", RF_PIERCE), UF("PRIMAL BOULDER", "MAKES VULNERABLE", RF_VULN) } },
        { "TEMPEST FURY", "ULTIMATE: A STORM FOLLOWS YOU", CAT_ULTIMATE, SB_GROUND, EL_LIGHT, ST_NONE, 0, TAG_STORM,
          IC_STORM, 1.0, 0, 45, 72, 0, 1, 240, 25, RF_FOLLOW, RGB565(120, 230, 255),
          UD("ENHANCED TEMPEST", "+3 SECONDS", 90),
          { UC("PRIME TEMPEST", "+40% DAMAGE", 40), UF("SUPREME TEMPEST", "MAKES VULNERABLE", RF_VULN) } },
    },
    [CLASS_SPIRITBORN] = {
        { "THRUST", "GLAIVE THRUST. +10 VIGOR", CAT_BASIC, SB_ARC, EL_PHYS, ST_NONE, 0, TAG_JAGUAR, IC_SPEAR,
          0.45, -10, 0, 24, 0, 1, 0, 40, 0, C_GOLD,
          UX("ENHANCED THRUST", "+10% CRIT CHANCE", 10),
          { UF("PIERCING THRUST", "HITS MAKE VULNERABLE", RF_VULN), UG("RUSHING THRUST", "+40% VIGOR GENERATED", 40) } },
        { "WITHERING FIST", "POISONED PUNCH. +10 VIGOR", CAT_BASIC, SB_ARC, EL_POISON, ST_POISON, 0, TAG_CENTIPEDE,
          IC_FIST, 0.4, -10, 0, 22, 0, 1, 0, 40, 0, C_POIS,
          UC("ENHANCED FIST", "+30% DAMAGE", 30),
          { UF("ACIDIC FIST", "HITS MAKE VULNERABLE", RF_VULN), UR("SPREADING FIST", "+50% RADIUS", 50) } },
        { "QUILL VOLLEY", "A FAN OF SPIRIT QUILLS. 30 VIGOR", CAT_CORE, SB_PROJ, EL_PHYS, ST_NONE, 0, TAG_EAGLE,
          IC_FEATHER, 0.55, 30, 0, 0, 120, 6, 0, 17, 0, RGB565(220, 240, 255),
          UF("ENHANCED VOLLEY", "MAKES VULNERABLE", RF_VULN),
          { UN("RAGING VOLLEY", "+2 QUILLS", 2), UX("KEEN VOLLEY", "+15% CRIT CHANCE", 15) } },
        { "CRUSHING PALM", "A SPIRIT HAND SLAMS DOWN. 30 VIGOR", CAT_CORE, SB_STRIKE, EL_PHYS, ST_STUN, 0, TAG_NONE,
          IC_PALM, 1.6, 30, 0, 30, 110, 1, 20, 40, 0, RGB565(255, 190, 120),
          UO("ENHANCED PALM", "+20% OVERPOWER CHANCE", 20),
          { UN("DOUBLE PALM", "+1 SLAM", 1), UF("DREADED PALM", "MAKES VULNERABLE", RF_VULN) } },
        { "STINGER", "DASH AND STING WITH VENOM. 25 VIGOR", CAT_CORE, SB_DASH, EL_POISON, ST_POISON, 0, TAG_CENTIPEDE,
          IC_STINGER, 1.4, 25, 0, 16, 70, 1, 0, 30, 0, C_POIS,
          UF("ENHANCED STINGER", "MAKES VULNERABLE", RF_VULN),
          { UC("VENOMOUS STINGER", "+40% DAMAGE", 40), UF("TOXIC STINGER", "LEAVES A POISON POOL", RF_GROUND) } },
        { "CENTIPEDE SWARM", "A CRAWLING SWARM POISONS FOES", CAT_DEFENSIVE, SB_GROUND, EL_POISON, ST_POISON, 0,
          TAG_CENTIPEDE, IC_CENTIPEDE, 0.5, 0, 12, 40, 0, 1, 150, 25, 0, C_POIS,
          UF("ENHANCED SWARM", "FOLLOWS YOU", RF_FOLLOW),
          { UC("RAVENOUS SWARM", "+40% DAMAGE", 40), UE("BINDING SWARM", "SLOWS (CHILL)", ST_CHILL, -1) } },
        { "ARMORED HIDE", "BARRIER OF 40% LIFE FOR 5S", CAT_DEFENSIVE, SB_BUFF, EL_PHYS, ST_NONE, BUFF_BARRIER, TAG_NONE,
          IC_SHIELD, 40, 0, 16, 0, 0, 1, 150, 0, 0, C_GOLD,
          UF("ENHANCED HIDE", "GRANTS UNSTOPPABLE", RF_UNSTOP),
          { UF("REJUVENATING HIDE", "ALSO HEALS 15%", RF_HEAL), UC("THICK HIDE", "+30% BARRIER", 30) } },
        { "SOAR", "LEAP ON EAGLE WINGS AND STUN", CAT_MASTERY, SB_STRIKE, EL_PHYS, ST_STUN, 0, TAG_EAGLE, IC_WING,
          2.2, 0, 9, 34, 140, 1, 10, 50, 0, RGB565(220, 240, 255),
          UN("ENHANCED SOAR", "+1 STRIKE", 1),
          { UF("RAPTOR SOAR", "MAKES VULNERABLE", RF_VULN), UE("STORM SOAR", "LIGHTNING DAMAGE", 0, EL_LIGHT) } },
        { "JAGUAR RUSH", "+30% ATTACK SPEED FOR 5S", CAT_MASTERY, SB_BUFF, EL_PHYS, ST_NONE, BUFF_SPEED, TAG_JAGUAR,
          IC_CLAWS, 30, 0, 14, 0, 0, 1, 150, 0, 0, C_GOLD,
          UD("ENHANCED RUSH", "+2 SECONDS", 60),
          { UF("FEROCIOUS RUSH", "ALSO GRANTS BERSERKING", RF_BERSERK), UC("SWIFT RUSH", "+30% STRENGTH", 30) } },
        { "SPIRIT AVATAR", "ULTIMATE: [x]35% DMG, SPEED, UNSTOPPABLE", CAT_ULTIMATE, SB_BUFF, EL_PHYS, ST_NONE,
          BUFF_ULT, TAG_NONE, IC_STAR, 35, 0, 45, 0, 0, 1, 240, 0, 0, RGB565(150, 255, 230),
          UD("ENHANCED AVATAR", "+3 SECONDS", 90),
          { UC("EMPOWERED AVATAR", "+40% STRENGTH", 40), UF("RENEWING AVATAR", "ALSO HEALS 15%", RF_HEAL) } },
    },
};

/* name, desc (per rank), icon, mod, arg, value per rank */
const PassiveDef passive_defs[CLASS_COUNT][CLASS_PASSIVES] = {
    [CLASS_BARBARIAN] = {
        { "PIT FIGHTER", "+4% DAMAGE TO CLOSE ENEMIES", IC_SWORD, MOD_ADD_CLOSE, 0, 4 },
        { "THICK SKIN", "+3% DAMAGE REDUCTION", IC_SHIELD, MOD_DR, 0, 3 },
        { "BLOODLETTER", "+8% DAMAGE OVER TIME", IC_BLOOD, MOD_ADD_DOT, 0, 8 },
        { "HEAVY HANDED", "+6% CRITICAL STRIKE DAMAGE", IC_HAMMER, MOD_CRIT_DMG, 0, 6 },
        { "BRAWLER", "+8% OVERPOWER DAMAGE", IC_FIST, MOD_OP_DMG, 0, 8 },
        { "IMPOSING PRESENCE", "+5% MAXIMUM LIFE", IC_HEART, MOD_LIFE_PCT, 0, 5 },
    },
    [CLASS_SORCERER] = {
        { "GLASS CANNON", "+6% DAMAGE", IC_STAR, MOD_ADD_DMG, 0, 6 },
        { "DEVASTATION", "+10 MAXIMUM MANA", IC_EYE, MOD_MAX_RES, 0, 10 },
        { "ELEMENTAL DOMINANCE", "+6% CORE SKILL DAMAGE", IC_FIREBALL, MOD_ADD_CORE, 0, 6 },
        { "PRECISION MAGIC", "+2% CRITICAL STRIKE CHANCE", IC_SPARK, MOD_CRIT, 0, 2 },
        { "PERMAFROST", "+6% DAMAGE TO CROWD CONTROLLED", IC_SNOW, MOD_ADD_CC, 0, 6 },
        { "ALIGN THE ELEMENTS", "+3% COOLDOWN REDUCTION", IC_STORM, MOD_CDR, 0, 3 },
    },
    [CLASS_ROGUE] = {
        { "SHARPSHOOTER", "+6% CRITICAL STRIKE DAMAGE", IC_EYE, MOD_CRIT_DMG, 0, 6 },
        { "EXPLOITATION", "+8% VULNERABLE DAMAGE", IC_DAGGER, MOD_VULN_DMG, 0, 8 },
        { "DEADLY VENOM", "+8% DAMAGE OVER TIME", IC_VIAL, MOD_ADD_DOT, 0, 8 },
        { "TRICK ATTACKS", "+6% DAMAGE TO CROWD CONTROLLED", IC_TRAP, MOD_ADD_CC, 0, 6 },
        { "WEAPON MASTERY", "+3% ATTACK SPEED", IC_BOW, MOD_ATK_SPD, 0, 3 },
        { "AGILITY", "+4% MOVEMENT SPEED", IC_DASH, MOD_MOVE, 0, 4 },
    },
    [CLASS_NECRO] = {
        { "GRIM HARVEST", "+2 ESSENCE ON KILL", IC_SCYTHE, MOD_RES_KILL, 0, 2 },
        { "UNLIFE", "+5% MAXIMUM LIFE", IC_HEART, MOD_LIFE_PCT, 0, 5 },
        { "DEATH'S EMBRACE", "+4% DAMAGE REDUCTION FROM CLOSE", IC_SHIELD, MOD_DR_CLOSE, 0, 4 },
        { "SKELETAL MASTERY", "+10% MINION DAMAGE", IC_SKULL, MOD_ADD_TAG, TAG_MINION, 10 },
        { "BONE MASTERY", "+6% CRITICAL STRIKE DAMAGE", IC_BONE, MOD_CRIT_DMG, 0, 6 },
        { "IMPERFECT BALANCE", "+5% CORE SKILL DAMAGE", IC_BLOOD, MOD_ADD_CORE, 0, 5 },
    },
    [CLASS_DRUID] = {
        { "NATURE'S FURY", "+5% DAMAGE", IC_PAW, MOD_ADD_DMG, 0, 5 },
        { "VIGILANCE", "+3% DAMAGE REDUCTION", IC_SHIELD, MOD_DR, 0, 3 },
        { "PREDATORY INSTINCT", "+2% CRITICAL STRIKE CHANCE", IC_CLAWS, MOD_CRIT, 0, 2 },
        { "EARTHEN MIGHT", "+8% OVERPOWER DAMAGE", IC_ROCKS, MOD_OP_DMG, 0, 8 },
        { "ELECTRIC SHOCK", "+6% LIGHTNING DAMAGE", IC_SPARK, MOD_ADD_ELEM, EL_LIGHT, 6 },
        { "PROTECTION", "+8% BARRIER GENERATION", IC_BOULDER, MOD_BARRIER_GEN, 0, 8 },
    },
    [CLASS_SPIRITBORN] = {
        { "VIGOROUS", "+10 MAXIMUM VIGOR", IC_HEART, MOD_MAX_RES, 0, 10 },
        { "SWIFT FEATHERS", "+3% ATTACK SPEED", IC_FEATHER, MOD_ATK_SPD, 0, 3 },
        { "PROTECTOR", "+3% DAMAGE REDUCTION", IC_SHIELD, MOD_DR, 0, 3 },
        { "PREDATOR'S FOCUS", "+6% CRITICAL STRIKE DAMAGE", IC_EYE, MOD_CRIT_DMG, 0, 6 },
        { "ACIDIC VENOM", "+6% POISON DAMAGE", IC_CENTIPEDE, MOD_ADD_ELEM, EL_POISON, 6 },
        { "FERAL STRENGTH", "+8% OVERPOWER DAMAGE", IC_PALM, MOD_OP_DMG, 0, 8 },
    },
};

const KeyPassiveDef key_defs[CLASS_COUNT][CLASS_KEYS] = {
    [CLASS_BARBARIAN] = {
        { "UNBRIDLED RAGE", "WHILE BERSERKING: [x]25% DAMAGE. CRITS GRANT BERSERKING." },
        { "HEMORRHAGE", "BLEEDING ENEMIES TAKE [x]30% DAMAGE FROM CORE SKILLS." },
        { "TITAN'S GRIP", "+10% OVERPOWER CHANCE AND [x]40% OVERPOWER DAMAGE." },
    },
    [CLASS_SORCERER] = {
        { "COMBUSTION", "BURNING ENEMIES EXPLODE ON DEATH. [x]20% BURNING DAMAGE." },
        { "SHATTERING ICE", "FROZEN ENEMIES TAKE [x]60% DAMAGE." },
        { "OVERCHARGE", "LIGHTNING CRITICAL STRIKES DEAL TRIPLE CRIT DAMAGE." },
    },
    [CLASS_ROGUE] = {
        { "DEADEYE", "+30% CRIT CHANCE VS HEALTHY ENEMIES, [x]20% CRIT DAMAGE." },
        { "VIRULENCE", "POISON DEALS [x]100% DAMAGE." },
        { "HAIL OF STEEL", "PROJECTILE SKILLS FIRE 2 EXTRA PROJECTILES." },
    },
    [CLASS_NECRO] = {
        { "MARROW SURGE", "BONE SKILLS DEAL [x]30% DAMAGE, +10% CRIT CHANCE." },
        { "COMMANDER", "+2 MINIONS. MINIONS DEAL [x]60% DAMAGE." },
        { "BLOOD BOND", "BLOOD SKILLS HEAL 3% LIFE. [x]30% DAMAGE WHILE HEALTHY." },
    },
    [CLASS_DRUID] = {
        { "EARTH SPIRIT", "EARTH SKILLS +15% OVERPOWER CHANCE, [x]30% OVERPOWER DMG." },
        { "STORMCALLER", "STORM SKILLS +1 CHAIN OR TORNADO AND DEAL [x]25%." },
        { "PRIMAL BEAST", "BEAST SKILLS DEAL [x]30% DAMAGE, +15% ATTACK SPEED." },
    },
    [CLASS_SPIRITBORN] = {
        { "JAGUAR'S FEROCITY", "KILLS STACK [x]6% DAMAGE AND +4% ATTACK SPEED, UP TO 5." },
        { "EAGLE'S EYE", "EAGLE SKILLS MAKE VULNERABLE AND DEAL [x]25% DAMAGE." },
        { "CENTIPEDE'S PLAGUE", "POISONED ENEMIES TAKE [x]20% DAMAGE. POISON [x]50%." },
    },
};

const SkillDef *skill_def(int cls, int i)
{
    return &skill_defs[CLAMP(cls, 0, CLASS_COUNT - 1)][CLAMP(i, 0, CLASS_SKILLS - 1)];
}

const char *skill_cat_name(SkillCat c)
{
    static const char *const n[CAT_COUNT] = { "BASIC", "CORE", "DEFENSIVE", "MASTERY", "ULTIMATE" };
    return n[c % CAT_COUNT];
}

/* ------------------------------------------------------------- the tree */

int skill_cat_gate(SkillCat c)
{
    static const int gate[CAT_COUNT] = { 0, 2, 5, 10, 16 };
    return gate[c % CAT_COUNT];
}

int key_passive_gate(void) { return 22; }

static int passive_gate(int i) { return i < 2 ? 2 : i < 4 ? 5 : 10; }

int skill_points_spent(const Profile *p)
{
    int i, n = p->key_passive ? 1 : 0;
    for (i = 0; i < CLASS_SKILLS; i++)
        n += p->skill_rank[i] + (p->skill_enh[i] ? 1 : 0) + (p->skill_upg[i] ? 1 : 0);
    for (i = 0; i < CLASS_PASSIVES; i++)
        n += p->passive[i];
    return n;
}

bool skill_can_rank(const Profile *p, int i)
{
    if (i < 0 || i >= CLASS_SKILLS || p->skill_points <= 0 || p->skill_rank[i] >= SKILL_MAX_RANK)
        return false;
    return skill_points_spent(p) >= skill_cat_gate((SkillCat)skill_def(p->cls, i)->cat);
}

bool skill_rank_up(Profile *p, int i)
{
    int s;
    if (!skill_can_rank(p, i))
        return false;
    p->skill_rank[i]++;
    p->skill_points--;
    /* A freshly learned skill goes on the bar if there is room. */
    for (s = 0; s < BAR_SLOTS && p->skill_rank[i] == 1 && !skill_on_bar(p, i); s++)
        if (p->bar[s] == NO_SKILL)
            p->bar[s] = (uint8_t)i;
    return true;
}

bool skill_can_enhance(const Profile *p, int i)
{
    return i >= 0 && i < CLASS_SKILLS && p->skill_points > 0 && p->skill_rank[i] > 0 && !p->skill_enh[i];
}

bool skill_enhance(Profile *p, int i)
{
    if (!skill_can_enhance(p, i))
        return false;
    p->skill_enh[i] = 1;
    p->skill_points--;
    return true;
}

bool skill_set_upgrade(Profile *p, int i, int upg)
{
    if (i < 0 || i >= CLASS_SKILLS || upg < 1 || upg > 2 || !p->skill_enh[i])
        return false;
    if (p->skill_upg[i] == 0) {          /* the first choice costs a point */
        if (p->skill_points <= 0)
            return false;
        p->skill_points--;
    }
    p->skill_upg[i] = (uint8_t)upg;
    return true;
}

bool passive_can_rank(const Profile *p, int i)
{
    return i >= 0 && i < CLASS_PASSIVES && p->skill_points > 0 && p->passive[i] < PASSIVE_MAX_RANK
        && skill_points_spent(p) >= passive_gate(i);
}

bool passive_rank_up(Profile *p, int i)
{
    if (!passive_can_rank(p, i))
        return false;
    p->passive[i]++;
    p->skill_points--;
    return true;
}

bool key_passive_set(Profile *p, int k)
{
    if (k < 1 || k > CLASS_KEYS)
        return false;
    if (p->key_passive == 0) {
        if (p->skill_points <= 0 || skill_points_spent(p) < key_passive_gate())
            return false;
        p->skill_points--;
    }
    p->key_passive = (uint8_t)k;         /* switching between keys is free */
    return true;
}

bool skill_on_bar(const Profile *p, int i)
{
    int s;
    for (s = 0; s < BAR_SLOTS; s++)
        if (p->bar[s] == i)
            return true;
    return false;
}

bool skill_bar_set(Profile *p, int slot, int i)
{
    int s;
    if (slot < 0 || slot >= BAR_SLOTS || i < 0 || i >= CLASS_SKILLS || p->skill_rank[i] == 0)
        return false;
    for (s = 0; s < BAR_SLOTS; s++)
        if (p->bar[s] == i)
            p->bar[s] = p->bar[slot];     /* swap places */
    p->bar[slot] = (uint8_t)i;
    return true;
}

void skill_refund_all(Profile *p)
{
    p->skill_points += skill_points_spent(p);
    memset(p->skill_rank, 0, sizeof p->skill_rank);
    memset(p->skill_enh, 0, sizeof p->skill_enh);
    memset(p->skill_upg, 0, sizeof p->skill_upg);
    memset(p->passive, 0, sizeof p->passive);
    memset(p->bar, NO_SKILL, sizeof p->bar);
    p->key_passive = 0;
}

/* ------------------------------------------------------------ names */

const char *element_name(Element e)
{
    static const char *const n[EL_COUNT] = { "PHYSICAL", "FIRE", "COLD", "LIGHTNING", "POISON", "SHADOW" };
    return n[e % EL_COUNT];
}

uint16_t element_color(Element e)
{
    switch (e) {
    case EL_FIRE:   return C_FIRE;
    case EL_COLD:   return C_COLD;
    case EL_LIGHT:  return C_LIGHT;
    case EL_POISON: return C_POIS;
    case EL_SHADOW: return C_SHAD;
    default:        return C_PHYS;
    }
}

const char *status_name(StatusKind s)
{
    static const char *const n[ST_COUNT] = { "", "FROZEN", "STUNNED", "CHILLED", "IMMOBILIZED",
                                             "BURNING", "POISONED", "BLEEDING", "WITHERING" };
    return n[s % ST_COUNT];
}
