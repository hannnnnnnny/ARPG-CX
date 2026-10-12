/*
 * input_win32.c - keyboard bindings from settings.ini, Xbox-style gamepads
 * through XInput, and the few window settings the desktop remembers.
 *
 * settings.ini lives next to the exe and is written with the defaults on
 * first start, with a comment explaining the key names, so players can
 * rebind keys in any text editor. A missing or broken entry falls back to
 * its default. XInput is loaded at run time, so the game still starts on
 * systems without it and simply has no gamepad.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input_win32.h"
#include "../../src/input/input.h"

#define MAX_KEYS 4

typedef struct { const char *name; uint32_t bits; const char *def; } Action;

static const Action actions[] = {
    { "up", BTN_UP, "W,UP" },           { "down", BTN_DOWN, "S,DOWN" },
    { "left", BTN_LEFT, "A,LEFT" },     { "right", BTN_RIGHT, "D,RIGHT" },
    { "ok", BTN_OK, "ENTER" },          { "auto_battle", BTN_OK | BTN_AUTO, "SPACE" },
    { "back", BTN_BACK, "ESC" },        { "next_page", BTN_TAB, "TAB,M" },
    { "salvage", BTN_ALT, "DELETE,X" }, { "lock", BTN_LOCK, "CTRL,L" },
    { "fps", BTN_DEBUG, "F,F3" },       { "potion", BTN_POTION, "Q" },
    { "dodge", BTN_DODGE, "SHIFT" },
    { "skill1", BTN_SKILL1 << 0, "1" }, { "skill2", BTN_SKILL1 << 1, "2" },
    { "skill3", BTN_SKILL1 << 2, "3" }, { "skill4", BTN_SKILL1 << 3, "4" },
    { "skill5", BTN_SKILL1 << 4, "5" }, { "skill6", BTN_SKILL1 << 5, "6" },
};
#define ACTIONS (int)(sizeof actions / sizeof actions[0])

static uint8_t g_bind[ACTIONS][MAX_KEYS];
static char g_ini[MAX_PATH];

/* ------------------------------------------------------------ key names */

static const struct { const char *name; uint8_t vk; } named[] = {
    { "UP", VK_UP }, { "DOWN", VK_DOWN }, { "LEFT", VK_LEFT }, { "RIGHT", VK_RIGHT },
    { "SPACE", VK_SPACE }, { "ENTER", VK_RETURN }, { "ESC", VK_ESCAPE }, { "TAB", VK_TAB },
    { "DELETE", VK_DELETE }, { "BACKSPACE", VK_BACK }, { "CTRL", VK_CONTROL }, { "SHIFT", VK_SHIFT },
    { "ALT", VK_MENU }, { "HOME", VK_HOME }, { "END", VK_END }, { "PAGEUP", VK_PRIOR }, { "PAGEDOWN", VK_NEXT },
};

/* "W", "7", "F5", "SPACE" ... -> virtual key, 0 if unknown. */
int input_key_code(const char *name)
{
    size_t i;
    if (name[0] && !name[1] && ((name[0] >= 'A' && name[0] <= 'Z') || (name[0] >= '0' && name[0] <= '9')))
        return name[0];
    if (name[0] == 'F' && name[1] >= '1' && name[1] <= '9') {
        int n = atoi(name + 1);
        return n >= 1 && n <= 12 ? VK_F1 + n - 1 : 0;
    }
    for (i = 0; i < sizeof named / sizeof named[0]; i++)
        if (!strcmp(name, named[i].name))
            return named[i].vk;
    return 0;
}

/* "W, up" -> up to max virtual keys (case and spaces ignored); returns how many. */
int input_parse_keys(const char *list, uint8_t *out, int max)
{
    char word[16];
    int n = 0, len = 0;
    for (;; list++) {
        char c = *list;
        if (c == ',' || c == 0) {
            int vk;
            word[len] = 0;
            if (len && n < max && (vk = input_key_code(word)) != 0)
                out[n++] = (uint8_t)vk;
            len = 0;
            if (!c)
                return n;
        } else if (c != ' ' && c != '\t' && len < (int)sizeof word - 1) {
            word[len++] = (char)(c >= 'a' && c <= 'z' ? c - 32 : c);
        }
    }
}

/* ------------------------------------------------------------- settings */

static void write_defaults(void)
{
    FILE *f = fopen(g_ini, "w");
    int i;
    if (!f)
        return;
    fputs("; Ashen Depths desktop settings. Delete this file to get the defaults back.\n"
          "; Keys: one or more per action, separated by commas. Names: A-Z, 0-9, F1-F12,\n"
          "; UP DOWN LEFT RIGHT SPACE ENTER ESC TAB DELETE BACKSPACE CTRL SHIFT ALT\n"
          "; HOME END PAGEUP PAGEDOWN. Gamepads (Xbox layout) work without setup.\n\n[keys]\n", f);
    for (i = 0; i < ACTIONS; i++)
        fprintf(f, "%s=%s\n", actions[i].name, actions[i].def);
    fputs("\n[window]\nfullscreen=0\n", f);
    fclose(f);
}

void input_load_settings(void)
{
    char buf[64];
    int i;
    GetFullPathNameA("settings.ini", sizeof g_ini, g_ini, NULL);
    if (GetFileAttributesA(g_ini) == INVALID_FILE_ATTRIBUTES)
        write_defaults();
    for (i = 0; i < ACTIONS; i++) {
        memset(g_bind[i], 0, sizeof g_bind[i]);
        GetPrivateProfileStringA("keys", actions[i].name, actions[i].def, buf, sizeof buf, g_ini);
        if (!input_parse_keys(buf, g_bind[i], MAX_KEYS))           /* nothing usable: the default */
            input_parse_keys(actions[i].def, g_bind[i], MAX_KEYS);
    }
}

int input_setting(const char *section, const char *key, int fallback)
{
    return (int)GetPrivateProfileIntA(section, key, fallback, g_ini);
}

void input_set_setting(const char *section, const char *key, int value)
{
    char buf[16];
    snprintf(buf, sizeof buf, "%d", value);
    WritePrivateProfileStringA(section, key, buf, g_ini);
}

/* ------------------------------------------------------------- reading */

static bool down(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

uint32_t input_keys(void)
{
    uint32_t b = 0;
    int i, k;
    for (i = 0; i < ACTIONS; i++)
        for (k = 0; k < MAX_KEYS && g_bind[i][k]; k++)
            if (down(g_bind[i][k]) && !(g_bind[i][k] == VK_RETURN && down(VK_MENU)))  /* Alt+Enter: fullscreen */
                b |= actions[i].bits;
    return b;
}

typedef DWORD (WINAPI *XInputGetStateFn)(DWORD, XINPUT_STATE *);
static XInputGetStateFn g_xinput;

static void load_xinput(void)
{
    static bool tried;
    static const char *const dlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
    size_t i;
    if (tried)
        return;
    tried = true;
    for (i = 0; i < sizeof dlls / sizeof dlls[0] && !g_xinput; i++) {
        HMODULE m = LoadLibraryA(dlls[i]);
        if (m)
            g_xinput = (XInputGetStateFn)(void *)GetProcAddress(m, "XInputGetState");
    }
}

/* Xbox layout: stick or d-pad moves, A confirms, B goes back, X Y LB RB
 * LT RT are skill slots 1-6, B also rolls in battle, View pages through menus, Menu confirms,
 * left stick click drinks a potion, right stick click toggles auto battle. */
static uint32_t pad_bits(const XINPUT_GAMEPAD *p)
{
    static const struct { WORD mask; uint32_t bits; } map[] = {
        { XINPUT_GAMEPAD_DPAD_UP, BTN_UP }, { XINPUT_GAMEPAD_DPAD_DOWN, BTN_DOWN },
        { XINPUT_GAMEPAD_DPAD_LEFT, BTN_LEFT }, { XINPUT_GAMEPAD_DPAD_RIGHT, BTN_RIGHT },
        { XINPUT_GAMEPAD_A, BTN_OK }, { XINPUT_GAMEPAD_B, BTN_BACK | BTN_DODGE }, { XINPUT_GAMEPAD_START, BTN_OK },
        { XINPUT_GAMEPAD_BACK, BTN_TAB }, { XINPUT_GAMEPAD_X, BTN_SKILL1 }, { XINPUT_GAMEPAD_Y, BTN_SKILL1 << 1 },
        { XINPUT_GAMEPAD_LEFT_SHOULDER, BTN_SKILL1 << 2 }, { XINPUT_GAMEPAD_RIGHT_SHOULDER, BTN_SKILL1 << 3 },
        { XINPUT_GAMEPAD_LEFT_THUMB, BTN_POTION }, { XINPUT_GAMEPAD_RIGHT_THUMB, BTN_OK | BTN_AUTO },
    };
    enum { STICK = 16000, TRIGGER = 100 };
    uint32_t b = 0;
    size_t i;
    for (i = 0; i < sizeof map / sizeof map[0]; i++)
        if (p->wButtons & map[i].mask)
            b |= map[i].bits;
    if (p->sThumbLX < -STICK) b |= BTN_LEFT;
    if (p->sThumbLX > STICK)  b |= BTN_RIGHT;
    if (p->sThumbLY > STICK)  b |= BTN_UP;
    if (p->sThumbLY < -STICK) b |= BTN_DOWN;
    if (p->bLeftTrigger > TRIGGER)  b |= BTN_SKILL1 << 4;
    if (p->bRightTrigger > TRIGGER) b |= BTN_SKILL1 << 5;
    return b;
}

uint32_t input_pad(void)
{
    XINPUT_STATE s;
    DWORD i;
    uint32_t b = 0;
    load_xinput();
    for (i = 0; g_xinput && i < XUSER_MAX_COUNT; i++)
        if (g_xinput(i, &s) == ERROR_SUCCESS)
            b |= pad_bits(&s.Gamepad);
    return b;
}
