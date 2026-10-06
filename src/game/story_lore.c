/*
 * story_lore.c - the sixteen lost pages (ORIGINAL text), found beside
 * fallen adventurers on the floors and kept in the journal.
 */
#include "story.h"

const LorePage lore_pages[LORE_COUNT] = {
    { "A MINER'S NOTE", { "WE DUG TOO DEEP UNDER THE CHAPEL.",
                          "THE ROCK WAS WARM, AND IT WAS BREATHING.", NULL } },
    { "THE BELL RINGER", { "ALDRIC RINGS THE BELLS TO WAKE THE TOWN.",
                           "NO. HE RINGS THEM TO KEEP SOMETHING ASLEEP.", NULL } },
    { "A CULTIST'S PRAYER", { "HOLLOW ONE, WE GIVE YOU OUR NAMES.",
                              "GIVE US BACK THE FIRE.", NULL } },
    { "THE KING'S DECREE", { "LET NO KING AFTER ME BE BURIED.",
                             "THE SEAL NEEDS A LIVING HEART.", NULL } },
    { "A CHILD'S DRAWING", { "A LITTLE FIGURE WITH A TORCH,",
                             "AND BELOW IT A HUGE EYE, COLOURED RED.", NULL } },
    { "DWARVEN LEDGER", { "ONE THOUSAND KEYS FORGED. NONE FIT.",
                          "THE LOCK CHANGES WHEN YOU LOOK AT IT.", NULL } },
    { "VESSA'S LETTER", { "MY LOVE, THE ICE IS KIND. IT ASKS NOTHING.",
                          "COME HOME, OR LET ME COME DOWN.", NULL } },
    { "A HUNTER'S WARNING", { "IF YOU SEE A GOBLIN WITH A SACK, CHASE IT.",
                              "IF THE SACK CHASES YOU, RUN.", NULL } },
    { "THE SECOND KEEPER", { "I HAVE FORGOTTEN MY MOTHER'S FACE.",
                             "I REMEMBER EVERY CRACK IN THE SEAL.", NULL } },
    { "SHRINE INSCRIPTION", { "WHO KNEELS HERE IS LENT A LITTLE FIRE.",
                              "ALL FIRE IS RETURNED IN THE END.", NULL } },
    { "MAELIS' DIARY", { "THE WATER RISES ONE STEP EACH YEAR.",
                         "WHEN IT REACHES THE BELLS, IT IS YOUR TURN.", NULL } },
    { "A MERCHANT'S RECEIPT", { "ONE TORCH, BLESSED. PAID IN FULL BY A MONK.",
                                "HE INSISTED IT NEVER BE PUT OUT.", NULL } },
    { "THE ROOT MOTHER'S SONG", { "SLEEP, LITTLE BONES, AND GROW.",
                                  "THE HEART BELOW IS HUNGRY TOO.", NULL } },
    { "KARRUK'S LAST ORDER", { "BREAK EVERY HAMMER IF I FALL.",
                               "THEY DID NOT. THEY NEVER DO.", NULL } },
    { "THE FIRST PAGE", { "BEFORE CINDERMERE THERE WAS ONLY THE HEART,",
                          "AND ONE STRANGER WHO CHOSE TO STAY.", NULL } },
    { "AN UNSENT LETTER", { "IF YOU READ THIS, YOU WENT DEEPER THAN I DID.",
                            "TELL THEM I TRIED.", NULL } },
};
