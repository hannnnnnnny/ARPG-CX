/*
 * game.h - top-level state machine:
 *
 *   TITLE -> SLOTS -> CLASS SELECT -> CREATE (look + name) -> BATTLE
 *   TITLE -> CONTINUE / LOAD -> (offline report) -> BATTLE
 *   BATTLE <-> MENU [HERO|BAG|SKILLS|BOARD|TOWN|GOALS|EMBERS|OPTIONS]
 *   EMBERS: rebirth -> CLASS SELECT (next life's class) -> BATTLE
 *
 * The dungeon keeps running while menus are open: it is an idle game. The
 * story plays as subtitles over the battle (or as full pages if the
 * player turns that on), so nothing ever waits for a key press.
 */
#ifndef AD_GAME_H
#define AD_GAME_H

#include "session.h"
#include "progress.h"
#include "save.h"
#include "../input/input.h"

typedef enum {
    GS_TITLE, GS_SLOTS, GS_CLASS_SELECT, GS_CREATE, GS_OFFLINE, GS_STORY, GS_BATTLE, GS_MENU
} GameState;
typedef enum { PG_HERO, PG_BAG, PG_SKILLS, PG_PARAGON, PG_TOWN, PG_GOALS, PG_EMBERS, PG_OPTIONS, PG_COUNT } MenuPage;
typedef enum {
    CF_NONE, CF_NEW_GAME, CF_REBIRTH, CF_SALVAGE_ALL, CF_RESET_SKILLS, CF_SWITCH_PRESET, CF_RESET_PARAGON,
    CF_DELETE_SLOT
} ConfirmKind;
typedef enum { TOWN_LIST, TOWN_SMITH, TOWN_OCCULT, TOWN_JEWEL, TOWN_ALCHEMY, TOWN_GAMBLE, TOWN_COUNT } TownScreen;

#define SAVE_SLOTS 15
#define SLOT_ROWS  4      /* slot screen rows visible at once (the list scrolls) */

/* What the slot screen shows without loading a whole profile into play. */
typedef struct {
    bool used;
    SaveStatus status;
    uint8_t cls, preset;
    int level, paragon, floor, best_floor, rebirths;
    char name[NAME_LEN + 1];
    uint32_t save_time;
} SlotInfo;

typedef struct { int fps, logic_us, render_us; } PerfStats;

/* Enchanting shows two new rolls next to the current affix. */
typedef struct { bool open; int slot, affix, pick; Affix opt[2]; } EnchantDialog;

typedef struct {
    GameState state;
    MenuPage page;
    int sel[PG_COUNT];
    int scroll[PG_COUNT];
    int title_sel;
    int class_sel;
    bool class_for_rebirth;  /* class select leads to a rebirth, not a new game */
    /* character creation */
    int create_sel;
    bool create_edit_only;   /* opened from OPTIONS: change the look of the current hero */
    int name_cursor;
    bool name_edit;
    Look create_look;
    /* hero page */
    bool hero_stats;         /* character sheet instead of the paper doll */
    /* skills page dialogs */
    int upgrade_pick;        /* skill whose upgrade dialog is open, -1 = none */
    int upgrade_sel;
    int preset_pick;         /* preset chosen for CF_SWITCH_PRESET */
    /* paragon page */
    int para_board, para_x, para_y;
    /* town */
    TownScreen town;
    int town_sel, town_opt;  /* row and the left/right choice (recipe, gem, aspect) */
    int town_mode;           /* occultist: 0 enchant, 1 imprint */
    Item gamble_last;
    EnchantDialog ench;
    Rng craft_rng;
    /* story */
    int story_id;            /* STORY_* event being shown on a full page */
    int story_t;
    int sub_event, sub_t;    /* subtitle story playing over the battle, -1 = none */
    bool from_journal;
    bool journal;            /* OPTIONS shows the story journal */
    int journal_sel;
    ConfirmKind confirm;
    Profile p;
    Session s;
    bool has_save;
    SaveStatus save_status;
    const char *save_base;      /* platform save path; slots derive from it */
    char save_path[300];        /* current slot's file ("" = saving disabled) */
    int slot;                   /* 1..SAVE_SLOTS */
    int slot_sel;
    bool slot_new;              /* slot screen picks a slot for a NEW game */
    SlotInfo slots[SAVE_SLOTS];
    int autosave_t;
    int hitstop_cool;           /* frames before the next hit stop may freeze the battle */
    OfflineReport off;
    char toast[112];
    uint16_t toast_color;
    int toast_t;
    int tick;
    bool quit;
    PerfStats perf;
    /* desktop: pointer, and menu clicks replayed as key presses */
    bool mouse;
    int mx, my;
    int nav_pending;            /* UP (<0) / DOWN (>0) presses still to replay */
    bool nav_ok;                /* then press OK */
} Game;

void game_init(Game *g, const char *save_path, uint32_t now);
void game_tick(Game *g, Input *in, uint32_t now);
void game_render(Game *g);
void game_save(Game *g, uint32_t now);
static inline bool game_should_quit(const Game *g) { return g->quit; }
static inline bool game_low_power(const Game *g) { return g->p.low_power != 0; }
/* Battle view benefits from in-between (interpolated) frames; menus don't. */
static inline bool game_animating(const Game *g) { return g->state == GS_BATTLE; }
static inline void game_set_perf(Game *g, PerfStats p) { g->perf = p; }
void game_toast(Game *g, const char *text, uint16_t color);

/* game_control.c: keyboard and mouse play, menu clicks, sound */
void game_hero_command(Game *g, const Input *in);
void game_mouse_ui(Game *g, Input *in);
void game_ui_row(int y, int h, bool on);   /* a clickable list row, from the last frame */
void game_ui_rect(int x, int y, int w, int h, bool on);
void game_ui_frame(void);
void game_sfx(const Game *g, int id);
void game_ui_sounds(const Game *g, const Input *in);
void game_world_sounds(const Game *g);
void game_control_render(const Game *g);
void game_profile_changed(Game *g);

/* Save slots */
void game_slot_path(const Game *g, int slot, char *out, size_t cap);
void game_scan_slots(Game *g);
bool game_load_slot(Game *g, int slot);
void game_select_slot(Game *g, int slot);

/* screens.c: title, slots, offline report, confirmations */
void title_tick(Game *g, Input *in, uint32_t now);
void title_render(Game *g);
void slots_tick(Game *g, Input *in, uint32_t now);
void slots_render(Game *g);
void offline_render(Game *g);
void confirm_render(Game *g);
bool screens_confirm_tick(Game *g, Input *in, uint32_t now);
void begin_battle(Game *g, uint32_t now, bool fresh);
void title_backdrop(int tick);
/* screens_hero.c: class select and character creation */
void class_select_tick(Game *g, Input *in, uint32_t now);
void class_select_render(Game *g);
void create_open(Game *g, bool edit_only);
void create_tick(Game *g, Input *in, uint32_t now);
void create_render(Game *g);
/* story.c screens: full pages, subtitles and the journal */
void story_tick(Game *g, Input *in);
void story_render(Game *g);
void subtitle_tick(Game *g);
void subtitle_render(const Game *g);
const char *const *story_page(int id, const char **title);
#define JOURNAL_MAX 80   /* every story event: STORY_EVENT_END */
int  journal_events(const Profile *p, int out[JOURNAL_MAX]);
/* menus*.c */
void menu_tick(Game *g, Input *in, uint32_t now);
void menu_render(Game *g);

#endif
