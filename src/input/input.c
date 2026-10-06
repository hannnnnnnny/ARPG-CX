#include "input.h"

#define REPEAT_DELAY 12 /* ticks before a held arrow starts repeating */
#define REPEAT_RATE   3

void input_init(Input *in)
{
    in->held = in->pressed = in->released = in->blocked = in->raw_prev = 0;
    in->repeat_mask = BTN_UP | BTN_DOWN | BTN_LEFT | BTN_RIGHT;
    in->repeat_timer = 0;
}

void input_feed(Input *in, uint32_t raw)
{
    uint32_t rep;
    in->blocked &= raw;
    in->pressed  = raw & ~in->raw_prev & ~in->blocked;
    in->released = ~raw & in->raw_prev;
    in->held     = raw & ~in->blocked;
    /* Held arrows repeat so long menus can be scrolled. */
    rep = in->held & in->repeat_mask;
    if (rep && (rep & in->raw_prev) == rep) {
        if (++in->repeat_timer >= REPEAT_DELAY) {
            in->pressed |= rep;
            in->repeat_timer = REPEAT_DELAY - REPEAT_RATE;
        }
    } else {
        in->repeat_timer = 0;
    }
    in->raw_prev = raw;
}

void input_block_held(Input *in)
{
    in->blocked |= in->raw_prev;
    in->held = 0;
    in->pressed = 0;
}
