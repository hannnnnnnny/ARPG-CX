#include "session.h"
#include "progress.h"
#include "balance.h"
#include "story.h"
#include "events.h"
#include "mythic.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"
#include <stdio.h>

/* Events are only marked seen once displayed, so nothing is ever lost
 * even if several happen while menus are open. */
static void queue_story(Session *s, Profile *p, int event)
{
    int i;
    if (story_event_seen(p, event) || s->story_pending == event)
        return;
    for (i = 0; i < s->story_count; i++)
        if (s->story_queue[i] == event)
            return;
    if (s->story_pending < 0)
        s->story_pending = event;
    else if (s->story_count < 8)
        s->story_queue[s->story_count++] = event;
}

void session_story_shown(Session *s, Profile *p)
{
    int i;
    if (s->story_pending >= 0)
        story_mark_seen(p, s->story_pending);
    s->story_pending = s->story_count > 0 ? s->story_queue[0] : -1;
    for (i = 1; i < s->story_count; i++)
        s->story_queue[i - 1] = s->story_queue[i];
    if (s->story_count > 0)
        s->story_count--;
}

/* A new floor inside an act whose intro has not been told yet (also
 * catches heroes who were already past floor 50 when acts VI-X arrived). */
static void check_act_intro(Session *s, Profile *p)
{
    int act = story_act(p->floor);
    if (act >= 0 && (story_is_act_start(p->floor) || act >= CAMPAIGN_ACTS))
        queue_story(s, p, STORY_INTRO + act);
}

static void act_cleared(Session *s, Profile *p)
{
    int act = story_act(p->floor);
    if (!story_is_act_boss(p->floor) || act < 0)
        return;
    queue_story(s, p, STORY_VICTORY + act);
    if (act == CAMPAIGN_ACTS - 1)
        queue_story(s, p, STORY_EPILOGUE);
    if (act == CAMPAIGN_FLOORS / 10 - 1)
        queue_story(s, p, STORY_FINALE);
}

void session_start(Session *s, Profile *p)
{
    s->story_pending = -1;
    s->story_count = 0;
    world_init_floor(&s->w, p, p->floor);
    goals_refill(p, &s->w.rng);         /* saves from before v6 have no bounties */
    check_act_intro(s, p);
}

static void announce_floor(World *w, const Profile *p)
{
    char buf[96];
    if (is_boss_floor(p->floor))
        snprintf(buf, sizeof buf, T("%s AWAITS"), T(w->boss_name));
    else
        snprintf(buf, sizeof buf, T("FLOOR %d"), p->floor);
    world_message(w, buf, RGB565(255, 200, 120));
}

/* First step into a new Torment tier: monsters and loot both grow. */
static void announce_tier(World *w, const Profile *p)
{
    char roman[8], buf[48];
    torment_name(roman, sizeof roman, p->floor);
    snprintf(buf, sizeof buf, T("TORMENT %s"), roman);
    world_banner(w, buf, RGB565(255, 90, 60));
    world_message(w, "THE DEPTHS GROW DARKER. LOOT GROWS RICHER.", RGB565(255, 150, 90));
}

void session_tick(Session *s, Profile *p)
{
    world_tick(&s->w, p);
    if (p->auto_craft && s->w.tick % (5 * TICK_HZ) == 0 && prog_auto_craft(p, &s->w.rng) > 0)
        world_refresh_stats(&s->w, p);
    if (s->w.ev_lore > 0) {
        queue_story(s, p, STORY_LORE + s->w.ev_lore - 1);
        s->w.ev_lore = 0;
    }
    if (s->w.ev_floor_done) {
        int best = p->best_floor, dive;
        s->floors_cleared++;
        act_cleared(s, p);
        prog_floor_cleared(p, is_boss_floor(p->floor));
        dive = myth_dive(&s->w, p);
        p->floor += dive;
        p->best_floor = MAX(p->best_floor, p->floor);
        p->best_floor_ever = MAX(p->best_floor_ever, p->best_floor);
        world_init_floor(&s->w, p, p->floor);
        if (dive)
            world_banner(&s->w, "THE ABYSS PULLS YOU DEEPER", RGB565(170, 90, 255));
        announce_floor(&s->w, p);
        check_act_intro(s, p);
        world_goal(&s->w, p, GE_FLOOR, 0);
        if (p->floor == p->best_floor && p->floor >= TORMENT_FLOOR && (p->floor - TORMENT_FLOOR) % TORMENT_TIER_FLOORS == 0)
            announce_tier(&s->w, p);
        if (p->best_floor > best && p->best_floor > MAX(p->best_floor_ever, 10))
            bark(&s->w.bark, BK_RECORD, (uint32_t)s->w.tick + (uint32_t)p->floor);
    } else if (s->w.ev_stuck) {
        s->stuck_resets++;
        world_init_floor(&s->w, p, p->floor);
    } else if (s->w.ev_died) {
        s->deaths++;
        prog_died(p);
        world_init_floor(&s->w, p, p->floor);
        announce_floor(&s->w, p);
    }
}

void session_profile_changed(Session *s, const Profile *p)
{
    world_refresh_stats(&s->w, p);
}
