/*
 * art_icons.c - ORIGINAL 12x12 skill icon glyphs.
 *
 * 'x' is the skill's colour, 'z' a darker shade of it; other letters are
 * the fixed palette (k outline, w white, y gold, n brown, o bone, r red).
 * '.' shows the icon's tinted background. Order follows the IC_* enum.
 */
#include "icons.h"

const char *const art_skill_icons[IC_COUNT][ICON_N] = {
    /* IC_SWORD */
    { "..........kk", ".........kwk", "........kwxk", ".......kwxk.", "......kwxk..", "..k..kwxk...",
      "..kkkwxk....", "...kyxk.....", "...kyyk.....", "..kyk.kk....", ".kyk........", ".kk........." },
    /* IC_AXE */
    { "....kkkk....", "...kwwxxk...", "..kwxxxxxk..", "..kxxxkkxk..", "..kxxkk.kk..", "...kkknk....",
      ".....knk....", ".....knk....", ".....knk....", ".....knk....", ".....knk....", ".....kkk...." },
    /* IC_WHIRL */
    { "....kkkk....", "..kkxxxxkk..", ".kxxkkkkxwk.", ".kxk....kxk.", "kxk..kk..kxk", "kxk.kwwk.kxk",
      "kxk.kwwk.kxk", "kxk..kk..kxk", ".kxk....kxk.", ".kwxkkkkxxk.", "..kkxxxxkk..", "....kkkk...." },
    /* IC_HAMMER */
    { ".kkkkkkkkkk.", ".kwwxxxxxxk.", ".kxxxxxxxzk.", ".kzzzzzzzzk.", ".kkkkknkkkk.", ".....knk....",
      ".....knk....", ".....knk....", ".....knk....", ".....knk....", "....kyyyk...", "....kkkkk..." },
    /* IC_CLAWS */
    { "xk..xk..xk..", ".xk..xk..xk.", ".wxk.wxk.wxk", "..xk..xk..xk", "..wxk.wxk.wx", "...xk..xk..x",
      "...wxk.wxk.w", "....xk..xk..", "....wxk.wxk.", ".....xk..xk.", ".....xk..xk.", "......k...k." },
    /* IC_SHOUT */
    { "............", ".k.......k..", "kx..kkk..xk.", "x..kwwwk..x.", "x.kwxxxwk.x.", "x.kxkkkxk.x.",
      "x.kxkrkxk.x.", "x.kxkkkxk.x.", "x..kxxxk..x.", "kx..kkk..xk.", ".k.......k..", "............" },
    /* IC_SHIELD */
    { ".kkkkkkkkkk.", ".kwwxxxxxxk.", ".kwxxxyxxxk.", ".kxxxyyyxxk.", ".kxxxxyxxxk.", ".kxxxxyxxxk.",
      "..kxxxxxxk..", "..kxxxxxxk..", "...kxxxxk...", "....kxxk....", ".....kk.....", "............" },
    /* IC_LEAP */
    { ".....kk.....", ".....xx.....", ".....xx.....", ".....xx.....", "..k..xx..k..", "..kx.xx.xk..",
      "...kxxxxk...", "....kxxk....", ".....kk.....", "k.kk.kk.kk.k", "xkxxkxxkxxkx", "kzzzzzzzzzzk" },
    /* IC_QUAKE */
    { "...k....k...", "..kxk..kxk..", "...k....k...", "............", "kkkkkkkkkkkk", "zzzkzzzzkzzz",
      "zzk.kzzk.kzz", "zk...kk...kz", "zzk.kzzk.kzz", "zzzkzzzzkzzz", "zzzzkzzzzzzz", "kkkkkkkkkkkk" },
    /* IC_RAGE */
    { "k..........k", "xk........kx", ".xk.kkkk.kx.", "..kxxxxxxk..", ".kxxxxxxxxk.", ".kxkkxxkkxk.",
      ".kxkrxxrkxk.", ".kxxxxxxxxk.", "..kxxkkxxk..", "..kxkxxkxk..", "...kxxxxk...", "....kkkk...." },
    /* IC_SPARK */
    { ".......kkk..", "......kwxk..", ".....kwxk...", "....kwxk....", "...kwxxkkk..", "..kxxxxxxk..",
      "..kkkkxxk...", ".....kxk....", "....kxk.....", "...kxk......", "...kk.......", "............" },
    /* IC_FLAME */
    { ".....k......", "....kxk.....", "....kxk..k..", "...kxxk.kxk.", "..kxxwxkkxk.", "..kxwwxxxxk.",
      ".kxxwyyxxxk.", ".kxwyyyywxk.", ".kxwyyyywxk.", "..kxwyywxk..", "...kxxxxk...", "....kkkk...." },
    /* IC_FIREBALL */
    { "............", "......kkkk..", ".....kxxxxk.", "....kxwwxxxk", "x..kxwyywxxk", ".x.kxwyywxxk",
      "x.xkxxwwxxxk", ".x..kxxxxxk.", "x.x..kxxxxk.", ".x.x..kkkk..", "..x.........", "............" },
    /* IC_CHAIN */
    { "kk..........", "kwk.....kk..", ".kx....kwk..", "..kx..kxk...", "...kxkxk....", "....kxk.....",
      "....kxk.....", "...kxkxk....", "..kxk..kx...", ".kwk....kxk.", ".kk......kwk", "..........kk" },
    /* IC_SHARDS */
    { "kk..kkk..kk.", "kwk.kwxk.kwk", "kxk.kwxk.kxk", "kxk.kwxk.kxk", ".kxkkwxkkxk.", ".kxk.kxk.kxk",
      "..kk.kxk.kk.", ".....kxk....", ".....kxk....", "......k.....", "............", "............" },
    /* IC_SNOW */
    { ".....kk.....", "..k..xx..k..", "...k.xx.k...", "....kxxk....", ".kk.kxxk.kk.", "kxxxxwwxxxxk",
      "kxxxxwwxxxxk", ".kk.kxxk.kk.", "....kxxk....", "...k.xx.k...", "..k..xx..k..", ".....kk....." },
    /* IC_METEOR */
    { "x...........", ".x.x........", "..x.x.......", ".x.x.x......", "..x.x.kkkk..", "...x.kxxxxk.",
      "....kxwwxxxk", "....kxwyxxxk", "....kxxxxxzk", "....kxxxxzzk", ".....kzzzzk.", "......kkkk.." },
    /* IC_STORM */
    { "...kkkk.....", "..kwwwwk.kk.", ".kwwwwwwkwwk", "kwwwwwwwwwwk", "kzzzzzzzzzzk", ".kkkkkkkkkk.",
      "...kx...kx..", "..kx...kx...", "..kxk..kxk..", "...kx...kx..", "..kx...kx...", "..k....k...." },
    /* IC_TORNADO */
    { "kkkkkkkkkkkk", "kxxxxxxxxxxk", ".kxwwwwwwxk.", "..kxxxxxxk..", "...kxwwxk...", "...kxxxxk...",
      "....kxwk....", "...kxxk.....", "....kxk.....", ".....kxk....", "....kxk.....", "....kk......" },
    /* IC_DAGGER */
    { "..k...k...k.", "..w...w...w.", ".kxk.kxk.kxk", ".kxk.kxk.kxk", ".kxk.kxk.kxk", ".kxk.kxk.kxk",
      "kkykkkykkkyk", ".kyk.kyk.kyk", ".kyk.kyk.kyk", "..k...k...k.", "............", "............" },
    /* IC_EYE */
    { "............", "............", "...kkkkkk...", "..kwwwwwwk..", ".kwwkxxkwwk.", "kwwkxkkxkwwk",
      "kwwkxkkxkwwk", ".kwwkxxkwwk.", "..kwwwwwwk..", "...kkkkkk...", "............", "............" },
    /* IC_ARROW */
    { "........kkkk", ".........kwk", "........kwxk", ".......kwk.k", "......kwk...", ".....kwk....",
      "....kwk.....", "...kwk......", "..kxk.......", ".kxk........", "kxkk........", "kk.........." },
    /* IC_BOW */
    { "...kk.......", "...kxk....k.", "....kxk..kw.", "....kxk.kw..", ".....kxkw...", ".....kxw....",
      ".....kxw....", ".....kxkw...", "....kxk.kw..", "....kxk..kw.", "...kxk....k.", "...kk......." },
    /* IC_ARROWS */
    { "k....k....k.", "wk...w...kw.", ".x...x...x..", "..x..x..x...", "...x.x.x....", "....xxx.....",
      ".....x......", "............", "..kk.kk.kk..", "..kk.kk.kk..", "............", "............" },
    /* IC_TRAP */
    { "............", "............", "..k..k..k...", ".kwk.kwkkwk.", ".kxk.kxkkxk.", "kxxxkxxxkxxk",
      "kkkkkkkkkkkk", "kzzzzzzzzzzk", "kzkkzzzzkkzk", "kzzzzzzzzzzk", "kkkkkkkkkkkk", "............" },
    /* IC_DASH */
    { "............", "......kkk...", ".....kxxxk..", "x.x..kxxxk..", "......kxk...", "xxx.kkxxxkk.",
      "...kxxxxxxk.", "x.x..kxxk...", ".....kx.xk..", "xxx.kx...xk.", "....kk....kk", "............" },
    /* IC_VIAL */
    { "....kkkk....", "....kwwk....", "....kyyk....", ".....kk.....", "....kwxk....", "...kwxxxk...",
      "..kwxxxxxk..", "..kxxwxxxk..", "..kxxxxxxk..", "..kxxxxxzk..", "...kzzzzk...", "....kkkk...." },
    /* IC_RAIN */
    { ".x...x...x..", ".x...x...x..", "kxk.kxk.kxk.", ".k...k...k..", "...x...x...x", "...x...x...x",
      "..kxk.kxk.kx", "...k...k...k", "............", "kkkkkkkkkkkk", "zzzzzzzzzzzz", "kkkkkkkkkkkk" },
    /* IC_SCYTHE */
    { "..kkkkkkk...", ".kwxxxxxxkk.", "kwkkkkkkxxk.", "kk.....knkxk", ".......knk.k", "......knk...",
      "......knk...", ".....knk....", ".....knk....", "....knk.....", "....knk.....", "....kk......" },
    /* IC_BONE */
    { ".kk.........", "kook........", "koook.......", ".kOook......", "..kOook.....", "...kOook....",
      "....kOook...", ".....kOook..", "......kOook.", ".......kOook", "........kook", ".........kk." },
    /* IC_SPEAR */
    { "..........kk", ".........kwk", "........kwxk", ".......kxxk.", "......kxxk..", ".....kxk....",
      "....kxk.....", "...kxk......", "..kxk.......", ".kxk........", "kxk.........", "kk.........." },
    /* IC_CLOUD */
    { "............", "...kkk.kkk..", "..kxxxkxxxk.", ".kxxwxxxxxxk", "kxxxxxxxxxxk", "kxkkxxxkkxxk",
      "kxkwxxxkwxxk", "kxxxxkxxxxxk", ".kxxxxxxxxk.", "..kxkxkxkk..", "...k.k.k....", "............" },
    /* IC_BLOOD */
    { ".....kk.....", ".....kk.....", "....kxxk....", "....kxxk....", "...kxxxxk...", "..kxwxxxxk..",
      "..kxwxxxxk..", ".kxwxxxxxxk.", ".kxxxxxxxxk.", ".kxxxxxxxzk.", "..kxxxxxzk..", "...kkkkkk..." },
    /* IC_PRISON */
    { "kk.kk.kk.kk.", "kwkkwkkwkkwk", "kxk.xk.xk.xk", "kxk.xk.xk.xk", "kxk.xk.xk.xk", "kxk.xk.xk.xk",
      "kxk.xk.xk.xk", "kxk.xk.xk.xk", "kxk.xk.xk.xk", "kwk.wk.wk.wk", "kkkkkkkkkkkk", "zzzzzzzzzzzz" },
    /* IC_SKULL */
    { "...kkkkkk...", "..kxxxxxxk..", ".kxwxxxxxxk.", ".kxxxxxxxxk.", ".kxkkxxkkxk.", ".kxkkxxkkxk.",
      ".kxxxkkxxxk.", "..kxxxxxxk..", "...kxkxkxk..", "...kxxxxxk..", "....kkkkk...", "............" },
    /* IC_BURST */
    { ".....k......", "..k..x..k...", "...kxwxk....", ".k.xwwwx.k..", "..xwwywwx...", "kxwwyyywwxk.",
      "..xwwywwx...", ".k.xwwwx.k..", "...kxwxk....", "..k..x..k...", ".....k......", "............" },
    /* IC_HAND */
    { "..k.k.k.k...", ".kxkxkxkxk..", ".kxkxkxkxk..", ".kxkxkxkxkk.", ".kxxxxxxxxxk", ".kxxxxxxxk..",
      "..kxxxxxk...", "...kxxxk....", "...kxxxk....", "kkkkkkkkkkkk", "zzzzzzzzzzzz", "kkkkkkkkkkkk" },
    /* IC_PAW */
    { "..kk....kk..", ".kxxk..kxxk.", ".kxxk..kxxk.", "..kk.kk.kk..", "....kxxk....", "kk.kxxxxk.kk",
      "kxkxxxxxxkxk", ".kkxxxxxxkk.", "..kxxxxxxk..", "..kxxxxxxk..", "...kxxxxk...", "....kkkk...." },
    /* IC_FIST */
    { "............", "..kkkkkkk...", ".kxkxkxkxk..", ".kxkxkxkxkk.", ".kxxxxxxxkxk", ".kxwxxxxxxxk",
      ".kxxxxxxxxk.", ".kxxxxxxxk..", "..kxxxxxk...", "..kzzzzzk...", "..kzzzzzk...", "..kkkkkkk..." },
    /* IC_ROCKS */
    { "............", "..kk.....kk.", ".kxxk...kxxk", ".kxwk.kkkxwk", ".kxxkkxxkxxk", ".kxxkxwxkxxk",
      ".kxzkxxxkxzk", ".kxzkxxzkxzk", ".kzzkxzzkzzk", "kkkkkkkkkkkk", "zzzzzzzzzzzz", "kkkkkkkkkkkk" },
    /* IC_BOULDER */
    { "............", "...kkkkk....", "..kxxwwxk...", ".kxxwxxxxk..", "kxxxxxxkxxk.", "kxxkxxxxxxk.",
      "kxxxxxxxzxk.", "kxxxxkxxxzk.", ".kxxxxxxzk..", "..kzzzzzk...", "k..kkkkk....", "kk.k...k.k.." },
    /* IC_FEATHER */
    { "..........kk", "........kkwk", ".......kxwxk", "......kxwxk.", ".....kxwxxk.", "....kxwxxk..",
      "...kxwxxk...", "...kwxxk....", "..kwxkk.....", ".kwk........", "kwk.........", "kk.........." },
    /* IC_PALM */
    { "..k.k.k.....", ".kxkxkxk....", ".kxkxkxk.k..", ".kxkxkxkkxk.", ".kxxxxxxkxk.", ".kxxxxxxxxk.",
      ".kxwxxxxxk..", ".kxxxxxxxk..", "..kxxxxxk...", "...kxxxxk...", "...kxxxxk...", "...kkkkkk..." },
    /* IC_STINGER */
    { "kk..........", "kxk.........", ".kxk........", "..kxkk......", "...kxxk.....", "....kxxkk...",
      "....kxxxxk..", ".....kxxwxk.", "......kxwwxk", ".......kxxk.", "........kk..", "............" },
    /* IC_CENTIPEDE */
    { "............", "............", "k.k.k.k.k...", ".kxkxkxkxkk.", "kxxxxxxxxxxk", "kxwxwxwxwxxk",
      "kxxxxxxxxxyk", ".kxkxkxkxkk.", "k.k.k.k.k...", "............", "............", "............" },
    /* IC_WING */
    { "k...........", "kxk.........", "kwxk........", ".kwxkk......", ".kwwxxkk....", "..kwwxxxkk..",
      "..kwwwxxxxk.", "...kwwwxxxxk", "....kkwwxxk.", "......kkkk..", "............", "............" },
    /* IC_STAR */
    { ".....kk.....", ".....xx.....", "....kxxk....", "kkkkxwwxkkkk", ".kxxwwwwxxk.", "..kxwwwwxk..",
      "...kxwwxk...", "..kxxkkxxk..", "..kxk..kxk..", ".kxk....kxk.", ".kk......kk.", "............" },
    /* IC_KEY */
    { ".....kk.....", "....kxxk....", "...kxwwxk...", "..kxwwwwxk..", ".kxwwyywwxk.", "kxwwyyyywwxk",
      "kxwwyyyywwxk", ".kxwwyywwxk.", "..kxwwwwxk..", "...kxwwxk...", "....kxxk....", ".....kk....." },
    /* IC_HEART */
    { "............", ".kkk...kkk..", "kxxxk.kxxxk.", "kxwxxkxxxxk.", "kxwxxxxxxxk.", "kxxxxxxxxxk.",
      ".kxxxxxxxk..", "..kxxxxxk...", "...kxxxk....", "....kxk.....", ".....k......", "............" },
    /* IC_POTION */
    { "....kkkk....", "....kwwk....", "....knnk....", "...kkwwkk...", "..kwrrrrrk..", ".kwrrrrrrrk.",
      ".krwrrrrrrk.", ".krrrrrrrrk.", ".krrrrrrRRk.", "..krrrrRRk..", "...kkkkkk...", "............" },
};
