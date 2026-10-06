/*
 * art_hero.c - ORIGINAL hero, companion and loot icon art.
 *
 * Hero parts use ROLE letters, coloured per hero when the sprite is built
 * from the chosen look and the equipped gear (see sprites.c):
 *   1 skin  2 skin shade  3 hair  4 hair shade  5 eyes
 *   a armour  A armour shade  t armour trim     (chest piece)
 *   c cloth  C cloth shade                      (look: cloth colour)
 *   g gloves  l pants  L pants shade  b boots   (those slots)
 *   m helm  M helm shade  h helm trim           (helm)
 * Fixed palette letters (k outline, w white, n brown...) work as in art.c.
 * The hero is 16x20: head rows 0-6, body rows 7-13, legs rows 14-19.
 */
#include "art.h"

const char *const art_hero_head[7] = {
    ".....kkkkkk.....",
    "....k111111k....",
    "....k111111k....",
    "....k151151k....",
    "....k111111k....",
    ".....k1221k.....",
    "......k22k......",
};

/* Hair styles (rows 0-6): short, long, mohawk, bald, braids, ponytail. */
const char *const art_hair[HAIR_STYLES][7] = {
    { ".....kkkkkk.....", "....k333333k....", "....k3....3k....", "................",
      "................", "................", "................" },
    { ".....kkkkkk.....", "....k333333k....", "...k33....33k...", "...k3k....k3k...",
      "...k3k....k3k...", "...k33k..k33k...", "....kk....kk...." },
    { "......k33k......", "....k113311k....", "................", "................",
      "................", "................", "................" },
    { ".....kkkkkk.....", "....k111111k....", "................", "................",
      "................", "................", "................" },
    { ".....kkkkkk.....", "....k333333k....", "...k3......3k...", "...k3k....k3k...",
      "...k4k....k4k...", "...k3k....k3k...", "...kk......kk..." },
    { ".....kkkkkk.....", "....k333333kk...", "....k3....3k3k..", "...........k3k..",
      "...........k4k..", "............kk..", "................" },
};

/* Faces (rows 0-6): none, beard, scar, war paint, tattoo. */
const char *const art_face[FACE_STYLES][7] = {
    { "................", "................", "................", "................",
      "................", "................", "................" },
    { "................", "................", "................", "................",
      "....k3....3k....", ".....k3333k.....", "......k33k......" },
    { "................", "................", "........R.......", "........R.......",
      ".......R........", "................", "................" },
    { "................", "................", "................", "................",
      "....kr1111rk....", "................", "................" },
    { "................", "................", ".....G....G.....", "................",
      "................", "................", "................" },
};

/* Helms by base type (rows 0-6): hood, skullcap, horned helm, great helm. */
const char *const art_helm[4][7] = {
    { ".....kkkkkk.....", "....kmmmmmmk....", "...kmM....Mmk...", "...km......mk...",
      "...km......mk...", "...kmk....kmk...", "....kk....kk...." },
    { ".....kkkkkk.....", "....kmmmmmmk....", "....khhhhhhk....", "................",
      "................", "................", "................" },
    { ".kk..kkkkkk..kk.", ".kwkkmmmmmmkkwk.", "..kkmMmmmmMmkk..", "....kmk..kmk....",
      "................", "................", "................" },
    { ".....kkkkkk.....", "....kmmmmmmk....", "....kmMmmMmk....", "....km5kk5mk....",
      "....kmmhhmmk....", ".....kmmmmk.....", "................" },
};

/* Bodies (rows 7-13), one silhouette per class. */
const char *const art_body[6][7] = {
    /* Barbarian: bare arms, harness, belt, kilt */
    { "..kk11kaak11kk..", ".k111kaAAak111k.", ".k11kaaAAaak11k.", ".kgg.kaaaak.ggk.",
      ".kggkttttttkggk.", "..kk.kcCCck.kk..", ".....kcccck....." },
    /* Sorcerer: mantle over a robe */
    { "....kcaaaack....", "...kccatacck....", "..k1kcctcck1k...", "..kgkcccccckgk..",
      "...kkctttcckk...", "....kcccccck....", "....kcCcCcck...." },
    /* Rogue: leather jerkin and a short cape */
    { "...kkaaaaaakk...", "..kcaAattAAack..", "..kc1kaAAak1ck..", "..kcgkaaaakgck..",
      "..kc.kttttk.ck..", "..kk.kaAAak.kk..", ".....kaaaak....." },
    /* Necromancer: bone pauldrons over a dark robe */
    { "...kwwkcckwwk...", "..kowkcaackwok..", "..k1kcctcckk1k..", "..kgkcccccckgk..",
      "...kkcttttckk...", "....kcccccck....", "....kcCcCcck...." },
    /* Druid: fur mantle, broad chest */
    { "..kknnnnnnnnkk..", ".knNnkaaaaknNnk.", ".k11kaAAAAak11k.", ".kggkaaaaaakggk.",
      "..kkkttttttkkk..", "....kcCccCck....", "....kccccccck..." },
    /* Spiritborn: wraps, light armour and a sash */
    { "...kk1aaaa1kk...", "..k11kaAAak11k..", "..k1kaattaak1k..", "..kgkaaAAaakgk..",
      "..kk.kcttck.kk..", ".....kcCCck.....", ".....kccccck...." },
};

/* Shoulder plates for heavier chest pieces (rows 7-8): chain/scale, plate. */
const char *const art_pauldrons[2][2] = {
    { "..kAAk....kAAk..", ".kAaAk....kAaAk." },
    { "..ktAk....kAtk..", ".kAaAk....kAaAk." },
};

/* Legs (rows 14-19): [0 trousers, 1 robe][stand, step A, step B] */
const char *const art_legs[2][3][6] = {
    { { ".....kLllLk.....", ".....kllkllk....", ".....kllkllk....", ".....kllkllk....",
        "....kbbk.kbbk...", "....kkkk.kkkk..." },
      { ".....kLllLk.....", ".....kllkllk....", "....kllk.kllk...", "...kbbk...kllk..",
        "...kkkk...kbbk..", "..........kkkk.." },
      { ".....kLllLk.....", ".....kllkllk....", "....kllk.kllk...", "..kllk...kbbk...",
        "..kbbk...kkkk...", "..kkkk.........." } },
    { { "....kcCcCck.....", "....kcccccck....", "...kcccCccccck..", "...kcccccccck...",
        "...kkkkkkkkkk...", "....kbk..kbk...." },
      { "....kcCcCck.....", "....kcccccck....", "....kcccCcccck..", "....kccccccccck.",
        "....kkkkkkkkkkk.", ".....kbk...kbk.." },
      { "....kcCcCck.....", "....kcccccck....", "..kcccCcccck....", ".kccccccccck....",
        ".kkkkkkkkkkk....", "..kbk...kbk....." } },
};

/* Spirit wolf (druid companion), two frames. */
const char *const art_wolf[2][16] = {
    { "................", "................", "................", "................",
      "................", "..kk............", ".kwskk..........", ".kwwwskkkkkkk...",
      "..kkwssssssssk..", "....ksssssssssk.", "....kssSSsssSsk.", "....ksk.ksk.ksk.",
      "....kk..kk..kk..", "................", "................", "................" },
    { "................", "................", "................", "................",
      "................", "..kk............", ".kwskk..........", ".kwwwskkkkkkk...",
      "..kkwssssssssk..", "....ksssssssssk.", "....kssSSsssSsk.", ".....ksk.ksk.ksk",
      ".....kk...kk.kk.", "................", "................", "................" },
};

/* 8x8 loot icons per slot, 'x' = rarity colour. */
const char *const art_icons[SLOT_ICONS][8] = {
    { "......kk", ".....kxk", "....kxk.", "k..kxk..", ".kkxk...", "..kk....", ".kyk....", "kk......" },
    { "..kkkk..", ".kxxxxk.", "kxxyyxxk", "kxyxxyxk", "kxxyyxxk", ".kxxxxk.", "..kxxk..", "...kk..." },
    { "..kkkk..", ".kxxxxk.", "kxxxxxxk", "kxkkkkxk", "kxk..kxk", "kxk..kxk", ".k....k.", "........" },
    { ".kk..kk.", "kxxkkxxk", "kxxxxxxk", ".kxxxxk.", ".kxxxxk.", ".kxxxxk.", ".kkkkkk.", "........" },
    { "........", ".kkk....", "kxxxk...", "kxxxxkk.", "kxxxxxxk", ".kxxxxxk", "..kkkkk.", "........" },
    { ".kkkkkk.", ".kxxxxk.", ".kxkkxk.", ".kxk.kxk", ".kxk.kxk", ".kxk.kxk", ".kkk.kkk", "........" },
    { "........", ".kkk....", ".kxk....", ".kxk....", ".kxxkkk.", ".kxxxxxk", ".kkkkkkk", "........" },
    { "kk....kk", ".k....k.", "..k..k..", "...kk...", "..kxxk..", ".kxxxxk.", "..kxxk..", "...kk..." },
    { "........", "..kkkk..", ".kxkkxk.", "kxk..kxk", "kxk..kxk", ".kxkkxk.", "..kkkk..", "........" },
};
