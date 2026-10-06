/*
 * main_nspire.c - Ashen Depths: TI-Nspire CX / CX CAS (Gen 1) platform layer.
 *
 * Hardware notes (see docs/HARDWARE.md for sources):
 *  - Display: lcd_init(SCR_320x240_565) + lcd_blit(). libndls handles the
 *    rotated 240x320 panel used on hardware revision W and later, so the
 *    same binary works on every CX Gen 1. lcd_init(SCR_TYPE_INVALID) on exit
 *    gives the screen back to the OS.
 *  - Timer: the first SP804 timer block at 0x900C0000 runs free (as in the
 *    nSDL port); its real rate is calibrated against msleep() at startup
 *    because some hardware revisions don't run it at 32768 Hz. msleep() in
 *    libndls uses the *other* timer (0x900D0000), so they never conflict.
 *    Registers are saved first and restored on exit. If the counter does
 *    not move, frames are paced from the RTC instead (see timer section).
 *  - Input: isKeyPressed(). Menus only need one key at a time, so the
 *    touchpad works fine; the keypad (4/6/8/2, 5 = OK) is an alternative.
 *  - Clock: time() reads the battery-backed RTC (Ndless newlib), used for
 *    offline progress.
 *  - CX II models use a different SoC; this file targets CX Gen 1 only.
 */
#include <os.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "../../src/core/platform.h"
#include "../../src/input/input.h"
#include "../../src/game/runner.h"

/* ---------------------------------------------------------------- timer */

#define TIMER1_LOAD    ((volatile uint32_t *)0x900C0000)
#define TIMER1_VALUE   ((volatile uint32_t *)0x900C0004)
#define TIMER1_CONTROL ((volatile uint32_t *)0x900C0008)
#define TIMER_CLOCKSEL ((volatile uint32_t *)0x900C0080)
#define BUS_DISABLE    ((volatile uint32_t *)0x900B0018)

/* SP804 control: enable | 32-bit | free-running, no interrupt, no prescale. */
#define SP804_FREE_RUN_32 0x82u
#define CALIBRATE_MS 100

/*
 * Frame clock. The timer's input frequency is NOT assumed: on some CX
 * hardware revisions the clock-select write does not give the documented
 * 32768 Hz, so the real rate is measured against msleep() at startup.
 * Raw counter deltas are accumulated in 64 bits, so counter wrap-around
 * never makes time jump. If the counter does not move at all, frames are
 * paced from the battery-backed RTC (time(), 1 s resolution) instead.
 */
typedef enum { CLOCK_TIMER, CLOCK_RTC_PACED } ClockMode;

typedef struct {
    uint32_t load, control, clocksel, bus;  /* saved registers */
    bool     touched;
    ClockMode mode;
    uint32_t hz;                             /* measured timer rate */
    uint32_t last;                           /* last raw counter value */
    uint64_t ticks;                          /* accumulated ticks */
    /* RTC-paced fallback */
    uint64_t fake_us;
    uint32_t frame_us;
    uint32_t frames_this_sec;
    time_t   rtc_last;
    char     desc[40];
} NspireTimer;

static NspireTimer g_timer;

static void timer_start(void)
{
    uint32_t a, b, d;
    g_timer.mode = CLOCK_RTC_PACED;
    g_timer.frame_us = 1000000u / 30u;
    g_timer.rtc_last = time(NULL);
    strcpy(g_timer.desc, "CLOCK: RTC-PACED");
    if (is_cx2)
        return; /* different SoC: never poke its timers */
    g_timer.touched = true;
    g_timer.bus = *BUS_DISABLE;
    g_timer.load = *TIMER1_LOAD;
    g_timer.control = *TIMER1_CONTROL;
    g_timer.clocksel = *TIMER_CLOCKSEL;

    *BUS_DISABLE = g_timer.bus & ~(1u << 11); /* make sure the timer block is clocked */
    *TIMER1_CONTROL = 0;
    *TIMER_CLOCKSEL = 0xA;                    /* documented 32768 Hz source */
    *TIMER1_LOAD = 0xFFFFFFFFu;
    *TIMER1_CONTROL = SP804_FREE_RUN_32;

    /* Measure the real rate: SP804 counts down. */
    a = *TIMER1_VALUE;
    msleep(CALIBRATE_MS);
    b = *TIMER1_VALUE;
    d = a - b;
    if (d >= 50 && d <= 400000000u) {
        g_timer.mode = CLOCK_TIMER;
        g_timer.hz = (uint32_t)((uint64_t)d * 1000u / CALIBRATE_MS);
        g_timer.last = *TIMER1_VALUE;
        snprintf(g_timer.desc, sizeof g_timer.desc, "CLOCK: TIMER %uHZ", (unsigned)g_timer.hz);
    }
}

static void timer_stop(void)
{
    if (!g_timer.touched)
        return;
    *TIMER1_CONTROL = 0;
    *TIMER_CLOCKSEL = g_timer.clocksel;
    *TIMER1_LOAD = g_timer.load;
    *TIMER1_CONTROL = g_timer.control;
    *BUS_DISABLE = g_timer.bus;
}

uint32_t plat_time_us(void)
{
    uint32_t v;
    if (g_timer.mode != CLOCK_TIMER)
        return (uint32_t)g_timer.fake_us;
    v = *TIMER1_VALUE;
    g_timer.ticks += (uint32_t)(g_timer.last - v); /* modulo 2^32: wrap-safe */
    g_timer.last = v;
    return (uint32_t)(g_timer.ticks * 1000000u / g_timer.hz);
}

/* Fallback pacing: count frames per RTC second and spread that second
 * evenly over them, so game speed stays right on average. */
static void rtc_pace_frame(void)
{
    time_t now = time(NULL);
    g_timer.frames_this_sec++;
    if (now != g_timer.rtc_last) {
        uint32_t f = g_timer.frames_this_sec;
        g_timer.frame_us = f ? CLAMP(1000000u / f, 10000u, 200000u) : g_timer.frame_us;
        g_timer.frames_this_sec = 0;
        g_timer.rtc_last = now;
    }
    g_timer.fake_us += g_timer.frame_us;
}

void plat_wait(void)
{
    if (g_timer.mode != CLOCK_TIMER)
        g_timer.fake_us += 1000;
}

const char *plat_clock_desc(void) { return g_timer.desc; }

/* ---------------------------------------------------------------- input */

typedef struct { const t_key *key; uint32_t buttons; } KeyBinding;

uint32_t plat_read_buttons(void)
{
    /* Physical key -> logical button(s). Only this table knows keycodes. */
    static const KeyBinding map[] = {
        { &KEY_NSPIRE_LEFT,  BTN_LEFT },  { &KEY_NSPIRE_4, BTN_LEFT },
        { &KEY_NSPIRE_RIGHT, BTN_RIGHT }, { &KEY_NSPIRE_6, BTN_RIGHT },
        { &KEY_NSPIRE_UP,    BTN_UP },    { &KEY_NSPIRE_8, BTN_UP },
        { &KEY_NSPIRE_DOWN,  BTN_DOWN },  { &KEY_NSPIRE_2, BTN_DOWN },
        { &KEY_NSPIRE_LEFTUP,    BTN_LEFT | BTN_UP },
        { &KEY_NSPIRE_UPRIGHT,   BTN_RIGHT | BTN_UP },
        { &KEY_NSPIRE_RIGHTDOWN, BTN_RIGHT | BTN_DOWN },
        { &KEY_NSPIRE_DOWNLEFT,  BTN_LEFT | BTN_DOWN },
        { &KEY_NSPIRE_ENTER, BTN_OK }, { &KEY_NSPIRE_RET, BTN_OK },
        { &KEY_NSPIRE_CLICK, BTN_OK }, { &KEY_NSPIRE_5, BTN_OK },
        { &KEY_NSPIRE_ESC, BTN_BACK },
        { &KEY_NSPIRE_TAB, BTN_TAB }, { &KEY_NSPIRE_MENU, BTN_TAB },
        { &KEY_NSPIRE_DEL, BTN_ALT },
        { &KEY_NSPIRE_CTRL, BTN_LOCK },
        { &KEY_NSPIRE_F, BTN_DEBUG },
    };
    uint32_t b = 0;
    unsigned i;
    for (i = 0; i < sizeof map / sizeof map[0]; i++)
        if (isKeyPressed(*map[i].key))
            b |= map[i].buttons;
    return b;
}

const char *const *plat_control_lines(void) { return NULL; }
/* No pointer and no speaker: keyboard play and silence. */
bool plat_read_mouse(int *x, int *y) { (void)x; (void)y; return false; }
void plat_sound(int id, int volume) { (void)id; (void)volume; }

const char *plat_name(void) { return "TI-NSPIRE CX (NDLESS)"; }

/* ---------------------------------------------------------------- display */

void plat_present(const uint16_t *fb)
{
    lcd_blit((void *)fb, SCR_320x240_565);
    if (g_timer.mode != CLOCK_TIMER)
        rtc_pace_frame();
}

bool plat_quit_requested(void) { return false; }

/* ------------------------------------------------------------- lifecycle */

static char g_save_path[256];

const char *plat_save_path(void) { return g_save_path; }

/* Save next to the program (normally /documents/ndless/). */
static void init_save_path(const char *argv0)
{
    const char *slash = argv0 ? strrchr(argv0, '/') : NULL;
    size_t dir = slash ? (size_t)(slash - argv0) : 0;
    if (!slash || dir + 24 >= sizeof g_save_path) {
        strcpy(g_save_path, "/documents/ndless/AshenDepths.sav.tns");
        return;
    }
    memcpy(g_save_path, argv0, dir);
    strcpy(g_save_path + dir, "/AshenDepths.sav.tns");
}

bool plat_init(void)
{
    if (!lcd_init(SCR_320x240_565))
        return false;
    timer_start();
    return true;
}

void plat_shutdown(void)
{
    timer_stop();
    lcd_init(SCR_TYPE_INVALID); /* hand the screen back to the OS */
}

int main(int argc, char **argv)
{
    int rc;
    (void)argc;
    /* lcd_blit / lcd_init need Ndless >= r2004 (4.2+ era SDK). */
    assert_ndless_rev(2004);
    if (is_classic)
        return 1; /* monochrome / classic Nspire: not supported */
    init_save_path(argv[0]);
    if (!plat_init())
        return 1;
    rc = run_game();
    plat_shutdown();
    /* Wait for keys to be released so ESC doesn't leak into the OS. */
    while (any_key_pressed())
        msleep(10);
    return rc;
}
