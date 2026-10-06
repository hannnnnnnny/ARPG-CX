/*
 * story.c - ORIGINAL story text for Ashen Depths. All names, places and
 * events are original to this game.
 *
 * Lines are at most 50 characters so they fit the 320 px screen.
 */
#include "story.h"
#include "world.h"
#include "../i18n/i18n.h"
#include <stdio.h>

const ActDef act_defs[ACT_COUNT] = {
    { "ACT I - THE CRYPT",
      { "FOR A YEAR ASH HAS FALLEN ON CINDERMERE,",
        "SOFT AND GREY, AND IT NEVER MELTS.",
        "NOW THE DEAD OF THE ROYAL CRYPT WALK AGAIN.",
        "OLD BROTHER ALDRIC, LAST KEEPER OF THE SEAL,",
        "PRESSES A TORCH INTO YOUR HAND:",
        "\"GO BELOW. FIND WHAT WAKES THEM.\"", NULL },
      "MORDRAIN THE BONE WARDEN", MT_SKELETON,
      { "THE BONE WARDEN CRUMBLES INTO ASH.",
        "IN ITS RIBS: A CRACKED IRON KEY",
        "STAMPED WITH THE ASHEN KING'S SIGIL.",
        "THE STAIRS GO DOWN. THE AIR SMELLS OF EARTH.", NULL } },
    { "ACT II - THE CATACOMBS",
      { "BENEATH THE CRYPT LIE THE CATACOMBS",
        "OF THE FIRST SETTLERS OF CINDERMERE.",
        "THE BONES HERE HAVE BEEN MOVED. STACKED.",
        "SOMETHING IS GATHERING THE DEAD",
        "INTO AN ARMY - AND FEEDING ON THE REST.", NULL },
      "THE GLUTTON QUEEN", MT_SPIDER,
      { "THE QUEEN'S NEST FALLS SILENT.",
        "HER BROOD WAS NEVER THE ARMY. IT WAS A MEAL",
        "FOR SOMETHING THAT BURNS FAR BELOW.",
        "THE WALLS GROW WARM TO THE TOUCH.", NULL } },
    { "ACT III - THE HELLFORGE",
      { "THE ANCIENT FORGES OF THE DWARF-KINGS",
        "BURN AGAIN AFTER A THOUSAND YEARS.",
        "ROBED CULTISTS FEED THEM STOLEN SOULS,",
        "HAMMERING NOT CHAINS, BUT A KEY:",
        "A KEY TO OPEN THE ASHEN KING'S SEAL.", NULL },
      "FORGEMASTER KARRUK", MT_GOLEM,
      { "KARRUK'S HAMMER FALLS FOR THE LAST TIME.",
        "THE UNFINISHED KEY COOLS ON THE ANVIL.",
        "BUT ONE PIECE IS MISSING - TAKEN BELOW,",
        "TO THE KING'S OWN TOMB.", NULL } },
    { "ACT IV - THE FROZEN HALLS",
      { "BELOW THE FIRE LIES ICE: THE TOMB-PALACE",
        "OF THE ASHEN KING, FROZEN BY HIS LAST SPELL",
        "THE NIGHT HE SEALED THE CINDER HEART.",
        "HIS WIDOW STILL RULES THESE HALLS,",
        "AND SHE HAS BEEN WAITING FOR THE KEY.", NULL },
      "VESSA THE WINTER WIDOW", MT_CULTIST,
      { "VESSA SHATTERS LIKE A WINDOW IN WINTER.",
        "\"FOOL,\" SHE WHISPERS AS SHE FADES,",
        "\"THE KING IS NOT IN HIS TOMB.",
        "HE BECAME THE SEAL. AND THE SEAL IS FAILING.\"", NULL } },
    { "ACT V - THE VOID",
      { "PAST THE KING'S TOMB THE STONE SIMPLY ENDS.",
        "HERE THE CINDER HEART BEATS IN THE DARK,",
        "AND THE HOLLOW GOD SLEEPING INSIDE IT",
        "WEARS THE FACE OF THE ASHEN KING.",
        "WHAT IS LEFT OF HIM WILL NOT LET YOU PASS.", NULL },
      "THE HOLLOW KING", MT_GOLEM,
      { "THE HOLLOW KING KNEELS. FOR A MOMENT",
        "THE OLD KING'S EYES ARE HIS OWN AGAIN.",
        "\"KEEP THE SEAL,\" HE SAYS, AND IS ASH.",
        "THE CINDER HEART DIMS - BUT DOES NOT DIE.", NULL } },
};

const char *const epilogue[STORY_LINES] = {
    "THE ASH STOPS FALLING ON CINDERMERE.",
    "BROTHER ALDRIC RINGS THE CRYPT BELLS AT DAWN.",
    "BUT THE DEPTHS GO ON, DEEPER THAN ANY MAP,",
    "AND THE HEART BELOW STILL BEATS.",
    "THE DESCENT CONTINUES: TORMENT AWAITS.",
    NULL, NULL,
};

int story_act(int floor)
{
    if (floor < 1 || floor > ACT_COUNT * 10)
        return -1;
    return (floor - 1) / 10;
}

bool story_is_act_start(int floor)
{
    return floor >= 1 && floor <= ACT_COUNT * 10 && floor % 10 == 1;
}

bool story_is_act_boss(int floor)
{
    return floor >= 10 && floor <= ACT_COUNT * 10 && floor % 10 == 0;
}

void story_boss_name(char *out, size_t cap, int floor)
{
    static const char *const titles[MT_COUNT] = {
        "BONELORD", "NIGHTWING", "GRAVE GLUTTON", "BROODMOTHER", "PIT FIEND", "HIGH ZEALOT", "COLOSSUS",
    };
    int act = story_act(floor);
    if (act >= 0) {
        snprintf(out, cap, "%s", act_defs[act].boss_name);
        return;
    }
    snprintf(out, cap, T("TORMENTED %s"), T(titles[story_boss_type(floor) % MT_COUNT]));
}

int story_boss_type(int floor)
{
    int act = story_act(floor);
    if (act >= 0)
        return act_defs[act].boss_type;
    return (floor / 10) % MT_COUNT;
}
