/*
 * test_d4.c - the second wave of Diablo IV style content: fifteen save
 * slots, the packed floors and their events (harvest, cursed shrine,
 * bloodmarked hunt, hell rift), the Fleshrender, the mythic powers and
 * the signature builds' resonance with the depths.
 */
#include "../src/game/aspects.h"
#include "../src/game/balance.h"
#include "../src/game/build.h"
#include "../src/game/events.h"
#include "../src/game/events_int.h"
#include "../src/game/game.h"
#include "../src/game/items.h"
#include "../src/game/mythic.h"
#include "../src/game/progress.h"
#include "../src/game/session.h"
#include "../src/game/story.h"
#include "../src/game/world_int.h"
#include <stdio.h>
#include <string.h>

static int g_fail, g_checks;
#define CHECK(c) do { g_checks++; if (!(c)) { g_fail++; printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static Profile P;
static Session S;

/* A fresh level 40 hero of 'cls' on floor 'floor', no monsters around. */
static World *setup(int cls, int floor)
{
    World *w = &S.w;
    int i;
    prog_new(&P, 77, cls);
    P.level = 40;
    P.skill_points = 40;
    build_auto_spend(&P);
    P.floor = P.best_floor = floor;
    memset(&S, 0, sizeof S);
    session_start(&S, &P);
    for (i = 0; i < w->nmon; i++)
        w->mon[i].alive = 0;
    w->butcher_t = 0;
    return w;
}

/* A monster next to the hero; returns its index or -1. */
static int dummy(World *w, int dx, double hp)
{
    int k, before;
    for (k = 0; k < 8; k++, dx = -dx) {
        before = w->nmon;
        spawn_monster(w, MT_SKELETON, px_to_cell(w->h.x) + dx, px_to_cell(w->h.y) + k / 2, false, false);
        if (w->nmon > before) {
            w->mon[before].max_hp = w->mon[before].hp = hp;
            return before;
        }
    }
    return -1;
}

static void wear_mythic(int power)
{
    Rng r;
    int uid = mythic_unique(power);
    rng_seed(&r, 5);
    item_make_unique(&P.equip[unique_def(uid)->slot], &r, 60, uid, true, P.cls);
    session_profile_changed(&S, &P);
}

static int alive_count(const World *w)
{
    int i, n = 0;
    for (i = 0; i < w->nmon; i++)
        n += w->mon[i].alive;
    return n;
}

/* ---------------------------------------------------------------- slots */

static void test_slots(void)
{
    static Game g;
    char path[300];
    CHECK(SAVE_SLOTS >= 15 && SLOT_ROWS < SAVE_SLOTS);
    memset(&g, 0, sizeof g);
    g.save_base = "/documents/ndless/AshenDepths.sav.tns";
    game_slot_path(&g, SAVE_SLOTS, path, sizeof path);
    CHECK(!strcmp(path, "/documents/ndless/AshenDepths15.sav.tns"));
}

/* --------------------------------------------------------------- events */

static void test_density_and_events(void)
{
    int f, seen[EV_COUNT] = { 0 };
    CHECK(floor_monsters(1) >= 26 && floor_monsters(400) <= MAX_MON);
    prog_new(&P, 9, CLASS_BARBARIAN);
    for (f = 2; f < 200; f++) {
        memset(&S, 0, sizeof S);
        world_init_floor(&S.w, &P, f);
        seen[S.w.ev.kind]++;
        CHECK(S.w.boss_floor || S.w.ev.kind != EV_NONE || S.w.nmon > 0);
    }
    CHECK(seen[EV_HARVEST] > 5 && seen[EV_CURSED] > 5 && seen[EV_HUNT] > 5 && seen[EV_RIFT] > 3);
}

static void test_harvest(void)
{
    World *w = setup(CLASS_BARBARIAN, 30);
    int k;
    memset(&w->ev, 0, sizeof w->ev);
    events_d4_init(w, EV_HARVEST);
    CHECK(w->ev.kind == EV_HARVEST && w->ev.goal >= 30);
    w->kills = 1;
    events_d4_tick(w, &P);
    CHECK(w->ev.state == ES_RUNNING && events_wave_active(w));
    events_d4_tick(w, &P);
    CHECK(alive_count(w) > 0);                        /* the first pack poured in */
    for (k = 0; k < w->ev.goal; k++)
        events_d4_on_kill(w, &P, &w->mon[0]);
    events_d4_tick(w, &P);
    CHECK(w->ev.state == ES_DONE && P.n_events >= 1);
}

static void test_cursed_shrine(void)
{
    World *w = setup(CLASS_BARBARIAN, 30);
    int wave, i;
    memset(&w->ev, 0, sizeof w->ev);
    w->ev.kind = EV_CURSED;
    w->ev.cx = px_to_cell(w->h.x);
    w->ev.cy = px_to_cell(w->h.y);
    events_touch(w, &P);
    CHECK(w->ev.stage == 1 && w->ev.wave_left > 0);
    for (wave = 0; wave < 3 && w->ev.state != ES_DONE; wave++)
        for (i = 0; i < w->nmon; i++)
            if (w->mon[i].alive && w->mon[i].wave)
                kill_rewards(w, &P, &w->mon[i]);
    CHECK(w->ev.state == ES_DONE && w->ev.stage == 3);
}

static void test_rift_and_hunt(void)
{
    World *w = setup(CLASS_BARBARIAN, 30);
    int i, t;
    memset(&w->ev, 0, sizeof w->ev);
    w->ev.kind = EV_RIFT;
    w->ev.cx = px_to_cell(w->h.x);
    w->ev.cy = px_to_cell(w->h.y);
    events_touch(w, &P);
    for (t = 0; t < 19 * TICK_HZ; t++)
        events_d4_tick(w, &P);
    CHECK(w->ev.stage == 1 && w->ev.state == ES_RUNNING);    /* collapsed, its spawn still standing */
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && w->mon[i].wave)
            kill_rewards(w, &P, &w->mon[i]);
    events_d4_tick(w, &P);
    CHECK(w->ev.state == ES_DONE);
    /* the hunt: an elite with one affix more than its depth gives, and its guard */
    w = setup(CLASS_BARBARIAN, 40);
    memset(&w->ev, 0, sizeof w->ev);
    events_d4_init(w, EV_HUNT);
    for (i = 0, t = -1; i < w->nmon; i++)
        if (w->mon[i].alive && w->mon[i].special == MS_HUNT)
            t = i;
    CHECK(t >= 0 && w->ev.kind == EV_HUNT);
    if (t >= 0) {
        kill_rewards(w, &P, &w->mon[t]);
        CHECK(w->ev.state == ES_DONE);
    }
}

/* -------------------------------------------------------------- butcher */

static void test_butcher(void)
{
    World *w;
    int f, n = 0, i, b = -1;
    prog_new(&P, 3, CLASS_BARBARIAN);
    for (f = 21; f < 2021; f++) {                   /* about 1 floor in 40, never on a guardian floor */
        memset(&S, 0, sizeof S);
        world_init_floor(&S.w, &P, f);
        n += S.w.butcher_t > 0;
        CHECK(!(S.w.boss_floor && S.w.butcher_t > 0));
    }
    CHECK(n > 20 && n < 90);
    w = setup(CLASS_BARBARIAN, 40);
    w->butcher_t = 1;
    butcher_tick(w, &P);
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive && w->mon[i].special == MS_BUTCHER)
            b = i;
    CHECK(b >= 0 && w->mon[b].aggro && w->mon[b].type == MT_BUTCHER);
    if (b < 0)
        return;
    i = (int)P.n_elites;
    kill_rewards(w, &P, &w->mon[b]);
    CHECK((int)P.n_elites == i + 1 && P.n_events >= 1);
}

/* -------------------------------------------------------------- mythics */

static void test_mythic_table(void)
{
    int k, n = 0;
    for (k = MY_NONE + 1; k < MY_COUNT; k++) {
        int uid = mythic_unique(k);
        CHECK(uid > 0 && unique_def(uid)->mythic && unique_def(uid)->cls == ANY_CLASS);
    }
    for (k = 1; k <= unique_count; k++)
        n += unique_def(k)->mythic;
    CHECK(n == MY_COUNT - 1 && n >= 19);              /* 17 new, the 2 old ones re-forged */
}

static void test_mythic_powers(void)
{
    World *w = setup(CLASS_BARBARIAN, 60);
    double x_all = w->st.b.x_all;
    int m, k;
    wear_mythic(MY_ONENAME);
    CHECK(w->st.b.myth_any && w->st.b.x_all > x_all * 1.9);
    /* starfall: stars find the pack */
    m = dummy(w, 2, 1e15);
    wear_mythic(MY_STARFALL);
    for (k = 0; k < 40; k++)
        myth_tick(w, &P);
    CHECK(m >= 0 && w->mon[m].hp < 1e15 && w->hs.sig > 0);
    /* the reaper: a weakened foe dies outright */
    wear_mythic(MY_REAPER);
    m = dummy(w, -2, 1000);
    if (m >= 0) {
        Hit h = skill_hit(w, w->core_skill, 1.0);
        w->mon[m].hp = 100;
        h.base = 1;
        deal_damage(w, &P, m, &h);
        CHECK(!w->mon[m].alive);
    }
    /* the aegis: death is refused once, then the cooldown holds */
    wear_mythic(MY_UNDYING);
    CHECK(w->st.b.x_life >= 3.0);
    hurt_hero(w, &P, w->st.max_hp * 1e6, EL_PHYS, -1);
    CHECK(w->h.dead_t == 0 && w->h.hp > 0 && w->myth.undying_cd > 0);
    /* the orbit: nothing costs anything */
    wear_mythic(MY_ORBIT);
    for (k = 0; k < CLASS_SKILLS; k++)
        CHECK(w->st.b.skill[k].cost <= 0);
}

static void test_mythic_dive_and_loot(void)
{
    World *w = setup(CLASS_BARBARIAN, 59);
    Item it;
    Rng r;
    int k, dives = 0;
    wear_mythic(MY_DIVER);
    P.mode = MODE_PUSH;
    for (k = 0; k < 200; k++) {
        int d = myth_dive(w, &P);
        dives += d > 0;
        CHECK(d == 0 || (!is_boss_floor(P.floor) && !is_boss_floor(P.floor + 1) && !is_boss_floor(P.floor + 2)));
    }
    P.floor = 61;
    for (k = 0; k < 200; k++)
        dives += myth_dive(w, &P) > 0;
    CHECK(dives > 20);
    /* a mythic is never swapped out for a better-looking ordinary item */
    rng_seed(&r, 8);
    item_roll_slot(&it, &r, 400, 0, RAR_LEGEND, SLOT_BOOTS, P.cls);
    k = it.rarity;
    prog_handle_loot(&P, &it, &(double){ 0 });
    CHECK(k == RAR_MYTHIC || P.equip[SLOT_BOOTS].rarity == RAR_MYTHIC);
}

/* ------------------------------------------------------------ resonance */

static void test_resonance(void)
{
    World *w;
    Rng r;
    int uid = sig_unique_for(CLASS_NECRO);
    double plain;
    CHECK(sig_resonance(1) == 1.0 && sig_resonance(300) > 50.0 && sig_resonance(300) > sig_resonance(200));
    w = setup(CLASS_NECRO, 300);
    CHECK(w->sig.res == 1.0);
    hurt_hero(w, &P, 1000, EL_PHYS, -1);
    plain = w->st.max_hp - w->h.hp;
    rng_seed(&r, 3);
    item_make_unique(&P.equip[unique_def(uid)->slot], &r, 300, uid, false, CLASS_NECRO);
    session_profile_changed(&S, &P);
    CHECK(w->st.b.sig == SIG_BONESPEAR && w->sig.res > 50.0);
    w->h.hp = w->st.max_hp;
    hurt_hero(w, &P, 1000, EL_PHYS, -1);
    CHECK(w->st.max_hp - w->h.hp < plain);
}

void test_d4(int *checks, int *fails)
{
    printf("slots 15\n");          test_slots();
    printf("density/events\n");    test_density_and_events();
    printf("harvest\n");           test_harvest();
    printf("cursed shrine\n");     test_cursed_shrine();
    printf("rift/hunt\n");         test_rift_and_hunt();
    printf("butcher\n");           test_butcher();
    printf("mythic table\n");      test_mythic_table();
    printf("mythic powers\n");     test_mythic_powers();
    printf("mythic dive/loot\n");  test_mythic_dive_and_loot();
    printf("resonance\n");         test_resonance();
    *checks += g_checks;
    *fails += g_fail;
}
