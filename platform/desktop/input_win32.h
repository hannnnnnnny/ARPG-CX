/* input_win32.h - key bindings (settings.ini), gamepads and window settings. */
#ifndef AD_INPUT_WIN32_H
#define AD_INPUT_WIN32_H

#include "../../src/core/common.h"

/* Read settings.ini from the working directory (written with defaults if missing). */
void     input_load_settings(void);
/* BTN_* bits of the bound keys / of any connected XInput gamepad. */
uint32_t input_keys(void);
uint32_t input_pad(void);
/* Integer settings, e.g. ("window", "fullscreen"). */
int      input_setting(const char *section, const char *key, int fallback);
void     input_set_setting(const char *section, const char *key, int value);
/* Exposed for tests: key name -> virtual key (0 = unknown), and a comma list. */
int      input_key_code(const char *name);
int      input_parse_keys(const char *list, uint8_t *out, int max);

#endif
