/*
 * story.c - ORIGINAL story text for Ashen Depths. All names, places and
 * events are original to this game.
 *
 * Lines are at most 50 characters so they fit the 320 px screen.
 */
#include "story.h"
#include "balance.h"
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
    { "ACT VI - THE DROWNED CRYPT",
      { "BELOW THE VOID THE STAIRS BEGIN AGAIN,",
        "INTO A CRYPT YOU KNOW TOO WELL.",
        "BLACK WATER FILLS IT. THE DEAD HERE WEAR",
        "THE FACES OF HEROES WHO FELL BEFORE YOU.",
        "ALDRIC'S TORCH WHISPERS: SHE IS DOWN HERE.", NULL },
      "SISTER MAELIS THE DROWNED", MT_GHOUL,
      { "MAELIS, FIRST KEEPER OF THE SEAL, SINKS.",
        "\"I HELD IT FOR NINETY YEARS,\" SHE SIGHS.",
        "\"THEN IT BEGAN TO HOLD ME.\"",
        "BENEATH HER, ROOTS CRACK THE STONE.", NULL } },
    { "ACT VII - THE BONE ORCHARD",
      { "HERE THE DEAD ARE NOT BURIED. THEY ARE PLANTED.",
        "PALE TREES GROW FROM RIBS AND SKULLS,",
        "AND THEIR FRUIT WHISPERS IN THE OLD TONGUE.",
        "SOMETHING TENDS THIS ORCHARD,",
        "AND IT HAS BEEN WAITING FOR A NEW SEED.", NULL },
      "THE ROOT MOTHER", MT_SPIDER,
      { "THE ROOT MOTHER WITHERS TO DUST.",
        "EVERY TREE IN THE ORCHARD TURNS ITS FACE",
        "TOWARD THE HEAT RISING FROM BELOW.",
        "THE FORGES HAVE BEEN LIT AGAIN.", NULL } },
    { "ACT VIII - THE BLACK ANVIL",
      { "YOU KILLED THE FORGEMASTER. IT DID NOT MATTER.",
        "THE CULT POURED HIS ASHES INTO IRON",
        "AND HAMMERED HIM BACK TOGETHER.",
        "NOW HE FORGES THE LAST PIECE OF THE KEY",
        "ON AN ANVIL MADE FROM A FALLEN STAR.", NULL },
      "KARRUK REFORGED", MT_GOLEM,
      { "KARRUK FALLS APART, PLATE BY PLATE.",
        "THIS TIME HE THANKS YOU.",
        "\"THE KEY WAS NEVER FOR THE SEAL,\" HE RASPS.",
        "\"IT WAS FOR THE ONE WHO KEEPS IT.\"", NULL } },
    { "ACT IX - THE GLASS TOMB",
      { "THE ICE HERE IS CLEAR AS GLASS.",
        "FROZEN INSIDE ARE THE KEEPERS OF THE SEAL,",
        "ONE FOR EVERY CENTURY OF CINDERMERE.",
        "AT THE END OF THE HALL A QUEEN WAITS",
        "WHO REFUSED TO SHATTER A SECOND TIME.", NULL },
      "VESSA UNBROKEN", MT_CULTIST,
      { "VESSA KNEELS AND OFFERS YOU HER CROWN.",
        "\"HE WILL ASK YOU TO TAKE HIS PLACE.",
        "EVERY KEEPER SAYS YES. I SAID YES.\"",
        "THE FLOOR BELOW YOU BEGINS TO BEAT.", NULL } },
    { "ACT X - THE CINDER HEART",
      { "YOU STAND INSIDE THE CINDER HEART.",
        "EACH BEAT SHAKES ASH FROM THE WALLS.",
        "THE HOLLOW GOD WEARS NO STOLEN FACE NOW.",
        "IT WEARS YOURS.",
        "\"BECOME THE SEAL,\" IT OFFERS, \"AND REST.\"", NULL },
      "THE HOLLOW GOD", MT_GOLEM,
      { "THE HOLLOW GOD BREAKS LIKE A FALLING STAR.",
        "THE HEART IS STILL. THE SEAL IS WHOLE.",
        "NO KEEPER WAS NEEDED THIS TIME.",
        "YOU WERE ENOUGH.", NULL } },
    { "ACT XI - SUNKEN THRONE",
      { "BELOW THE STILLED HEART LIES A CITY",
        "OLDER THAN CINDERMERE, BUILT BY THE STRANGER",
        "WHO CHOSE TO STAY. ITS PEOPLE NEVER DIED.",
        "THEY WAITED, KNEELING, FOR A NEW RULER.",
        "THEY HAVE DECIDED IT SHOULD BE YOU.", NULL },
      "THE PALE REGENT", MT_CULTIST,
      { "THE PALE REGENT LAUGHS AS HE FALLS.",
        "\"THE HEART WAS NEVER A GOD. IT WAS A DOOR.",
        "AND SOMETHING ON THE OTHER SIDE",
        "HAS STARTED TO KNOCK.\"", NULL } },
    { "ACT XII - THE ASHEN SEA",
      { "THE STONE GIVES WAY TO A SEA OF ASH,",
        "GREY WAVES WITHOUT A SOUND.",
        "BOATS OF BONE DRIFT PAST, EMPTY.",
        "WITH EVERY STEP THE KNOCKING GROWS LOUDER,",
        "AND THE TORCH IN YOUR HAND BURNS LOW.", NULL },
      "THE FERRYMAN OF ASH", MT_SKELETON,
      { "THE FERRYMAN SINKS WITH HIS BOAT.",
        "HIS LAST COIN IS STAMPED WITH A GEAR,",
        "THE MARK OF THE DWARF-KINGS.",
        "THEY CAME THIS WAY LONG BEFORE YOU.", NULL } },
    { "ACT XIII - CLOCKWORK DEEP",
      { "VAST ENGINES TURN IN THE DARK,",
        "BUILT BY DWARVES WHO NEVER WENT HOME.",
        "EVERY GEAR PUSHES AGAINST THE DOOR,",
        "HOLDING IT SHUT. SOME HAVE STOPPED.",
        "THE ORACLE THAT RUNS THEM IS AFRAID.", NULL },
      "THE IRON ORACLE", MT_GOLEM,
      { "THE ORACLE'S LAST GEAR GRINDS TO A HALT.",
        "\"WE DID NOT BUILD THE DOOR TO KEEP IT CLOSED.",
        "WE BUILT IT TO KEEP SOMETHING OUT:",
        "THE NAMELESS TIDE.\"", NULL } },
    { "ACT XIV - NAMELESS TIDE",
      { "HERE THE DARK IS ALIVE. IT EATS THE LIGHT,",
        "THE SOUND, THE NAMES OF THINGS.",
        "YOUR TORCH IS THE ONLY FIRE LEFT,",
        "AND THE TIDE LEANS TOWARD IT, HUNGRY.",
        "A HERALD WALKS AHEAD OF THE WAVE.", NULL },
      "THE TIDE'S HERALD", MT_BAT,
      { "THE HERALD DISSOLVES INTO THE DARK.",
        "THE TIDE PULLS BACK, JUST FOR A MOMENT,",
        "AND THERE IT IS: THE DOOR ITSELF,",
        "STANDING OPEN.", NULL } },
    { "ACT XV - THE OTHER SIDE",
      { "BEYOND THE DOOR LIES CINDERMERE,",
        "BUT HERE THE ASH NEVER FELL.",
        "THE BELLS RING. THE MARKET IS FULL.",
        "AND IN THE CRYPT A KEEPER WAITS",
        "WHO WEARS YOUR FACE, AND SAID YES.", NULL },
      "THE KEEPER WHO STAYED", MT_CULTIST,
      { "YOUR OTHER SELF SMILES AND STEPS ASIDE.",
        "\"GO BACK. CLOSE THE DOOR. KEEP WALKING.\"",
        "THE DOOR SHUTS BEHIND YOU WITH A SIGH.",
        "THE DEPTHS NEVER END. NEITHER DO YOU.", NULL } },
};

const char *const epilogue[STORY_LINES] = {
    "THE ASH STOPS FALLING ON CINDERMERE.",
    "BROTHER ALDRIC RINGS THE CRYPT BELLS AT DAWN.",
    "BUT THE DEPTHS GO ON, DEEPER THAN ANY MAP,",
    "AND THE HEART BELOW STILL BEATS.",
    "THE DESCENT CONTINUES: TORMENT AWAITS.",
    NULL, NULL,
};

const char *const finale[STORY_LINES] = {
    "ALDRIC'S TORCH BURNS WHITE, THEN GOES OUT.",
    "HIS VOICE COMES ONE LAST TIME:",
    "\"I WAS NEVER JUST A MONK. I WAS A KEEPER.",
    "NOW THE DEPTHS ARE YOURS TO WALK.\"",
    "THE TORMENT GOES ON. SO DO YOU.",
    NULL, NULL,
};

/* Acts I-X take ten floors each; the chapters of the deep (XI-XV) take a
 * Torment tier of fifty floors each, from floor 101 to 350. */
int story_act(int floor)
{
    if (floor < 1 || floor > DEEP_LAST_FLOOR)
        return -1;
    if (floor <= CAMPAIGN_FLOORS)
        return (floor - 1) / 10;
    return 10 + (floor - CAMPAIGN_FLOORS - 1) / TORMENT_TIER_FLOORS;
}

bool story_is_act_start(int floor)
{
    if (floor < 1 || floor > DEEP_LAST_FLOOR)
        return false;
    return floor <= CAMPAIGN_FLOORS ? floor % 10 == 1 : (floor - CAMPAIGN_FLOORS - 1) % TORMENT_TIER_FLOORS == 0;
}

bool story_is_act_boss(int floor)
{
    if (floor < 10 || floor > DEEP_LAST_FLOOR)
        return false;
    return floor <= CAMPAIGN_FLOORS ? floor % 10 == 0 : (floor - CAMPAIGN_FLOORS) % TORMENT_TIER_FLOORS == 0;
}

/* story_seen bits keep the v4/v5 layout (0-4 intros, 5-9 victories,
 * 10 epilogue), then 11-15 intros VI-X, 16-20 victories VI-X, 21 finale,
 * 22-26 intros XI-XV and 27-31 victories XI-XV. */
static int seen_bit(int ev)
{
    int a;
    if (ev == STORY_FINALE)   return 21;
    if (ev == STORY_EPILOGUE) return 10;
    if (ev >= STORY_VICTORY) {
        a = ev - STORY_VICTORY;
        return a < CAMPAIGN_ACTS ? 5 + a : a < 10 ? 16 + a - CAMPAIGN_ACTS : 27 + a - 10;
    }
    return ev < CAMPAIGN_ACTS ? ev : ev < 10 ? 11 + ev - CAMPAIGN_ACTS : 22 + ev - 10;
}

bool story_event_seen(const Profile *p, int ev)
{
    if (ev >= STORY_LORE)
        return ev < STORY_EVENT_END && ((p->lore >> (ev - STORY_LORE)) & 1u);
    return (p->story_seen >> seen_bit(ev)) & 1u;
}

void story_mark_seen(Profile *p, int ev)
{
    if (ev >= STORY_LORE) {
        if (ev < STORY_EVENT_END)
            p->lore |= (uint64_t)1 << (ev - STORY_LORE);
        return;
    }
    p->story_seen |= (uint32_t)1 << seen_bit(ev);
}

int story_lore_found(const Profile *p)
{
    int i, n = 0;
    for (i = 0; i < LORE_COUNT; i++)
        n += (int)((p->lore >> i) & 1u);
    return n;
}

/* The first sixteen pages lie in the campaign; the rest one every ten
 * floors from floor 100 down. */
int story_lore_floor(int page)
{
    return page < 16 ? 0 : CAMPAIGN_FLOORS + (page - 16) * 10;
}

int story_unread_lore(const Profile *p, uint32_t roll)
{
    int deepest = MAX(p->best_floor, p->best_floor_ever), left = 0, i, k;
    for (i = 0; i < LORE_COUNT; i++)
        left += !((p->lore >> i) & 1u) && story_lore_floor(i) <= deepest;
    if (left <= 0)
        return -1;
    k = (int)(roll % (uint32_t)left);
    for (i = 0; i < LORE_COUNT; i++)
        if (!((p->lore >> i) & 1u) && story_lore_floor(i) <= deepest && k-- == 0)
            return i;
    return -1;
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
