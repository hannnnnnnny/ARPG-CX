/*
 * art.c - ORIGINAL pixel art for Ashen Depths (character grids).
 *
 * Palette letters (see gfx/sprites.c):
 *   k outline  w white   s silver  S steel   d dark    r red    R maroon
 *   y gold     Y orange  n brown   N dk brown g green  G dk green
 *   p purple   P dk purple b blue  B navy    o bone    O bone shade
 *   e eye glow c cyan    f skin    x rarity colour (loot icons)
 * '.' is transparent. tests/test_art.c checks every row's width.
 */
#include "art.h"

const char *const art_mon[MON_ART_COUNT][2][16] = {
    /* Skeleton */
    {{ "................", ".....kkkkk......", "....koooook.....", "....kokokok.....",
       "....koeoeok.....", "....kooooOk.....", ".....kokok......", "...kkoooookk....",
       "..kok.kOk.kok...", "..kok.kok.kok...", "...k..kOk..k....", "......kok.......",
       ".....kOkOk......", ".....kok.kok....", "....kok...kok...", "....kk.....kk..." },
     { "................", ".....kkkkk......", "....koooook.....", "....kokokok.....",
       "....koeoeok.....", "....kooooOk.....", ".....kokok......", "...kkoooookk....",
       "..kok.kOk.kok...", "..kok.kok.kok...", "...k..kOk..k....", "......kok.......",
       ".....kOkOk......", ".....kokkok.....", ".....kok.kok....", ".....kk..kk....." }},
    /* Bat */
    {{ "................", "................", "................", "kk...........kk.",
       "kPk...kkk...kPk.", "kPPk.kpppk.kPPk.", ".kPPkpepepkPPk..", "..kPkpppppkPk...",
       "...kkkpppkkk....", ".....kpkpk......", "......k.k.......", "................",
       "................", "................", "................", "................" },
     { "................", "................", "................", "................",
       "......kkk.......", ".....kpppk......", "..kkkpepepkkk...", ".kPPkpppppkPPk..",
       "kPPk.kpppk.kPPk.", "kPk...kpk...kPk.", "kk.....k.....kk.", "................",
       "................", "................", "................", "................" }},
    /* Ghoul */
    {{ "................", ".....kkkk.......", "....kggggk......", "....kgekgk......",
       "....kggggk......", ".....kGGk.......", "...kkggggkk.....", "..kgkgGGgkgk....",
       ".kgk.kggk.kgk...", ".kk..kGGk..kk...", ".....kggk.......", ".....kGGk.......",
       "....kgk.kgk.....", "....kgk.kgk.....", "...kNNk.kNNk....", "...kkk...kkk...." },
     { "................", ".....kkkk.......", "....kggggk......", "....kgekgk......",
       "....kggggk......", ".....kGGk.......", "...kkggggkk.....", "..kgkgGGgkgk....",
       "kgk..kggk..kgk..", "kk...kGGk...kk..", ".....kggk.......", ".....kGGk.......",
       "....kgkkgk......", ".....kgkgk......", "....kNNkNNk.....", "....kkk.kkk....." }},
    /* Spider */
    {{ "................", "................", "................", "................",
       "..k..k....k..k..", "...k.k.kk.k.k...", "....kkkddkkk....", "..kkkdddddkkk...",
       ".k..kdedeedk..k.", "k..kkdddddkk..k.", "...k.kdddk.k....", "..k..k.k.k..k...",
       ".k..k.....k..k..", "................", "................", "................" },
     { "................", "................", "................", "................",
       "...k.k....k.k...", "..k..k.kk.k..k..", "....kkkddkkk....", "..kkkdddddkkk...",
       ".k..kdedeedk..k.", "k..kkdddddkk..k.", "...k.kdddk.k....", "...k.k.k.k.k....",
       "..k.k.....k.k...", "................", "................", "................" }},
    /* Imp */
    {{ "................", "....k......k....", "....yk....ky....", ".....kkkkkk.....",
       "....krrrrrrk....", "....krykkyrk....", "....krrrrrrk....", ".....kRrrRk.....",
       "...kkrrrrrrkk...", "..krk.rRRr.krk..", "..kk..rrrr..kk..", "......kRRk......",
       ".....kRk.kRk....", ".....kk...kk....", "................", "................" },
     { "....k......k....", "....yk....ky....", ".....kkkkkk.....", "....krrrrrrk....",
       "....krykkyrk....", "....krrrrrrk....", ".....kRrrRk.....", "...kkrrrrrrkk...",
       "..krk.rRRr.krk..", "..kk..rrrr..kk..", "......kRRk......", ".....kRkkRk.....",
       ".....kk..kk.....", "................", "................", "................" }},
    /* Cultist */
    {{ "................", "......kkkk......", ".....kPPPPk.....", "....kPPkkPPk....",
       "....kPkeekPk....", "....kPPkkPPk....", "...kPPPPPPPPk...", "..kPpPPPPPPpPk..",
       "..kPkpPPPPpkPk..", "..kk.pPPPPp.kk..", "....kpPPPPpk....", "....kPPPPPPk....",
       "...kPPPPPPPPk...", "...kPpPPPPpPk...", "...kkkkkkkkkk...", "................" },
     { "................", "......kkkk......", ".....kPPPPk.....", "....kPPkkPPk....",
       "....kPkyykPk....", "....kPPkkPPk....", "...kPPPPPPPPk...", "..kPpPPPPPPpPk..",
       "..kPkpPPPPpkPk..", "..kk.pPPPPp.kk..", "....kpPPPPpk....", "....kPPPPPPk....",
       "...kPPPPPPPPk...", "...kpPPPPPPpk...", "...kkkkkkkkkk...", "................" }},
    /* Golem */
    {{ "................", "....kkkkkkkk....", "...kSSSSSSSSk...", "...kSdccdSSdk...",
       "...kSSSSSSSSk...", "..kkkSSSSSSkkk..", ".kSSkSscsSSkSSk.", ".kSdkSSSSSSkdSk.",
       ".kSSkSdddSSkSSk.", ".kkkkSSSSSSkkkk.", "....kSSkkSSk....", "....kSSk.kSSk...",
       "...kSSSk.kSSSk..", "...kkkkk.kkkkk..", "................", "................" },
     { "................", "................", "....kkkkkkkk....", "...kSSSSSSSSk...",
       "...kSdccdSSdk...", "...kSSSSSSSSk...", "..kkkSSSSSSkkk..", ".kSSkSscsSSkSSk.",
       ".kSdkSSSSSSkdSk.", ".kSSkSdddSSkSSk.", ".kkkkSSSSSSkkkk.", "....kSSkkSSk....",
       "...kSSSk.kSSSk..", "...kkkkk.kkkkk..", "................", "................" }},
    /* Fleshrender (the butcher): apron, cleaver, burning eyes */
    {{ "................", "......kkkk......", ".....kffffk.....", ".....kfefek.....",
       ".....kRffRk.....", "...kkkkffkkkk...", "..kffkwwwwkffk..", ".kffkwwrwwwkffk.",
       ".kfk.kwwrrwk.kfk", ".kfk.kwrwwwk.kSk", "..kk.kwwwwk.kSSk", ".....kNNNNk.kSSk",
       ".....kNk.kNk.ks.", ".....kNk.kNk..k.", "....kNNk.kNNk...", "....kkk...kkk..." },
     { "................", "......kkkk...kk.", ".....kffffk.kSSk", ".....kfefek.kSSk",
       ".....kRffRk.kSk.", "...kkkkffkkkkfk.", "..kffkwwwwkffk..", ".kffkwwrwwwkk...",
       ".kfk.kwwrrwk....", ".kfk.kwrwwwk....", "..kk.kwwwwk.....", ".....kNNNNk.....",
       "....kNk..kNk....", "....kNk...kNk...", "...kNNk...kNNk..", "...kkk.....kkk.." }},
};

