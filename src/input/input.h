/*
 * input.h - logical input abstraction.
 *
 * Platforms map physical keys to these bits; game code asks semantic
 * questions (in_ok, in_tab, ...) so keycodes never leak into game logic.
 */
#ifndef AD_INPUT_H
#define AD_INPUT_H

#include "../core/common.h"

enum {
    BTN_LEFT  = 1u << 0,
    BTN_RIGHT = 1u << 1,
    BTN_UP    = 1u << 2,
    BTN_DOWN  = 1u << 3,
    BTN_OK    = 1u << 4, /* enter / click */
    BTN_BACK  = 1u << 5, /* esc */
    BTN_TAB   = 1u << 6, /* tab: next menu page */
    BTN_ALT   = 1u << 7, /* del: salvage / secondary action */
    BTN_LOCK  = 1u << 8, /* ctrl: lock item */
    BTN_DEBUG = 1u << 9, /* F: performance overlay */
    /* Desktop: playing the hero by hand, and the mouse. */
    BTN_MOUSE_L = 1u << 10,
    BTN_MOUSE_R = 1u << 11,
    BTN_SKILL1  = 1u << 12, /* 1..6: skill bar slots (BTN_SKILL1 << slot) */
    BTN_POTION  = 1u << 18, /* Q */
    BTN_AUTO    = 1u << 19, /* space: auto battle on/off */
    BTN_WHEEL_UP   = 1u << 20,
    BTN_WHEEL_DOWN = 1u << 21
};

typedef struct {
    uint32_t held, pressed, released, blocked, raw_prev;
    uint32_t repeat_mask; /* buttons that auto-repeat while held */
    int      repeat_timer;
    bool     mouse;       /* a pointer is available (desktop) */
    int      mx, my;      /* pointer in logical 320x240 pixels */
} Input;

void input_init(Input *in);
/* Call once per logic tick with the platform's raw button state. */
void input_feed(Input *in, uint32_t raw);
/* Ignore everything currently held until released (screen changes). */
void input_block_held(Input *in);

static inline bool in_pressed(const Input *in, uint32_t b) { return (in->pressed & b) != 0; }
static inline bool in_held(const Input *in, uint32_t b)    { return (in->held & b) != 0; }
static inline bool in_up(const Input *in)    { return in_pressed(in, BTN_UP); }
static inline bool in_down(const Input *in)  { return in_pressed(in, BTN_DOWN); }
static inline bool in_left(const Input *in)  { return in_pressed(in, BTN_LEFT); }
static inline bool in_right(const Input *in) { return in_pressed(in, BTN_RIGHT); }
static inline bool in_ok(const Input *in)    { return in_pressed(in, BTN_OK); }
static inline bool in_back(const Input *in)  { return in_pressed(in, BTN_BACK); }
static inline bool in_tab(const Input *in)   { return in_pressed(in, BTN_TAB); }
static inline bool in_alt(const Input *in)   { return in_pressed(in, BTN_ALT); }
static inline bool in_lock(const Input *in)  { return in_pressed(in, BTN_LOCK); }
static inline bool in_click(const Input *in) { return in_pressed(in, BTN_MOUSE_L); }

#endif
