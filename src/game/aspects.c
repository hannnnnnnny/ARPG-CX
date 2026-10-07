#include "aspects.h"
#include "../i18n/i18n.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Shorthands for the tables below. */
#define OFF ASC_OFFENSE
#define DEF ASC_DEFENSE
#define UTL ASC_UTILITY
#define RES ASC_RESOURCE
#define MOB ASC_MOBILITY
#define ANY ANY_CLASS

/* Skill indices are per class: see skill_defs in skills.c. */
const AspectDef aspect_defs[] = {
    { "", "", ANY, OFF, MOD_NONE, 0, 0, 0, MOD_NONE, 0, 0 },
    /* ---- any class */
    { "ASPECT OF ASCENDANCY", "[x]#% DAMAGE", ANY, OFF, MOD_X_ALL, 0, 8, 15, 0, 0, 0 },
    { "ASPECT OF EXPOSURE", "[x]#% DAMAGE TO VULNERABLE ENEMIES", ANY, OFF, MOD_X_VULN, 0, 15, 30, 0, 0, 0 },
    { "ASPECT OF THE KEEN EDGE", "[x]#% CRITICAL STRIKE DAMAGE", ANY, OFF, MOD_X_CRIT, 0, 12, 25, 0, 0, 0 },
    { "ASPECT OF CRUSHING WEIGHT", "[x]#% OVERPOWER DAMAGE, +5% OVERPOWER CHANCE", ANY, OFF, MOD_X_OP, 0, 25, 50,
      MOD_OP_CHANCE, 0, 5 },
    { "ASPECT OF THE SNARE", "[x]#% DAMAGE TO CROWD CONTROLLED ENEMIES", ANY, OFF, MOD_X_CC, 0, 12, 25, 0, 0, 0 },
    { "ASPECT OF LINGERING WOUNDS", "[x]#% DAMAGE OVER TIME", ANY, OFF, MOD_X_DOT, 0, 15, 30, 0, 0, 0 },
    { "ASPECT OF THE HUNT", "LUCKY HIT: UP TO #% CHANCE TO MAKE ENEMIES VULNERABLE", ANY, OFF, MOD_LH_VULN, 0, 20, 40,
      0, 0, 0 },
    { "ASPECT OF RECKLESS FURY", "+#% ATTACK SPEED", ANY, OFF, MOD_ATK_SPD, 0, 8, 15, 0, 0, 0 },
    { "ASPECT OF THE WARDEN", "#% DAMAGE REDUCTION", ANY, DEF, MOD_DR, 0, 8, 15, 0, 0, 0 },
    { "ASPECT OF THE BULWARK", "COOLDOWN SKILLS GRANT A BARRIER OF #% LIFE", ANY, DEF, MOD_BARRIER_CD, 0, 10, 20,
      0, 0, 0 },
    { "ASPECT OF RENEWAL", "KILLS HEAL #% OF YOUR LIFE", ANY, DEF, MOD_HEAL_KILL, 0, 1, 3, 0, 0, 0 },
    { "ASPECT OF THORNED HIDE", "+#% THORNS", ANY, DEF, MOD_THORNS_PCT, 0, 50, 100, 0, 0, 0 },
    { "ASPECT OF THE UNBROKEN", "+#% MAXIMUM LIFE", ANY, DEF, MOD_LIFE_PCT, 0, 8, 15, 0, 0, 0 },
    { "ASPECT OF THE WELLSPRING", "KILLS RESTORE # RESOURCE", ANY, RES, MOD_RES_KILL, 0, 2, 6, 0, 0, 0 },
    { "ASPECT OF FRUGALITY", "#% RESOURCE COST REDUCTION", ANY, RES, MOD_COST, 0, 8, 16, 0, 0, 0 },
    { "ASPECT OF HASTENED HOURS", "#% COOLDOWN REDUCTION", ANY, UTL, MOD_CDR, 0, 6, 12, 0, 0, 0 },
    { "ASPECT OF FORTUNE", "+#% GOLD FOUND AND +20% EXPERIENCE", ANY, UTL, MOD_GOLD, 0, 30, 60, MOD_XP, 0, 20 },
    { "ASPECT OF THE WANDERER", "+#% MOVEMENT SPEED", ANY, MOB, MOD_MOVE, 0, 10, 20, 0, 0, 0 },
    /* ---- barbarian: 0 FLAY 1 FRENZY 2 WHIRLWIND 3 GROUND SLAM 4 REND 5 WAR CRY 6 IRON SKIN
     *                 7 LEAP SLAM 8 EARTHQUAKE 9 BLOOD FRENZY */
    { "ASPECT OF THE BLOODREAVER", "WHIRLWIND DEALS [x]#% DAMAGE AND HEALS", CLASS_BARBARIAN, OFF, MOD_X_SKILL, 2,
      25, 50, MOD_SK_FLAGS, 2, RF_HEAL },
    { "ASPECT OF THE RAVAGER", "REND DEALS [x]#% DAMAGE", CLASS_BARBARIAN, OFF, MOD_X_SKILL, 4, 40, 80, 0, 0, 0 },
    { "ASPECT OF TECTONIC FISTS", "LEAP SLAM DEALS [x]#% AND LEAVES AN EARTHQUAKE", CLASS_BARBARIAN, OFF,
      MOD_X_SKILL, 7, 20, 40, MOD_SK_FLAGS, 7, RF_GROUND },
    { "ASPECT OF AFTERMATH", "EARTHQUAKE +#% SIZE AND LASTS 2S LONGER", CLASS_BARBARIAN, OFF, MOD_SK_RADIUS, 8,
      30, 60, MOD_SK_DUR, 8, 60 },
    { "ASPECT OF THE THUNDER HAMMER", "GROUND SLAM DEALS [x]#% AND HITS ALL AROUND", CLASS_BARBARIAN, OFF,
      MOD_X_SKILL, 3, 30, 60, MOD_SK_FLAGS, 3, RF_NOVA },
    { "ASPECT OF ENDLESS RAGE", "WAR CRY COOLDOWN -#%, GRANTS UNSTOPPABLE", CLASS_BARBARIAN, UTL, MOD_SK_CD, 5,
      -20, -40, MOD_SK_FLAGS, 5, RF_UNSTOP },
    /* ---- sorcerer: 0 SPARK 1 FIRE BOLT 2 FIREBALL 3 CHAIN LIGHTNING 4 ICE SHARDS 5 FROST NOVA
     *                6 ICE ARMOR 7 METEOR 8 THUNDERSTORM 9 HELLFIRE */
    { "ASPECT OF THE EMBERFALL", "FIREBALL DEALS [x]#% AND LEAVES BURNING GROUND", CLASS_SORCERER, OFF,
      MOD_X_SKILL, 2, 20, 40, MOD_SK_FLAGS, 2, RF_GROUND },
    { "ASPECT OF THE CINDER CROWN", "METEOR COOLDOWN -#%", CLASS_SORCERER, OFF, MOD_SK_CD, 7, -30, -50, 0, 0, 0 },
    { "ASPECT OF WINTER'S HEART", "FROST NOVA COOLDOWN -#%, MAKES VULNERABLE", CLASS_SORCERER, OFF, MOD_SK_CD, 5,
      -30, -50, MOD_SK_FLAGS, 5, RF_VULN },
    { "ASPECT OF THE GLACIER", "ICE SHARDS +# SHARDS THAT FREEZE", CLASS_SORCERER, OFF, MOD_SK_COUNT, 4, 2, 4,
      MOD_SK_STATUS, 4, ST_FREEZE },
    { "ASPECT OF THE THUNDERLORD", "CHAIN LIGHTNING +# CHAINS", CLASS_SORCERER, OFF, MOD_SK_COUNT, 3, 3, 6, 0, 0, 0 },
    { "ASPECT OF THE STORM'S EYE", "THUNDERSTORM +#% SIZE AND FOLLOWS YOU", CLASS_SORCERER, OFF, MOD_SK_RADIUS, 8,
      30, 60, MOD_SK_FLAGS, 8, RF_FOLLOW },
    /* ---- rogue: 0 PUNCTURE 1 HEARTSEEKER 2 PENETRATING SHOT 3 RAPID FIRE 4 BARRAGE 5 SPIKE TRAP
     *             6 SHADOW DASH 7 POISON IMBUEMENT 8 COLD IMBUEMENT 9 RAIN OF ARROWS */
    { "ASPECT OF THE DUSK LONGBOW", "PENETRATING SHOT DEALS [x]#% DAMAGE", CLASS_ROGUE, OFF, MOD_X_SKILL, 2, 30, 60,
      0, 0, 0 },
    { "ASPECT OF THE HAWK", "+#% CRITICAL STRIKE CHANCE", CLASS_ROGUE, OFF, MOD_CRIT, 0, 8, 15, 0, 0, 0 },
    { "ASPECT OF THE VIPER", "IMBUED SKILLS DEAL [x]#% DAMAGE", CLASS_ROGUE, OFF, MOD_X_TAG, TAG_IMBUE, 20, 40,
      0, 0, 0 },
    { "ASPECT OF THE QUIVER", "BARRAGE FIRES +# ARROWS", CLASS_ROGUE, OFF, MOD_SK_COUNT, 4, 3, 5, 0, 0, 0 },
    { "ASPECT OF SKYFALL", "RAIN OF ARROWS COOLDOWN -#%", CLASS_ROGUE, OFF, MOD_SK_CD, 9, -25, -50, 0, 0, 0 },
    { "ASPECT OF THE PLAGUEBEARER", "SPIKE TRAP +#% SIZE AND POISONS", CLASS_ROGUE, OFF, MOD_SK_RADIUS, 5, 30, 60,
      MOD_SK_STATUS, 5, ST_POISON },
    /* ---- necromancer: 0 REAP 1 BONE SPLINTERS 2 BONE SPEAR 3 BLIGHT 4 BLOOD SURGE 5 BONE PRISON
     *                   6 BLOOD MIST 7 RAISE DEAD 8 CORPSE EXPLOSION 9 LEGION OF BONE */
    { "ASPECT OF THE SPLINTERED SPEAR", "BONE SPEAR DEALS [x]#% AND SHATTERS", CLASS_NECRO, OFF, MOD_X_SKILL, 2,
      30, 60, MOD_SK_FLAGS, 2, RF_EXPLODE },
    { "ASPECT OF THE BLIGHTED", "BLIGHT +#% SIZE, [x]20% DAMAGE", CLASS_NECRO, OFF, MOD_SK_RADIUS, 3, 40, 80,
      MOD_X_SKILL, 3, 20 },
    { "ASPECT OF THE SANGUINE", "BLOOD SURGE DEALS [x]#% DAMAGE", CLASS_NECRO, OFF, MOD_X_SKILL, 4, 30, 60, 0, 0, 0 },
    { "ASPECT OF THE GRAVE LEGION", "+# SKELETON WARRIORS", CLASS_NECRO, OFF, MOD_MINION_COUNT, 0, 1, 3, 0, 0, 0 },
    { "ASPECT OF THE HOLLOW MARROW", "MINIONS DEAL [x]#% DAMAGE", CLASS_NECRO, OFF, MOD_X_MINION, 0, 30, 60, 0, 0, 0 },
    { "ASPECT OF CORPSE BLOOM", "CORPSE EXPLOSION +#% SIZE, [x]30% DAMAGE", CLASS_NECRO, OFF, MOD_SK_RADIUS, 8,
      30, 60, MOD_X_SKILL, 8, 30 },
    /* ---- druid: 0 STORM STRIKE 1 MAUL 2 TORNADO 3 LANDSLIDE 4 SHRED 5 EARTHEN BULWARK
     *             6 CYCLONE ARMOR 7 WOLF PACK 8 BOULDER 9 TEMPEST FURY */
    { "ASPECT OF THE TEMPEST", "TORNADO SPAWNS +# TORNADOES", CLASS_DRUID, OFF, MOD_SK_COUNT, 2, 1, 3, 0, 0, 0 },
    { "ASPECT OF THE MOUNTAIN", "LANDSLIDE DEALS [x]#% DAMAGE", CLASS_DRUID, OFF, MOD_X_SKILL, 3, 30, 60, 0, 0, 0 },
    { "ASPECT OF THE PACK LEADER", "+# WOLVES, MINIONS [x]30% DAMAGE", CLASS_DRUID, OFF, MOD_MINION_COUNT, 0, 1, 2,
      MOD_X_MINION, 0, 30 },
    { "ASPECT OF THE ROLLING STONE", "BOULDER DEALS [x]#% AND PIERCES", CLASS_DRUID, OFF, MOD_X_SKILL, 8, 25, 50,
      MOD_SK_FLAGS, 8, RF_PIERCE },
    { "ASPECT OF THE MOONLIT CLAW", "SHRED DEALS [x]#% DAMAGE", CLASS_DRUID, OFF, MOD_X_SKILL, 4, 30, 60, 0, 0, 0 },
    { "ASPECT OF THE STORMHIDE", "CYCLONE ARMOR COOLDOWN -#%", CLASS_DRUID, DEF, MOD_SK_CD, 6, -30, -50, 0, 0, 0 },
    /* ---- spiritborn: 0 THRUST 1 WITHERING FIST 2 QUILL VOLLEY 3 CRUSHING PALM 4 STINGER
     *                  5 CENTIPEDE SWARM 6 ARMORED HIDE 7 SOAR 8 JAGUAR RUSH 9 SPIRIT AVATAR */
    { "ASPECT OF THE FOUR WINDS", "QUILL VOLLEY FIRES +# QUILLS", CLASS_SPIRITBORN, OFF, MOD_SK_COUNT, 2, 2, 4,
      0, 0, 0 },
    { "ASPECT OF THE IRON PALM", "CRUSHING PALM DEALS [x]#% DAMAGE", CLASS_SPIRITBORN, OFF, MOD_X_SKILL, 3, 30, 60,
      0, 0, 0 },
    { "ASPECT OF THE VENOM COIL", "STINGER DEALS [x]#% AND LEAVES POISON", CLASS_SPIRITBORN, OFF, MOD_X_SKILL, 4,
      25, 50, MOD_SK_FLAGS, 4, RF_GROUND },
    { "ASPECT OF THE SWARM", "CENTIPEDE SWARM +#% SIZE AND FOLLOWS YOU", CLASS_SPIRITBORN, OFF, MOD_SK_RADIUS, 5,
      30, 60, MOD_SK_FLAGS, 5, RF_FOLLOW },
    { "ASPECT OF THE PROWLING JAGUAR", "JAGUAR SKILLS DEAL [x]#% DAMAGE", CLASS_SPIRITBORN, OFF, MOD_X_TAG,
      TAG_JAGUAR, 20, 40, 0, 0, 0 },
    { "ASPECT OF THE SKYBORNE", "SOAR COOLDOWN -#%", CLASS_SPIRITBORN, MOB, MOD_SK_CD, 7, -30, -50, 0, 0, 0 },
    /* ---- signature facets (sig.h): each feeds one build-defining unique */
    { "ASPECT OF THE STORMCLAW", "SKY BOLTS CHAIN TO # MORE FOES, SHRED DEALS [x]20%", CLASS_DRUID, OFF,
      MOD_SIG_FACET, SF_CHAIN, 2, 4, MOD_X_SKILL, SIG_SKILL_SHRED, 20 },
    { "ASPECT OF THE MOONLIT HUNT", "SKY BOLTS GRANT #% LIFE AS BARRIER, +15% MOVEMENT SPEED", CLASS_DRUID, MOB,
      MOD_SIG_FACET, SF_LUNGE, 1, 3, MOD_MOVE, 0, 15 },
    { "ASPECT OF SPLINTERED BONE", "BONE SPEAR BURSTS INTO +# SHARDS THAT PIERCE, [x]20% DAMAGE", CLASS_NECRO, OFF,
      MOD_SIG_FACET, SF_SHARDS, 2, 4, MOD_X_SKILL, SIG_SKILL_BONESPEAR, 20 },
    { "ASPECT OF THE MARROW WELL", "BONE NOVAS RESTORE # ESSENCE AND 2% LIFE, +10% MAXIMUM LIFE", CLASS_NECRO, RES,
      MOD_SIG_FACET, SF_MARROW, 4, 8, MOD_LIFE_PCT, 0, 10 },
    { "ASPECT OF THE FIRESTORM", "METEORS FALL IN SHOWERS OF #, FIREBALL DEALS [x]20%", CLASS_SORCERER, OFF,
      MOD_SIG_FACET, SF_SHOWER, 2, 4, MOD_X_SKILL, SIG_SKILL_FIREBALL, 20 },
    { "ASPECT OF THE PHOENIX", "BURNING FOES EXPLODE ON DEATH FOR #%, 10% DAMAGE REDUCTION", CLASS_SORCERER, DEF,
      MOD_SIG_FACET, SF_PHOENIX, 300, 600, MOD_DR, 0, 10 },
};
const int aspect_count = (int)(sizeof aspect_defs / sizeof aspect_defs[0]) - 1;

const UniqueDef unique_defs[] = {
    { "", "", 0, 0, ANY, 0, { 0 }, MOD_NONE, 0, 0, 0, MOD_NONE, 0, 0 },
    { "GRAVEWARDEN'S MAUL", "WHIRLWIND DEALS [x]#% AND IS 30% LARGER", SLOT_WEAPON, 2, CLASS_BARBARIAN, 0,
      { AF_MAINSTAT, AF_CRIT_DMG, AF_VULN_DMG, AF_PHYS }, MOD_X_SKILL, 2, 40, 70, MOD_SK_RADIUS, 2, 30 },
    { "CHAINS OF THE PIT", "#% DAMAGE REDUCTION, EARTHQUAKE FOLLOWS YOU", SLOT_CHEST, 3, CLASS_BARBARIAN, 0,
      { AF_LIFE, AF_ARMOR, AF_DR_CLOSE, AF_THORNS }, MOD_DR, 0, 10, 20, MOD_SK_FLAGS, 8, RF_FOLLOW },
    { "STARFALL CIRCLET", "METEOR FALLS +# TIMES AND DEALS [x]30%", SLOT_HELM, 3, CLASS_SORCERER, 0,
      { AF_CDR, AF_LIFE, AF_MAX_RES, AF_RES_ALL }, MOD_SK_COUNT, 7, 1, 2, MOD_X_SKILL, 7, 30 },
    { "EMBERWEAVE ROBE", "[x]#% FIRE DAMAGE", SLOT_CHEST, 2, CLASS_SORCERER, 0,
      { AF_LIFE, AF_ARMOR, AF_DR, AF_BARRIER }, MOD_X_ELEM, EL_FIRE, 20, 40, 0, 0, 0 },
    { "WHISPERWIND BOW", "PENETRATING SHOT +# ARROWS, [x]40% DAMAGE", SLOT_WEAPON, 5, CLASS_ROGUE, 0,
      { AF_MAINSTAT, AF_CRIT, AF_VULN_DMG, AF_DMG_FAR }, MOD_SK_COUNT, 2, 2, 3, MOD_X_SKILL, 2, 40 },
    { "VIPERSKIN WRAPS", "[x]#% POISON DAMAGE, +8% ATTACK SPEED", SLOT_GLOVES, 1, CLASS_ROGUE, 0,
      { AF_ATK_SPD, AF_CRIT, AF_LUCKY, AF_POISON }, MOD_X_ELEM, EL_POISON, 25, 50, MOD_ATK_SPD, 0, 8 },
    { "OSSUARY CROWN", "+# SKELETON WARRIORS, MINIONS [x]30%", SLOT_HELM, 2, CLASS_NECRO, 0,
      { AF_LIFE, AF_CDR, AF_ARMOR, AF_RES_ALL }, MOD_MINION_COUNT, 0, 2, 3, MOD_X_MINION, 0, 30 },
    { "THE HOLLOW BELL", "BONE SKILLS DEAL [x]#%, +5% CRIT CHANCE", SLOT_AMULET, 2, CLASS_NECRO, 0,
      { AF_CRIT_DMG, AF_VULN_DMG, AF_CDR, AF_MAINSTAT }, MOD_X_TAG, TAG_BONE, 25, 50, MOD_CRIT, 0, 5 },
    { "MOONBARK TOTEM", "+# TORNADOES, STORM SKILLS [x]25%", SLOT_OFFHAND, 2, CLASS_DRUID, 0,
      { AF_CRIT, AF_CDR, AF_COST, AF_LUCKY }, MOD_SK_COUNT, 2, 1, 2, MOD_X_TAG, TAG_STORM, 25 },
    { "STORMHIDE MANTLE", "EARTH SKILLS DEAL [x]#%, +5% OVERPOWER CHANCE", SLOT_CHEST, 3, CLASS_DRUID, 0,
      { AF_LIFE, AF_ARMOR, AF_DR, AF_OP_DMG }, MOD_X_TAG, TAG_EARTH, 25, 50, MOD_OP_CHANCE, 0, 5 },
    { "TALON OF THE FOUR", "QUILL VOLLEY DEALS [x]#%, +3 QUILLS", SLOT_WEAPON, 9, CLASS_SPIRITBORN, 0,
      { AF_MAINSTAT, AF_CRIT_DMG, AF_VULN_DMG, AF_PHYS }, MOD_X_SKILL, 2, 40, 70, MOD_SK_COUNT, 2, 3 },
    { "CENTIPEDE COIL", "[x]#% DAMAGE OVER TIME, 5% DAMAGE REDUCTION", SLOT_PANTS, 3, CLASS_SPIRITBORN, 0,
      { AF_LIFE, AF_ARMOR, AF_DR, AF_RES_ALL }, MOD_X_DOT, 0, 30, 60, MOD_DR, 0, 5 },
    { "BAND OF EMBERLIGHT", "+#% GOLD AND EXPERIENCE, [x]10% DAMAGE", SLOT_RING1, 2, ANY, 0,
      { AF_CRIT, AF_CRIT_DMG, AF_XP, AF_LIFE }, MOD_GOLD, 0, 60, 120, MOD_X_ALL, 0, 10 },
    { "ASHWALKER BOOTS", "+#% MOVEMENT SPEED, 5% DAMAGE REDUCTION", SLOT_BOOTS, 3, ANY, 0,
      { AF_MOVE, AF_LIFE, AF_RES_ALL, AF_ARMOR }, MOD_MOVE, 0, 20, 35, MOD_DR, 0, 5 },
    { "VEIL OF THE PALE MOON", "#% COOLDOWN REDUCTION, +20 MAXIMUM RESOURCE", SLOT_HELM, 1, ANY, 0,
      { AF_LIFE, AF_ARMOR, AF_RES_ALL, AF_COST }, MOD_CDR, 0, 12, 20, MOD_MAX_RES, 0, 20 },
    { "GLOAMING HEART", "[x]#% DAMAGE TO VULNERABLE, LUCKY HIT: 25% VULNERABLE", SLOT_AMULET, 3, ANY, 0,
      { AF_VULN_DMG, AF_CRIT, AF_CDR, AF_LIFE }, MOD_X_VULN, 0, 20, 40, MOD_LH_VULN, 0, 25 },
    /* mythic uniques (mythic.h): every affix is greater, purple light, a power of their own */
    { "CROWN OF THE NAMELESS KING", "STARS RAIN ON YOUR FOES FOR #% WEAPON DAMAGE, +3 RANKS TO ALL SKILLS",
      SLOT_HELM, 3, ANY, 1, { AF_LIFE, AF_CDR, AF_RES_ALL, AF_MAX_RES }, MOD_MYTHIC, MY_STARFALL, 700, 1100,
      MOD_ALL_RANKS, 0, 3 },
    { "THE SHATTERED STAR", "CRITICAL STRIKES BURST INTO SIX STAR SHARDS FOR #%, +15% CRITICAL STRIKE CHANCE",
      SLOT_RING1, 3, ANY, 1, { AF_CRIT_DMG, AF_VULN_DMG, AF_OP_DMG, AF_LIFE }, MOD_MYTHIC, MY_SHATTER, 300, 500,
      MOD_CRIT, 0, 15 },
    /* build-defining uniques: each switches on a signature build (sig.h) */
    { "STORMHOWL PELT", "SHRED IS LIGHTNING: EVERY HIT CALLS A SKY BOLT FOR #% THAT HEALS YOU", SLOT_CHEST, 3,
      CLASS_DRUID, 0, { AF_LIFE, AF_ATK_SPD, AF_CRIT, AF_LIGHT }, MOD_SIGNATURE, SIG_STORMWOLF, 900, 1300,
      MOD_ATK_SPD, 0, 15 },
    { "SPINE OF THE FIRST KEEPER", "BONE SPEAR BURSTS INTO SHARDS, CRITS ERUPT IN A BONE NOVA FOR #%", SLOT_WEAPON, 2,
      CLASS_NECRO, 0, { AF_MAINSTAT, AF_CRIT, AF_CRIT_DMG, AF_VULN_DMG }, MOD_SIGNATURE, SIG_BONESPEAR, 1200, 1700,
      MOD_CRIT, 0, 10 },
    { "HEART OF THE INFERNO", "FIREBALLS EXPLODE TWICE AND CALL METEORS FOR #%", SLOT_AMULET, 2, CLASS_SORCERER, 0,
      { AF_CRIT_DMG, AF_CDR, AF_MAINSTAT, AF_FIRE }, MOD_SIGNATURE, SIG_INFERNO, 1100, 1600, MOD_X_ELEM, EL_FIRE, 25 },
    /* super ancestral mythics: any class, several can be worn at once */
    { "WORLDSPLITTER", "EVERY FIFTH HIT LOOSES A CRESCENT WAVE FOR #%, [x]60% DAMAGE", SLOT_WEAPON, 0, ANY, 1,
      { AF_MAINSTAT, AF_CRIT_DMG, AF_VULN_DMG, AF_CORE_DMG }, MOD_MYTHIC, MY_CRESCENT, 900, 1500, MOD_X_ALL, 0, 60 },
    { "SCYTHE OF ENDLESS NIGHT", "HITS REAP ANY FOE BUT A GUARDIAN BELOW #% LIFE, +25% ATTACK SPEED", SLOT_WEAPON, 0,
      ANY, 1, { AF_MAINSTAT, AF_CRIT_DMG, AF_OP_DMG, AF_DMG_ELITE }, MOD_MYTHIC, MY_REAPER, 20, 30, MOD_ATK_SPD, 0,
      25 },
    { "TOME OF THE HUNGRY VOID", "A SINGULARITY DRAGS FOES IN AND BURSTS FOR #%, 20% COOLDOWN REDUCTION",
      SLOT_OFFHAND, 0, ANY, 1, { AF_CRIT, AF_CDR, AF_LUCKY, AF_COST }, MOD_MYTHIC, MY_VOID, 1200, 1800, MOD_CDR, 0,
      20 },
    { "HOURGLASS OF STILL TIME", "EVERY 15 SECONDS TIME STOPS, FROZEN FOES TAKE [x]#% DAMAGE, [x]20% DAMAGE",
      SLOT_OFFHAND, 0, ANY, 1, { AF_CRIT, AF_CDR, AF_LUCKY, AF_VULN_DMG }, MOD_MYTHIC, MY_TIMESTOP, 60, 120,
      MOD_X_ALL, 0, 20 },
    { "DIADEM OF A THOUSAND EYES", "HITS MAY LOOSE THREE EYE BEAMS FOR #%, +80% CRITICAL STRIKE DAMAGE", SLOT_HELM,
      2, ANY, 1, { AF_LIFE, AF_CDR, AF_RES_ALL, AF_COST }, MOD_MYTHIC, MY_EYES, 350, 600, MOD_CRIT_DMG, 0, 80 },
    { "AEGIS OF THE UNDYING", "[x]# MAXIMUM LIFE, DEATH IS REFUSED ONCE EVERY 30 SECONDS, 30% DAMAGE REDUCTION",
      SLOT_CHEST, 3, ANY, 1, { AF_LIFE, AF_ARMOR, AF_DR, AF_RES_ALL }, MOD_MYTHIC, MY_UNDYING, 3, 5, MOD_DR, 0, 30 },
    { "DRAGONSCALE HAUBERK", "A RING OF DRAGONFIRE BURNS AROUND YOU FOR #% EVERY SECOND, +60% MAXIMUM LIFE",
      SLOT_CHEST, 3, ANY, 1, { AF_LIFE, AF_ARMOR, AF_DR_CLOSE, AF_THORNS }, MOD_MYTHIC, MY_DRAGON, 500, 800,
      MOD_LIFE_PCT, 0, 60 },
    { "GRASP OF THE THUNDER KING", "CRITICAL STRIKES CALL CHAIN LIGHTNING THROUGH SIX FOES FOR #%, +20% ATTACK SPEED",
      SLOT_GLOVES, 2, ANY, 1, { AF_ATK_SPD, AF_CRIT, AF_CRIT_DMG, AF_LUCKY }, MOD_MYTHIC, MY_THUNDER, 350, 600,
      MOD_ATK_SPD, 0, 20 },
    { "HANDS OF A HUNDRED BLADES", "FOUR SPECTRAL BLADES CIRCLE YOU, CUTTING FOR #%, +40% ATTACK SPEED", SLOT_GLOVES,
      2, ANY, 1, { AF_ATK_SPD, AF_CRIT, AF_VULN_DMG, AF_LIFE_HIT }, MOD_MYTHIC, MY_BLADES, 120, 200, MOD_ATK_SPD, 0,
      40 },
    { "MAGMAWALKER LEGPLATES", "YOUR STEPS LEAVE BURNING GROUND FOR #% A SECOND, +30% MOVEMENT SPEED", SLOT_PANTS, 3,
      ANY, 1, { AF_LIFE, AF_ARMOR, AF_DR, AF_RES_ALL }, MOD_MYTHIC, MY_MAGMA, 250, 400, MOD_MOVE, 0, 30 },
    { "GREAVES OF THE SLAUGHTER", "EACH KILL GRANTS [x]4% DAMAGE FOR 4 SECONDS, UP TO [x]#%, 15% DAMAGE REDUCTION",
      SLOT_PANTS, 3, ANY, 1, { AF_LIFE, AF_ARMOR, AF_DR_CLOSE, AF_BARRIER }, MOD_MYTHIC, MY_SLAUGHTER, 120, 200,
      MOD_DR, 0, 15 },
    { "VOIDSTRIDE BOOTS", "YOU BLINK ONTO DISTANT FOES AND BURST FOR #%, +50% MOVEMENT SPEED", SLOT_BOOTS, 3, ANY, 1,
      { AF_MOVE, AF_LIFE, AF_RES_ALL, AF_ARMOR }, MOD_MYTHIC, MY_BLINK, 400, 700, MOD_MOVE, 0, 50 },
    { "ABYSSDIVER TREADS", "A CLEARED FLOOR HAS A #% CHANCE TO DROP YOU TWO FLOORS DEEPER, +40% MOVEMENT SPEED",
      SLOT_BOOTS, 3, ANY, 1, { AF_MOVE, AF_LIFE, AF_RES_ALL, AF_XP }, MOD_MYTHIC, MY_DIVER, 25, 40, MOD_MOVE, 0, 40 },
    { "HEART OF RUIN", "THE SLAIN EXPLODE FOR #%, [x]40% DAMAGE", SLOT_AMULET, 3, ANY, 1,
      { AF_CRIT_DMG, AF_VULN_DMG, AF_CDR, AF_MAINSTAT }, MOD_MYTHIC, MY_RUIN, 500, 800, MOD_X_ALL, 0, 40 },
    { "SIGIL OF THE ONE TRUE NAME", "[x]#% DAMAGE, +2 RANKS TO ALL SKILLS", SLOT_AMULET, 3, ANY, 1,
      { AF_CRIT, AF_CRIT_DMG, AF_ATK_SPD, AF_MAINSTAT }, MOD_MYTHIC, MY_ONENAME, 100, 160, MOD_ALL_RANKS, 0, 2 },
    { "RING OF DEVOURING", "EVERY 3 SECONDS FOES WITHIN # PACES ARE DRAGGED TO YOU AND FEED YOU, [x]25% DAMAGE",
      SLOT_RING1, 3, ANY, 1, { AF_CRIT, AF_CRIT_DMG, AF_LIFE_HIT, AF_LIFE }, MOD_MYTHIC, MY_DEVOUR, 110, 150,
      MOD_X_ALL, 0, 25 },
    { "THE ENDLESS ORBIT", "SKILLS COST NOTHING, COOLDOWNS -#%, +20% ATTACK SPEED", SLOT_RING1, 3, ANY, 1,
      { AF_CRIT, AF_LUCKY, AF_COST, AF_RES_GEN }, MOD_MYTHIC, MY_ORBIT, 40, 60, MOD_ATK_SPD, 0, 20 },
};
const int unique_count = (int)(sizeof unique_defs / sizeof unique_defs[0]) - 1;

const AspectDef *aspect_def(int id)
{
    return id >= 1 && id <= aspect_count ? &aspect_defs[id] : NULL;
}

bool item_is_signature(const Item *it)
{
    const UniqueDef *u;
    if (!it->used || it->rarity != RAR_UNIQUE || !(u = unique_def(it->power)))
        return false;
    return u->kind == MOD_SIGNATURE;
}

const UniqueDef *unique_def(int id)
{
    return id >= 1 && id <= unique_count ? &unique_defs[id] : NULL;
}

static double lerp_roll(double lo, double hi, int roll)
{
    double v = lo + (hi - lo) * CLAMP(roll, 0, 1000) / 1000.0;
    /* Counts (projectiles, minions) only exist in whole numbers. */
    return (fabs(hi) <= 6.0 && fabs(lo) <= 6.0) ? floor(v + 0.5) : v;
}

double aspect_value(const AspectDef *a, int roll) { return lerp_roll(a->lo, a->hi, roll); }
double unique_value(const UniqueDef *u, int roll) { return lerp_roll(u->lo, u->hi, roll); }

static void fill_hash(char *out, size_t cap, const char *fmt, double v)
{
    size_t n = 0;
    char num[16];
    snprintf(num, sizeof num, "%d", (int)(fabs(v) + 0.5));
    for (; *fmt && n + 1 < cap; fmt++) {
        if (*fmt == '#') {
            const char *s = num;
            while (*s && n + 1 < cap)
                out[n++] = *s++;
        } else {
            out[n++] = *fmt;
        }
    }
    out[n] = '\0';
}

void aspect_text(char *out, size_t cap, const AspectDef *a, int roll)
{
    fill_hash(out, cap, T(a->desc), aspect_value(a, roll));
}

void unique_text(char *out, size_t cap, const UniqueDef *u, int roll)
{
    fill_hash(out, cap, T(u->desc), unique_value(u, roll));
}

const char *aspect_cat_name(AspectCat c)
{
    static const char *const n[ASC_COUNT] = { "OFFENSIVE", "DEFENSIVE", "UTILITY", "RESOURCE", "MOBILITY" };
    return n[c % ASC_COUNT];
}

bool aspect_fits_slot(const AspectDef *a, Slot s)
{
    /* Bit per AspectCat: OFF DEF UTL RES MOB */
    static const uint8_t mask[SLOT_COUNT] = {
        [SLOT_WEAPON] = 0x01, [SLOT_OFFHAND] = 0x07, [SLOT_HELM] = 0x06, [SLOT_CHEST] = 0x02,
        [SLOT_GLOVES] = 0x0D, [SLOT_PANTS] = 0x02, [SLOT_BOOTS] = 0x16, [SLOT_AMULET] = 0x1F,
        [SLOT_RING1] = 0x0D, [SLOT_RING2] = 0x0D,
    };
    return (mask[s % SLOT_COUNT] >> a->cat) & 1u;
}

bool aspect_usable(const AspectDef *a, int cls)
{
    return a->cls == ANY_CLASS || a->cls == cls;
}

static int push_mod(Mod *out, int n, int cap, int kind, int arg, double v)
{
    if (kind == MOD_NONE || n >= cap)
        return n;
    out[n].kind = (uint8_t)kind;
    out[n].arg = (uint8_t)arg;
    out[n].value = v;
    return n + 1;
}

int item_power_mods(const Item *it, int cls, Mod *out, int cap)
{
    int n = 0;
    if (!it->used)
        return 0;
    if (it->rarity == RAR_LEGEND) {
        const AspectDef *a = aspect_def(it->power);
        if (a && aspect_usable(a, cls)) {
            n = push_mod(out, n, cap, a->kind, a->arg, aspect_value(a, it->power_roll));
            n = push_mod(out, n, cap, a->kind2, a->arg2, a->v2);
        }
    } else if (it->rarity >= RAR_UNIQUE) {
        const UniqueDef *u = unique_def(it->power);
        if (u && (u->cls == ANY_CLASS || u->cls == cls)) {
            n = push_mod(out, n, cap, u->kind, u->arg, unique_value(u, it->power_roll));
            n = push_mod(out, n, cap, u->kind2, u->arg2, u->v2);
        }
    }
    return n;
}

void codex_learn(Profile *p, int aspect, int roll)
{
    if (aspect < 1 || aspect > aspect_count || aspect >= ASPECT_MAX)
        return;
    roll = CLAMP(roll, 0, 1000);
    if (p->codex[aspect] < (uint16_t)(roll + 1))
        p->codex[aspect] = (uint16_t)(roll + 1);
}

bool codex_known(const Profile *p, int aspect)
{
    return aspect >= 1 && aspect < ASPECT_MAX && p->codex[aspect] > 0;
}

int codex_roll(const Profile *p, int aspect)
{
    return codex_known(p, aspect) ? p->codex[aspect] - 1 : -1;
}
