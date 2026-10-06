/*
 * platform.h - the ONLY interface between game code and a target.
 *
 * Implemented by:
 *   platform/nspire/   TI-Nspire CX (Ndless): lcd_blit, isKeyPressed, SP804 timer
 *   platform/desktop/  Windows (Win32 GDI) simulator
 *   platform/headless/ scripted runner for tests/screenshots (no window)
 *
 * Game logic never sees calculator keycodes; platforms translate physical
 * keys into the logical BTN_* bits declared in input.h.
 */
#ifndef AD_PLATFORM_H
#define AD_PLATFORM_H

#include "common.h"

bool        plat_init(void);
void        plat_shutdown(void);
uint32_t    plat_read_buttons(void);          /* BTN_* bitmask, currently held */
void        plat_present(const uint16_t *fb); /* SCREEN_W x SCREEN_H RGB565 */
uint32_t    plat_time_us(void);               /* monotonic, wraps at 2^32 */
void        plat_wait(void);                  /* yield briefly when ahead of schedule */
bool        plat_quit_requested(void);        /* e.g. desktop window closed */
const char *plat_save_path(void);             /* full path of the save file */
/* NULL-terminated "ACTION|KEYS" lines for the Controls screen. */
const char *const *plat_control_lines(void);
const char *plat_name(void);                  /* e.g. "TI-NSPIRE CX" */
const char *plat_clock_desc(void);            /* timing source, for the FPS overlay */

#endif
