/*
 * bark.c - ORIGINAL remarks of Brother Aldric (A) and the hero (H).
 */
#include "bark.h"

#define A SPK_ALDRIC
#define H SPK_HERO

const BarkLine bark_lines[BK_COUNT][BARK_VARIANTS] = {
    [BK_RECORD]      = { { A, "DEEPER THAN ANY KEEPER HAS GONE." }, { A, "THE TORCH BURNS COLDER HERE. GO ON." },
                         { H, "NEW GROUND. NOBODY HAS MAPPED THIS." } },
    [BK_ANCESTRAL]   = { { H, "THIS ONE REMEMBERS OLDER WARS." }, { H, "ANCESTRAL. IT HUMS IN MY HAND." },
                         { A, "FORGED BEFORE CINDERMERE HAD A NAME." } },
    [BK_UNIQUE]      = { { H, "NOW THAT IS A FIND." }, { H, "I KNOW THIS ONE FROM THE OLD SONGS." },
                         { A, "KEEP THAT ONE CLOSE." } },
    [BK_MYTHIC]      = { { A, "BY THE BELLS... A MYTHIC." }, { A, "FEW KEEPERS EVER SAW ONE OF THOSE." },
                         { H, "I AM NEVER LETTING GO OF THIS." } },
    [BK_LOW_HP]      = { { H, "NOT YET... NOT HERE." }, { H, "TOO CLOSE. FAR TOO CLOSE." },
                         { A, "BREATHE. THE TORCH STILL BURNS." } },
    [BK_DEATH]       = { { A, "GET UP. THE DEPTHS WILL WAIT." }, { A, "EVERY KEEPER FELL ONCE. RISE." },
                         { A, "THE TORCH BRINGS YOU BACK. AGAIN." } },
    [BK_BOSS_DOWN]   = { { H, "ONE MORE FOR THE JOURNAL." }, { H, "THAT ONE HAD A NAME. NOW IT HAS AN END." },
                         { A, "WELL DONE. THE SEAL HOLDS A LITTLE LONGER." } },
    [BK_GOBLIN]      = { { H, "A GOBLIN! AND A FAT SACK!" }, { H, "YOU ARE NOT GETTING AWAY." },
                         { A, "QUICKLY, BEFORE IT OPENS A PORTAL!" } },
    [BK_GOBLIN_KILL] = { { H, "GOLD EVERYWHERE!" }, { H, "THE SACK IS MINE NOW." },
                         { H, "THAT WAS WORTH THE CHASE." } },
    [BK_GOBLIN_GONE] = { { H, "IT GOT AWAY... WITH MY GOLD." }, { H, "NEXT TIME, LITTLE THIEF." },
                         { A, "TOO SLOW. IT WILL BE BACK." } },
    [BK_SHRINE]      = { { A, "AN OLD SHRINE. BORROW ITS FIRE." }, { A, "THE GODS STILL LISTEN DOWN HERE." },
                         { H, "I FEEL IT. LET THEM COME." } },
    [BK_AMBUSH]      = { { H, "IT IS A TRAP!" }, { H, "THEY WERE WAITING FOR ME." }, { A, "BEHIND YOU!" } },
    [BK_CHEST]       = { { A, "A CURSED CHEST. ITS GUARDS WAKE AT A TOUCH." },
                         { H, "SOMETHING INSIDE IS SCRATCHING." }, { H, "ONLY ONE WAY TO OPEN IT." } },
    [BK_LORE]        = { { H, "SOMEONE CAME THIS FAR BEFORE ME." }, { H, "A NOTE, STILL IN THEIR HAND." },
                         { A, "READ IT. THE DEAD REMEMBER." } },
    [BK_CHAMPION]    = { { H, "THAT ONE IS STRONGER THAN THE REST." }, { H, "A CHAMPION. GOOD." },
                         { A, "CAREFUL. THE HEART HAS TOUCHED THAT ONE." } },
    [BK_BOUNTY]      = { { A, "BOUNTY DONE. THE TOWN WILL PAY." }, { A, "ANOTHER NOTICE OFF THE BOARD." },
                         { H, "THEY WILL SING ABOUT THIS. MAYBE." } },
    [BK_ACHIEVE]     = { { A, "YOUR NAME GROWS IN CINDERMERE." }, { A, "RENOWN. THEY SPEAK OF YOU NOW." },
                         { A, "THE OLD KEEPERS WOULD BE PROUD." } },
    [BK_LEVEL]       = { { H, "STRONGER." }, { H, "I CAN FEEL THE ASH IN MY BLOOD." },
                         { H, "THE DEPTHS ARE TEACHING ME." } },
    [BK_IDLE]        = { { A, "THE BELLS RANG TWICE TODAY. A GOOD SIGN." }, { H, "HOW DEEP DOES THIS GO?" },
                         { H, "I CAN HEAR THE HEART FROM HERE." } },
};

#undef A
#undef H

#define BARK_TICKS   (4 * TICK_HZ)
#define BARK_GAP     (15 * TICK_HZ)     /* between ordinary remarks */
#define BARK_REPEAT  (90 * TICK_HZ)     /* before the same kind again */
#define BARK_IDLE    (150 * TICK_HZ)

/* Rare, big moments speak over the general gap. */
static bool urgent(BarkKind k)
{
    return k == BK_MYTHIC || k == BK_DEATH || k == BK_GOBLIN || k == BK_BOSS_DOWN || k == BK_AMBUSH;
}

void bark(BarkState *b, BarkKind kind, uint32_t salt)
{
    if (kind >= BK_COUNT || b->cd[kind] > 0 || (b->cd_all > 0 && !urgent(kind)))
        return;
    b->line = &bark_lines[kind][(salt * 2654435761u >> 16) % BARK_VARIANTS];
    b->t = BARK_TICKS;
    b->cd[kind] = BARK_REPEAT;
    b->cd_all = BARK_GAP;
    b->idle_t = BARK_IDLE;
}

void bark_tick(BarkState *b, uint32_t salt)
{
    int k;
    if (b->t > 0)
        b->t--;
    if (b->cd_all > 0)
        b->cd_all--;
    for (k = 0; k < BK_COUNT; k++)
        if (b->cd[k] > 0)
            b->cd[k]--;
    if (b->idle_t <= 0)
        b->idle_t = BARK_IDLE;
    else if (--b->idle_t == 0)
        bark(b, BK_IDLE, salt);
}
