/*
 * art.c - ORIGINAL pixel art for Ashen Depths (character grids).
 *
 * Palette letters (see gfx/sprites.c):
 *   k outline  w white   s silver  S steel   d dark    r red    R maroon
 *   y gold     Y orange  n brown   N dk brown g green  G dk green
 *   p purple   P dk purple b blue  B navy    o bone    O bone shade
 *   e eye glow c cyan    f skin    x rarity colour (loot icons)
 * Shading ramps, lit from the top left (light -> dark):
 *   bone H o O q   ghoul u g G v   flesh f F E   imp j r R
 *   robe I p P     stone T s S d   spider body d D
 * '.' is transparent. tests/test_art.c checks every row's width.
 */
#include "art.h"

const char *const art_mon[MON_ART_COUNT][2][16] = {
    /* Skeleton */
    {{ "................", ".....kkkkk......", "....kHHooOk.....", "....kHkokqk.....",
       "....koeoeqk.....", "....kooOOqk.....", ".....kOkqk......", "...kkHoooOkk....",
       "..kHk.kOk.kOk...", "..kok.kqk.kqk...", "...k..kOk..k....", "......kqk.......",
       ".....kHkOk......", ".....kok.kOk....", "....kok...kqk...", "....kk.....kk..." },
     { "................", ".....kkkkk......", "....kHHooOk.....", "....kHkokqk.....",
       "....koeoeqk.....", "....kooOOqk.....", ".....kOkqk......", "...kkHoooOkk....",
       "..kHk.kOk.kOk...", "..kok.kqk.kqk...", "...k..kOk..k....", "......kqk.......",
       ".....kHkOk......", ".....kokkOk.....", ".....kok.kqk....", ".....kk..kk....." }},
    /* Bat */
    {{ "................", "................", "................", "kk...........kk.",
       "kIk...kkk...kpk.", "kpPk.kIppk.kPPk.", ".kpPkIepepkPPk..", "..kPkppppPkPk...",
       "...kkkpPPkkk....", ".....kPkPk......", "......k.k.......", "................",
       "................", "................", "................", "................" },
     { "................", "................", "................", "................",
       "......kkk.......", ".....kIppk......", "..kkkIepepkkk...", ".kIpkppppPkPPk..",
       "kpPk.kpPPk.kPPk.", "kPk...kPk...kPk.", "kk.....k.....kk.", "................",
       "................", "................", "................", "................" }},
    /* Ghoul */
    {{ "................", ".....kkkk.......", "....kuggGk......", "....kuekGk......",
       "....kggGvk......", ".....kGvk.......", "...kkuggGkk.....", "..kukgGGvkGk....",
       ".kuk.kgGk.kGk...", ".kk..kGvk..kk...", ".....kgGk.......", ".....kGvk.......",
       "....kgk.kGk.....", "....kgk.kvk.....", "...kNNk.kNNk....", "...kkk...kkk...." },
     { "................", ".....kkkk.......", "....kuggGk......", "....kuekGk......",
       "....kggGvk......", ".....kGvk.......", "...kkuggGkk.....", "..kukgGGvkGk....",
       "kuk..kgGk..kGk..", "kk...kGvk...kk..", ".....kgGk.......", ".....kGvk.......",
       "....kgkkGk......", ".....kgkvk......", "....kNNkNNk.....", "....kkk.kkk....." }},
    /* Spider */
    {{ "................", "................", "................", "................",
       "..S..S....S..S..", "...S.S.kk.S.S...", "....SSkdDkSS....", "..SSkdSdDDkSS...",
       ".S..kdedeeDk..S.", "S..SkDDrDDkS..S.", "...S.kDDDk.S....", "..S..S.S.S..S...",
       ".S..S.....S..S..", "................", "................", "................" },
     { "................", "................", "................", "................",
       "...S.S....S.S...", "..S..S.kk.S..S..", "....SSkdDkSS....", "..SSkdSdDDkSS...",
       ".S..kdedeeDk..S.", "S..SkDDrDDkS..S.", "...S.kDDDk.S....", "...S.S.S.S.S....",
       "..S.S.....S.S...", "................", "................", "................" }},
    /* Imp */
    {{ "................", "....k......k....", "....yk....kY....", ".....kkkkkk.....",
       "....kjjrrrrk....", "....kjykkyRk....", "....krrrrRRk....", ".....kRrrRk.....",
       "...kkjrrrrRkk...", "..kjk.rRRR.kRk..", "..kk..rrRR..kk..", "......kRRk......",
       ".....kRk.kRk....", ".....kk...kk....", "................", "................" },
     { "....k......k....", "....yk....kY....", ".....kkkkkk.....", "....kjjrrrrk....",
       "....kjykkyRk....", "....krrrrRRk....", ".....kRrrRk.....", "...kkjrrrrRkk...",
       "..kjk.rRRR.kRk..", "..kk..rrRR..kk..", "......kRRk......", ".....kRkkRk.....",
       ".....kk..kk.....", "................", "................", "................" }},
    /* Cultist */
    {{ "................", "......kkkk......", ".....kIppPk.....", "....kIpkkPPk....",
       "....kpkeekPk....", "....kpPkkPPk....", "...kIpppPPPPk...", "..kIpppyPPPpPk..",
       "..kpkppyPPPkPk..", "..kk.ppyPPP.kk..", "....kppyPPPk....", "....kppyPPPk....",
       "...kIppyPPPPk...", "...kpPPyPPPPk...", "...kkkkkkkkkk...", "................" },
     { "................", "......kkkk......", ".....kIppPk.....", "....kIpkkPPk....",
       "....kpkyykPk....", "....kpPkkPPk....", "...kIpppPPPPk...", "..kIpppyPPPpPk..",
       "..kpkppyPPPkPk..", "..kk.ppyPPP.kk..", "....kppyPPPk....", "....kppyPPPk....",
       "...kIppyPPPPk...", "...kppPyPPPpk...", "...kkkkkkkkkk...", "................" }},
    /* Golem */
    {{ "................", "....kkkkkkkk....", "...kTssssSSSk...", "...ksdccdSSdk...",
       "...ksSSSSSSdk...", "..kkkSSSSSdkkk..", ".kTskTscsSSkSdk.", ".ksdksSSSSdkddk.",
       ".kSSkSdddSdkSdk.", ".kkkkSSSSddkkkk.", "....ksSkkSdk....", "....ksSk.kSdk...",
       "...ksSSk.kSSdk..", "...kkkkk.kkkkk..", "................", "................" },
     { "................", "................", "....kkkkkkkk....", "...kTssssSSSk...",
       "...ksdccdSSdk...", "...ksSSSSSSdk...", "..kkkSSSSSdkkk..", ".kTskTscsSSkSdk.",
       ".ksdksSSSSdkddk.", ".kSSkSdddSdkSdk.", ".kkkkSSSSddkkkk.", "....ksSkkSdk....",
       "...ksSSk.kSSdk..", "...kkkkk.kkkkk..", "................", "................" }},
    /* Fleshrender (the butcher): apron, cleaver, burning eyes */
    {{ "................", "......kkkk......", ".....kffFFk.....", ".....kfeFek.....",
       ".....kRfFRk.....", "...kkkkFFkkkk...", "..kffkwwwOkFFk..", ".kffkwwrwwOkFFk.",
       ".kfk.kwwrrOk.kFk", ".kfk.kwrwwOk.kSk", "..kk.kwwwOk.kTSk", ".....kNNNNk.kTSk",
       ".....kNk.kNk.ks.", ".....kNk.kNk..k.", "....kNNk.kNNk...", "....kkk...kkk..." },
     { "................", "......kkkk...kk.", ".....kffFFk.kTSk", ".....kfeFek.kTSk",
       ".....kRfFRk.kSk.", "...kkkkFFkkkkFk.", "..kffkwwwOkFFk..", ".kffkwwrwwOkk...",
       ".kfk.kwwrrOk....", ".kfk.kwrwwOk....", "..kk.kwwwOk.....", ".....kNNNNk.....",
       "....kNk..kNk....", "....kNk...kNk...", "...kNNk...kNNk..", "...kkk.....kkk.." }},
};

/* The butcher at full boss size, 32x32: idle with the cleaver down, then raised. */
const char *const art_butcher_big[2][BOSS_N] = {
    {
        "................................",
        "................................",
        ".............kkkkkk.............",
        "...........kkkfffFkkk...........",
        "...........kffffFFfFk...........",
        "..........kkffffFFFEkk..........",
        "..........kfffeeFFFeek..........",
        "..........kfffeeFFeeek..........",
        "..........kRRfffFFRRRk..........",
        ".........kkkkRfFFFRkkkk.........",
        ".......kkkkkkkfFFEkkkkkkk.......",
        ".....kkkffkkkkfFFEkkkkfFkkk.....",
        ".....kfffFkkkwwwwwOkkkfFfFk.....",
        "...kkffffFkwwwwwwwwoOkFFFFFkk...",
        "...kfffFFFwwwwrrwwwOqkkFFFFFk...",
        "..kkffFkkkwwRwrrrwwwOoOkFEEFFkk.",
        "..kfffFk..kwwRwwrrrrOOqk..kfFFk.",
        "..kfffFk..kwwwwwrrrROOqk..kfFEkk",
        "..kkffFk..kwwwrrrwRwOqkk..knNk..",
        "...kfFFk..kwwwRrwwwOqkk...knNk..",
        "...kkkkk..kwwwwwwwwOqk....knNk..",
        ".....kk...kwwwwwwwOOqkkkkkkkkk..",
        "..........kNNNNyyNNNNkksssSSSk..",
        "..........knNNNNNNNkNkksrrSSdk..",
        "..........knNNNk..knNNkssrSrSk..",
        "..........knNNNk..knNNkTTTTTTk..",
        "..........knNNNk..knNNkkkkkkkk..",
        ".........kknNNNk..kNNNNkk.......",
        ".........knNNNkk..kknNNNk.......",
        "........kknNNNk....kNNNNk.......",
        "........kkNNkkk....kkkNNk.......",
        ".........kkkk........kkkk.......",
    },
    {
        "................................",
        "......................kkkkkkkk..",
        ".............kkkkkk...ksssSSSk..",
        "...........kkkfffFkkk.ksrrSSSk..",
        "...........kffffFFfFk.kssrSrdk..",
        "..........kkffffFFFEkkksssSSSk..",
        "..........kfffeeFFFeekkkkkkkkk..",
        "..........kfffeeFFeeek....knNk..",
        "..........kRRfffFFRRRk....knNk..",
        ".........kkkkRfFFFRkkk....knNk..",
        ".......kkkkkkkfFFEkkkkkkkfFEkk..",
        ".....kkkffkkkkfFFEkkkkfffFFEk...",
        ".....kfffFkkkwwwwwOkkkfFFFEkk...",
        "...kkffffFkwwwwwwwwoOkFFEEk.....",
        "...kfffFFFwwwwrrwwwOqkkFkkk.....",
        "..kkffFkkkwwRwrrrwwwOOkkk.......",
        "..kfffFk..kwwRwwrrrrOOOkk.......",
        "..kfffFk..kwwwwwrrrROOqk........",
        "..kkffFk..kwwwrrrwRwOqkk........",
        "...kfFFk..kwwwRrwwwOOqk.........",
        "...kkkkk..kwwwwwwwwOqkk.........",
        ".....kk...kwwwwwwwOOqk..........",
        "..........kNNNNyyNNNNk..........",
        ".........kkNkNNNNNNkNkk.........",
        ".........knNNk....kNNNk.........",
        "........kknNNk.....kNNNkk.......",
        "........knNNNk.....kknNNk.......",
        ".......kknNNNk......kNNNNkk.....",
        ".......knNNNkk......kknNNNk.....",
        "......kknNNNk........kNNNNkk....",
        "......kkNNkkk........kkkNNkk....",
        ".......kkkk............kkkk.....",
    },
};

