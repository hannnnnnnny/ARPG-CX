/*
 * world_control.c - the player steering the hero, Diablo style:
 *
 *   WASD                 walk
 *   left click / hold    attack the monster under the cursor, pick up the
 *                        item under it, or walk there (hold to follow)
 *   right click, 1-6     cast that skill bar slot at the cursor's monster
 *   Q                    drink a potion
 *
 * Any input hands the hero to the player for MANUAL_HOLD; then the idle
 * auto battle takes over again (unless the player switched it off). The
 * same building blocks as the AI are used (walking, the primary attack
 * that spends and builds resource, cast_skill), so both play identically.
 */
#include "world_int.h"
#include "events.h"
#include "skills.h"
#include "../core/sound.h"
#include "../gfx/gfx.h"
#include "../i18n/i18n.h"
#include <stdio.h>

#define PICK_RADIUS 14        /* px around the cursor that count as clicking a monster */
#define CAST_RADIUS 40        /* skills aim at the monster nearest the cursor within this */
#define AUTO_LOOT   18        /* walking this close to loot picks it up */

bool hero_manual(const World *w, const Profile *p)
{
    return w->ctl.manual_t > 0 || !p->auto_battle;
}

static int nearest_monster_to(const World *w, fx x, fx y, int radius)
{
    int i, best = -1, bd = radius + 1;
    for (i = 0; i < w->nmon; i++)
        if (w->mon[i].alive) {
            int d = dist_px(x, y, w->mon[i].x, w->mon[i].y);
            if (d < bd) {
                bd = d;
                best = i;
            }
        }
    return best;
}

static int drop_at(const World *w, fx x, fx y)
{
    int i;
    for (i = 0; i < MAX_DROP; i++)
        if (w->dr[i].alive && dist_px(x, y, w->dr[i].x, w->dr[i].y) <= 10)
            return i;
    return -1;
}

static void keyboard_move(World *w, int mx, int my)
{
    w->ctl.target = w->ctl.drop = -1;
    w->ctl.dest = false;
    hero_walk_to(w, w->h.x + FX_FROM_INT(24 * mx), w->h.y + FX_FROM_INT(24 * my));
}

/* A click picks what is under the cursor; holding keeps walking after it. */
static void point_command(World *w, const HeroCommand *cmd)
{
    ControlRT *c = &w->ctl;
    int m;
    if (!cmd->click && c->target >= 0 && w->mon[c->target].alive)
        return;                                    /* holding on a monster: keep hitting it */
    m = nearest_monster_to(w, cmd->tx, cmd->ty, PICK_RADIUS);
    c->target = cmd->click ? m : -1;
    c->drop = cmd->click && m < 0 ? drop_at(w, cmd->tx, cmd->ty) : -1;
    c->dest = c->target < 0 && c->drop < 0;
    c->dx = cmd->tx;
    c->dy = cmd->ty;
}

static void follow(World *w, Profile *p)
{
    ControlRT *c = &w->ctl;
    if (c->target >= 0 && !w->mon[c->target].alive)
        c->target = -1;
    if (c->drop >= 0 && !w->dr[c->drop].alive)
        c->drop = -1;
    if (c->target >= 0 || c->drop >= 0) {
        w->h.target_kind = c->target >= 0 ? TGT_MON : TGT_DROP;
        w->h.target_idx = c->target >= 0 ? c->target : c->drop;
        hero_act_on_target(w, p);
    } else if (c->dest) {
        if (dist_px(w->h.x, w->h.y, c->dx, c->dy) > 4)
            hero_walk_to(w, c->dx, c->dy);
        else
            c->dest = false;
    }
}

/* Pay for a skill that went off: resource, cooldown and attack timer. */
static void paid(World *w, int i, const SkillRT *s)
{
    if (s->cat <= CAT_CORE) {
        w->h.atk_cd = MAX(3, (int)(TICK_HZ / MAX(w->st.aps, 0.5)));
        w->h.attack_t = 8;
    } else {
        w->h.skill_cd[i] = s->cd_ticks;
    }
    if (s->cost > 0 && s->behavior != SB_CHANNEL)
        w->h.res -= s->cost;
    else if (s->cost < 0)
        hero_gain_res(w, -s->cost);
}

static bool ready(const World *w, int i, const SkillRT *s)
{
    if (!s->usable || (s->cost > 0 && w->h.res < s->cost))
        return false;
    if (s->cat <= CAT_CORE)
        return w->h.atk_cd == 0 && w->h.channel_t == 0;
    return w->h.skill_cd[i] == 0;
}

/* The queued bar slot fires at the monster nearest the cursor (or the
 * hero); out of reach, the hero closes in and keeps trying for a moment. */
static void try_cast(World *w, Profile *p)
{
    ControlRT *c = &w->ctl;
    int i = p->bar[c->cast % BAR_SLOTS], m;
    const SkillRT *s;
    if (i == NO_SKILL || --c->cast_t < 0) {
        c->cast = -1;
        return;
    }
    s = &w->st.b.skill[i];
    m = nearest_monster_to(w, w->cmd.tx, w->cmd.ty, CAST_RADIUS);
    if (m < 0)
        m = nearest_monster_to(w, w->h.x, w->h.y, 160);
    if (m >= 0)
        w->h.face = w->mon[m].x > w->h.x ? 1 : -1;
    if (!ready(w, i, s))
        return;
    if (cast_skill(w, p, i, s, m >= 0 ? &w->mon[m] : NULL)) {
        paid(w, i, s);
        c->cast = -1;
    } else if (m >= 0 && c->target < 0) {
        c->target = m;                             /* walk into range */
    }
}

static void touch_surroundings(World *w, Profile *p)
{
    int i;
    for (i = 0; i < MAX_DROP; i++)
        if (w->dr[i].alive && w->dr[i].t > 6 && dist_px(w->h.x, w->h.y, w->dr[i].x, w->dr[i].y) <= AUTO_LOOT)
            hero_pick_up(w, p, i);
    if (events_object_pending(w)
        && dist_px(w->h.x, w->h.y, cell_center(w->ev.cx), cell_center(w->ev.cy)) <= 12)
        events_touch(w, p);
}

static void stairs_check(World *w)
{
    char buf[80];
    if (w->ctl.stairs_msg_t > 0)
        w->ctl.stairs_msg_t--;
    if (dist_px(w->h.x, w->h.y, cell_center(w->stairs_x), cell_center(w->stairs_y)) > 8)
        return;
    if (w->kills >= w->quota) {
        w->ev_floor_done = true;
        world_sound(w, SND_STAIRS);
    } else if (w->ctl.stairs_msg_t == 0) {
        snprintf(buf, sizeof buf, T("SLAY %d MORE TO OPEN THE STAIRS"), w->quota - w->kills);
        world_message(w, buf, RGB565(255, 160, 90));
        w->ctl.stairs_msg_t = 3 * TICK_HZ;
    }
}

void control_update(World *w, Profile *p)
{
    const HeroCommand *cmd = &w->cmd;
    ControlRT *c = &w->ctl;
    if (c->manual_t > 0)
        c->manual_t--;
    if (cmd->potion)
        hero_drink_potion(w, true);
    if (cmd->cast >= 0) {
        c->cast = cmd->cast;
        c->cast_t = TICK_HZ;
    }
    hero_cooldowns(w, p, NULL, false);
    if (w->h.dash_t > 0)
        return;
    if (cmd->mx || cmd->my)
        keyboard_move(w, cmd->mx, cmd->my);
    else if (cmd->click || cmd->hold)
        point_command(w, cmd);
    if (c->cast >= 0)
        try_cast(w, p);
    if (!cmd->mx && !cmd->my)
        follow(w, p);
    touch_surroundings(w, p);
    stairs_check(w);
}
