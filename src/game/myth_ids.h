/* myth_ids.h - mythic power ids (see mythic.h), shared with the build model. */
#ifndef AD_MYTH_IDS_H
#define AD_MYTH_IDS_H

typedef enum {
    MY_NONE,
    MY_STARFALL,   /* stars rain on the pack every 1.2 s            (helm)    */
    MY_SHATTER,    /* crits burst into star shards                  (ring)    */
    MY_CRESCENT,   /* every 5th hit looses a piercing crescent wave (weapon)  */
    MY_REAPER,     /* hits reap weakened foes outright              (weapon)  */
    MY_VOID,       /* a singularity pulls a pack in, then bursts    (offhand) */
    MY_TIMESTOP,   /* time stops: foes frozen and take more damage  (offhand) */
    MY_EYES,       /* hits loose eye beams at other foes            (helm)    */
    MY_UNDYING,    /* [x] life; once in a while death is refused    (chest)   */
    MY_DRAGON,     /* a ring of dragonfire every second             (chest)   */
    MY_THUNDER,    /* crits call chain lightning                    (gloves)  */
    MY_BLADES,     /* spectral blades circle the hero               (gloves)  */
    MY_MAGMA,      /* every step leaves burning ground              (pants)   */
    MY_SLAUGHTER,  /* kill streaks stack [x] damage                 (pants)   */
    MY_BLINK,      /* teleports onto far targets with a burst       (boots)   */
    MY_DIVER,      /* cleared floors may drop the hero deeper       (boots)   */
    MY_RUIN,       /* the slain explode                             (amulet)  */
    MY_ONENAME,    /* sheer power: [x] damage                       (amulet)  */
    MY_DEVOUR,     /* drags the pack to the hero and feeds on it    (ring)    */
    MY_ORBIT,      /* skills cost nothing, cooldowns shrink         (ring)    */
    MY_COUNT
} MythicPower;

#endif
