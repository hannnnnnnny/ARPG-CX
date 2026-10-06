#include "session.h"
#include "progress.h"
#include "balance.h"
#include "story.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"
#include <stdio.h>

int story_seen_bit(int event)
{
    if (event >= STORY_EPILOGUE) return 10;
    if (event >= STORY_VICTORY)  return 5 + (event - STORY_VICTORY);
    return event;
}

/* Events are only marked seen once displayed, so nothing is ever lost
 * even if several happen while menus are open. */
static void queue_story(Session *s, Profile *p, int event, int bit)
{
    int i;
    (void)bit;
    if (story_seen(p->story_seen, story_seen_bit(event)) || s->story_pending == event)
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
        p->story_seen |= (uint16_t)(1u << story_seen_bit(s->story_pending));
    s->story_pending = s->story_count > 0 ? s->story_queue[0] : -1;
    for (i = 1; i < s->story_count; i++)
        s->story_queue[i - 1] = s->story_queue[i];
    if (s->story_count > 0)
        s->story_count--;
}

/* A new floor: maybe the first step into a new act. */
static void check_act_intro(Session *s, Profile *p)
{
    int act = story_act(p->floor);
    if (act >= 0 && story_is_act_start(p->floor))
        queue_story(s, p, STORY_INTRO + act, act);
}

void session_start(Session *s, Profile *p)
{
    s->story_pending = -1;
    s->story_count = 0;
    world_init_floor(&s->w, p, p->floor);
    if (p->floor == 1)
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

void session_tick(Session *s, Profile *p)
{
    world_tick(&s->w, p);
    if (p->auto_craft && s->w.tick % (5 * TICK_HZ) == 0 && prog_auto_craft(p, &s->w.rng) > 0)
        world_refresh_stats(&s->w, p);
    if (s->w.ev_floor_done) {
        int act = story_act(p->floor);
        s->floors_cleared++;
        if (story_is_act_boss(p->floor) && act >= 0) {
            queue_story(s, p, STORY_VICTORY + act, 5 + act);
            if (act == 4)
                queue_story(s, p, STORY_EPILOGUE, 10);
        }
        prog_floor_cleared(p, is_boss_floor(p->floor));
        world_init_floor(&s->w, p, p->floor);
        announce_floor(&s->w, p);
        check_act_intro(s, p);
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
