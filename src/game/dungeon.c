/*
 * dungeon.c - procedural floors: rectangular rooms joined by 2-tile wide
 * corridors (wide enough that packs of monsters never jam a doorway),
 * BFS distance fields for pathfinding, and tile collision.
 */
#include "world_int.h"
#include "balance.h"
#include "story.h"
#include <string.h>

#define MAX_ROOMS 10

typedef struct { int x, y, w, h; } Room;

static void carve(World *w, int x, int y, int cw, int ch)
{
    int i, j;
    for (j = y; j < y + ch; j++)
        for (i = x; i < x + cw; i++)
            if (i > 0 && j > 0 && i < MAP_W - 1 && j < MAP_H - 1)
                w->cell[j][i] = CELL_FLOOR;
}

static bool overlaps(const Room *a, const Room *b)
{
    return a->x - 2 < b->x + b->w && b->x - 2 < a->x + a->w
        && a->y - 2 < b->y + b->h && b->y - 2 < a->y + a->h;
}

static void corridor(World *w, const Room *a, const Room *b, Rng *r)
{
    int ax = a->x + a->w / 2, ay = a->y + a->h / 2;
    int bx = b->x + b->w / 2, by = b->y + b->h / 2;
    if (rng_range(r, 0, 1)) {
        carve(w, MIN(ax, bx), ay, ABS(bx - ax) + 2, 2);
        carve(w, bx, MIN(ay, by), 2, ABS(by - ay) + 2);
    } else {
        carve(w, ax, MIN(ay, by), 2, ABS(by - ay) + 2);
        carve(w, MIN(ax, bx), by, ABS(bx - ax) + 2, 2);
    }
}

static int place_rooms(World *w, Room *rooms)
{
    int n = 0, tries;
    for (tries = 0; tries < 200 && n < MAX_ROOMS; tries++) {
        Room rm;
        int k;
        bool ok = true;
        rm.w = rng_range(&w->rng, 6, 10);
        rm.h = rng_range(&w->rng, 5, 8);
        rm.x = rng_range(&w->rng, 1, MAP_W - rm.w - 2);
        rm.y = rng_range(&w->rng, 1, MAP_H - rm.h - 2);
        for (k = 0; k < n && ok; k++)
            ok = !overlaps(&rm, &rooms[k]);
        if (!ok)
            continue;
        carve(w, rm.x, rm.y, rm.w, rm.h);
        if (n > 0)
            corridor(w, &rooms[n - 1], &rm, &w->rng);
        rooms[n++] = rm;
    }
    return n;
}

static int pick_type(World *w)
{
    int t, tries = 0;
    do {
        t = rng_range(&w->rng, 0, MT_COUNT - 1);
    } while (mon_defs[t].min_floor > w->floor && ++tries < 50);
    return mon_defs[t].min_floor > w->floor ? MT_SKELETON : t;
}

static void populate(World *w, Room *rooms, int n)
{
    int total = floor_monsters(w->floor), placed = 0, guard = 0;
    while (placed < total && guard++ < 400) {
        const Room *rm = &rooms[rng_range(&w->rng, 1, n - 1)];
        int roll = rng_range(&w->rng, 3, 7); /* macro args must be side-effect free */
        int pack = MIN(roll, total - placed), k;
        int type = pick_type(w);
        for (k = 0; k < pack && w->nmon < MAX_MON; k++) {
            bool elite = w->floor >= 3 && rng_range(&w->rng, 0, 99) < 8;
            spawn_monster(w, type, rng_range(&w->rng, rm->x, rm->x + rm->w - 1),
                          rng_range(&w->rng, rm->y, rm->y + rm->h - 1), elite, false);
            placed++;
        }
    }
    w->quota = MIN(floor_quota(w->floor), w->nmon);
}

/* Boss floor: one arena, the boss and a few guards. */
static void build_arena(World *w)
{
    Room arena = { 8, 8, 32, 16 };
    int k;
    carve(w, arena.x, arena.y, arena.w, arena.h);
    w->h.x = cell_center(arena.x + 2);
    w->h.y = cell_center(arena.y + arena.h / 2);
    spawn_monster(w, story_boss_type(w->floor), arena.x + arena.w - 6, arena.y + arena.h / 2, false, true);
    story_boss_name(w->boss_name, sizeof w->boss_name, w->floor);
    for (k = 0; k < 4; k++)
        spawn_monster(w, pick_type(w), arena.x + arena.w - 10, arena.y + 3 + k * 3, false, false);
    w->stairs_x = arena.x + arena.w - 3;
    w->stairs_y = arena.y + arena.h / 2;
    w->quota = w->nmon;
}

static void place_stairs(World *w, Room *rooms, int n)
{
    int best = 1, bestd = -1, i;
    bfs_field(w, w->ft, rooms[0].x + rooms[0].w / 2, rooms[0].y + rooms[0].h / 2);
    for (i = 1; i < n; i++) {
        int d = w->ft[rooms[i].y + rooms[i].h / 2][rooms[i].x + rooms[i].w / 2];
        if (d > bestd) {
            bestd = d;
            best = i;
        }
    }
    w->stairs_x = rooms[best].x + rooms[best].w / 2;
    w->stairs_y = rooms[best].y + rooms[best].h / 2;
}

void dungeon_build(World *w, const Profile *p)
{
    Room rooms[MAX_ROOMS];
    int n, x, y;
    (void)p;
    memset(w->cell, CELL_WALL, sizeof w->cell);
    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < MAP_W; x++)
            w->var[y][x] = (uint8_t)rng_range(&w->rng, 0, 7);
    if (w->boss_floor) {
        build_arena(w);
        return;
    }
    do {
        memset(w->cell, CELL_WALL, sizeof w->cell);
        n = place_rooms(w, rooms);
    } while (n < 4); /* always enough rooms for a real floor */
    w->h.x = cell_center(rooms[0].x + rooms[0].w / 2);
    w->h.y = cell_center(rooms[0].y + rooms[0].h / 2);
    place_stairs(w, rooms, n);
    populate(w, rooms, n);
}

/* ------------------------------------------------------------- queries */

bool world_walkable(const World *w, int cx, int cy)
{
    return cx >= 0 && cy >= 0 && cx < MAP_W && cy < MAP_H && w->cell[cy][cx] != CELL_WALL;
}

void bfs_field(World *w, int16_t field[MAP_H][MAP_W], int cx, int cy)
{
    static int16_t queue[MAP_W * MAP_H];
    static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
    int head = 0, tail = 0;
    memset(field, 0x7F, sizeof(int16_t) * MAP_W * MAP_H); /* ~32639 = unreachable */
    if (!world_walkable(w, cx, cy))
        return;
    field[cy][cx] = 0;
    queue[tail++] = (int16_t)(cy * MAP_W + cx);
    while (head < tail) {
        int c = queue[head++], x = c % MAP_W, y = c / MAP_W, k;
        for (k = 0; k < 4; k++) {
            int nx = x + dx[k], ny = y + dy[k];
            if (world_walkable(w, nx, ny) && field[ny][nx] > field[y][x] + 1) {
                field[ny][nx] = (int16_t)(field[y][x] + 1);
                queue[tail++] = (int16_t)(ny * MAP_W + nx);
            }
        }
    }
}

bool line_of_sight(const World *w, fx x0, fx y0, fx x1, fx y1)
{
    int ax = FX_TO_INT(x0), ay = FX_TO_INT(y0), bx = FX_TO_INT(x1), by = FX_TO_INT(y1);
    int steps = MAX(ABS(bx - ax), ABS(by - ay)) / 6 + 1, i;
    for (i = 0; i <= steps; i++) {
        int px = ax + (bx - ax) * i / steps, py = ay + (by - ay) * i / steps;
        if (!world_walkable(w, px >> TILE_SHIFT, py >> TILE_SHIFT))
            return false;
    }
    return true;
}

/* Line of sight for a body of half-size 'half': the centre line plus the
 * four lines through the box corners, so bodies never snag on wall corners
 * while walking "straight" at something they can see. */
bool body_line_clear(const World *w, fx x0, fx y0, fx x1, fx y1, int half)
{
    fx h = FX_FROM_INT(half);
    return line_of_sight(w, x0, y0, x1, y1)
        && line_of_sight(w, x0 - h, y0 - h, x1 - h, y1 - h)
        && line_of_sight(w, x0 + h - 1, y0 - h, x1 + h - 1, y1 - h)
        && line_of_sight(w, x0 - h, y0 + h - 1, x1 - h, y1 + h - 1)
        && line_of_sight(w, x0 + h - 1, y0 + h - 1, x1 + h - 1, y1 + h - 1);
}

static bool box_free(const World *w, fx x, fx y, int half)
{
    int l = FX_TO_INT(x) - half, r = FX_TO_INT(x) + half - 1;
    int t = FX_TO_INT(y) - half, b = FX_TO_INT(y) + half - 1;
    return world_walkable(w, l >> TILE_SHIFT, t >> TILE_SHIFT) && world_walkable(w, r >> TILE_SHIFT, t >> TILE_SHIFT)
        && world_walkable(w, l >> TILE_SHIFT, b >> TILE_SHIFT) && world_walkable(w, r >> TILE_SHIFT, b >> TILE_SHIFT);
}

void move_body(const World *w, fx *x, fx *y, fx dx, fx dy, int half)
{
    if (box_free(w, *x + dx, *y, half))
        *x += dx;
    if (box_free(w, *x, *y + dy, half))
        *y += dy;
}

int dist_px(fx ax, fx ay, fx bx, fx by)
{
    int dx = ABS(FX_TO_INT(bx - ax)), dy = ABS(FX_TO_INT(by - ay));
    /* Octagonal approximation of the Euclidean distance (no sqrt). */
    return dx > dy ? dx + dy * 3 / 8 : dy + dx * 3 / 8;
}

static uint32_t isqrt64(uint64_t v)
{
    uint64_t r = 0, bit = (uint64_t)1 << 62;
    while (bit > v)
        bit >>= 2;
    while (bit) {
        if (v >= r + bit) {
            v -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return (uint32_t)r;
}

/* Exact Euclidean length (in fx units) so speed is the same in every
 * direction; the octagonal dist_px() is fine for range checks only. */
void step_toward(fx x, fx y, fx tx, fx ty, fx speed, fx *dx, fx *dy)
{
    int64_t ex = tx - x, ey = ty - y;
    uint32_t len = isqrt64((uint64_t)(ex * ex + ey * ey));
    if (len == 0) {
        *dx = *dy = 0;
        return;
    }
    if (len <= (uint32_t)speed) { /* arrive exactly instead of overshooting */
        *dx = (fx)ex;
        *dy = (fx)ey;
        return;
    }
    *dx = (fx)(ex * speed / (int64_t)len);
    *dy = (fx)(ey * speed / (int64_t)len);
}

bool smooth_waypoint(const World *w, const int16_t field[MAP_H][MAP_W], fx x, fx y, int half, fx *wx, fx *wy)
{
    int hop;
    if (!field_waypoint(w, field, x, y, wx, wy))
        return false;
    /* String-pulling: skip ahead to later path cells while they are in a
     * clear straight line, so bodies glide instead of zig-zagging. */
    for (hop = 0; hop < 4; hop++) {
        fx nx, ny;
        if (!field_waypoint(w, field, *wx, *wy, &nx, &ny) || !body_line_clear(w, x, y, nx, ny, half))
            break;
        *wx = nx;
        *wy = ny;
    }
    return true;
}

bool field_waypoint(const World *w, const int16_t field[MAP_H][MAP_W], fx x, fx y, fx *wx, fx *wy)
{
    static const int dx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, dy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
    int cx = px_to_cell(x), cy = px_to_cell(y), k, best = -1, bv;
    if (!world_walkable(w, cx, cy))
        return false;
    bv = field[cy][cx];
    for (k = 0; k < 8; k++) {
        int nx = cx + dx[k], ny = cy + dy[k];
        if (!world_walkable(w, nx, ny))
            continue;
        /* Diagonals only when both orthogonal neighbours are open (no corner cutting). */
        if (k >= 4 && (!world_walkable(w, cx + dx[k], cy) || !world_walkable(w, cx, cy + dy[k])))
            continue;
        if (field[ny][nx] < bv) {
            bv = field[ny][nx];
            best = k;
        }
    }
    if (best < 0)
        return false;
    *wx = cell_center(cx + dx[best]);
    *wy = cell_center(cy + dy[best]);
    return true;
}
