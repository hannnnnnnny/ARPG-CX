/*
 * main_win32.c - the Windows build (plain Win32 + GDI + waveOut, no SDL).
 *
 * Runs the same game code as the calculator, rendered at 640x480 (GFX_HD:
 * 2x pixel art, 12px CJK text) and scaled up by an integer factor. Adds
 * what a PC has: the mouse (click to move and attack, wheel to scroll),
 * skill keys 1-6, Q for potions, Space for auto battle, and synthesized
 * sound effects (audio_win32.c).
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "../../src/core/platform.h"
#include "../../src/input/input.h"
#include "../../src/game/runner.h"

#include "../../src/gfx/gfx.h"
#include "audio_win32.h"

static HWND      g_wnd;
static int       g_view_scale = 1, g_view_x, g_view_y;  /* where the frame sits in the window */
static int       g_wheel;                               /* accumulated wheel delta */
static uint32_t  g_last_buttons;
static uint32_t  g_rgb[FB_W * FB_H];
static bool      g_quit;
static LARGE_INTEGER g_freq, g_t0;
static char      g_save_path[MAX_PATH];

static LRESULT CALLBACK wnd_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CLOSE:
        g_quit = true;
        return 0;
    case WM_ERASEBKGND:
        return 1; /* we paint every pixel; avoids flicker */
    case WM_MOUSEWHEEL:
        g_wheel += GET_WHEEL_DELTA_WPARAM(wp);
        return 0;
    default:
        return DefWindowProcA(h, msg, wp, lp);
    }
}

static void pump_messages(void)
{
    MSG m;
    while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
}

bool plat_init(void)
{
    WNDCLASSA wc;
    /* 2x the 640x480 buffer when the screen has room, else 1x. */
    int zoom = GetSystemMetrics(SM_CYSCREEN) >= FB_H * 2 + 120 ? 2 : 1;
    RECT r = { 0, 0, FB_W * zoom, FB_H * zoom };
    DWORD style = WS_OVERLAPPEDWINDOW;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "AshenDepthsSim";
    if (!RegisterClassA(&wc))
        return false;
    AdjustWindowRect(&r, style, FALSE);
    g_wnd = CreateWindowA(wc.lpszClassName, "Ashen Depths", style,
                          CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                          NULL, NULL, wc.hInstance, NULL);
    if (!g_wnd)
        return false;
    ShowWindow(g_wnd, SW_SHOW);
    QueryPerformanceFrequency(&g_freq);
    QueryPerformanceCounter(&g_t0);
    timeBeginPeriod(1); /* 1 ms Sleep granularity for frame pacing */
    audio_init();       /* without a sound device the game simply stays silent */
    return true;
}

void plat_shutdown(void)
{
    audio_shutdown();
    timeEndPeriod(1);
    if (g_wnd)
        DestroyWindow(g_wnd);
}

static bool key(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

/* The wheel arrives as messages; each notch becomes a one-read pulse. */
static uint32_t wheel_bits(void)
{
    uint32_t b = 0;
    if (g_wheel >= WHEEL_DELTA && !(g_last_buttons & BTN_WHEEL_UP)) {
        b = BTN_WHEEL_UP;
        g_wheel -= WHEEL_DELTA;
    } else if (g_wheel <= -WHEEL_DELTA && !(g_last_buttons & BTN_WHEEL_DOWN)) {
        b = BTN_WHEEL_DOWN;
        g_wheel += WHEEL_DELTA;
    }
    return b;
}

static uint32_t mouse_bits(void)
{
    int x, y;
    uint32_t b = 0;
    if (!plat_read_mouse(&x, &y))
        return 0;
    if (key(VK_LBUTTON)) b |= BTN_MOUSE_L;
    if (key(VK_RBUTTON)) b |= BTN_MOUSE_R;
    return b | wheel_bits();
}

uint32_t plat_read_buttons(void)
{
    uint32_t b = 0;
    int s;
    pump_messages();
    if (GetForegroundWindow() != g_wnd)
        return g_last_buttons = 0;
    if (key(VK_LEFT) || key('A'))              b |= BTN_LEFT;
    if (key(VK_RIGHT) || key('D'))             b |= BTN_RIGHT;
    if (key(VK_UP) || key('W'))                b |= BTN_UP;
    if (key(VK_DOWN) || key('S'))              b |= BTN_DOWN;
    if (key(VK_RETURN))                        b |= BTN_OK;
    if (key(VK_SPACE))                         b |= BTN_OK | BTN_AUTO;
    if (key(VK_ESCAPE))                        b |= BTN_BACK;
    if (key(VK_TAB) || key('M'))               b |= BTN_TAB;
    if (key(VK_DELETE) || key('X'))            b |= BTN_ALT;
    if (key(VK_CONTROL) || key('L'))           b |= BTN_LOCK;
    if (key('F') || key(VK_F3))                b |= BTN_DEBUG;
    if (key('Q'))                              b |= BTN_POTION;
    for (s = 0; s < 6; s++)
        if (key('1' + s))
            b |= BTN_SKILL1 << s;
    b |= mouse_bits();
    return g_last_buttons = b;
}

bool plat_read_mouse(int *x, int *y)
{
    POINT pt;
    int fx, fy;
    if (GetForegroundWindow() != g_wnd || !GetCursorPos(&pt) || !ScreenToClient(g_wnd, &pt))
        return false;
    fx = (pt.x - g_view_x) / g_view_scale;
    fy = (pt.y - g_view_y) / g_view_scale;
    if (pt.x < g_view_x || pt.y < g_view_y || fx >= FB_W || fy >= FB_H)
        return false;
    *x = fx / GFX_S;
    *y = fy / GFX_S;
    return true;
}

void plat_sound(int id, int volume) { audio_play(id, volume); }

void plat_present(const uint16_t *fb)
{
    BITMAPINFO bi;
    RECT rc;
    HDC dc;
    int i, cw, ch, scale, ox, oy;
    for (i = 0; i < FB_W * FB_H; i++) {
        uint16_t c = fb[i];
        uint32_t r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
        g_rgb[i] = ((r * 255 / 31) << 16) | ((g * 255 / 63) << 8) | (b * 255 / 31);
    }
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = FB_W;
    bi.bmiHeader.biHeight = -FB_H; /* top-down */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    GetClientRect(g_wnd, &rc);
    cw = rc.right;
    ch = rc.bottom;
    /* Largest integer scale that fits keeps pixels crisp. */
    scale = MAX(1, MIN(cw / FB_W, ch / FB_H));
    ox = (cw - FB_W * scale) / 2;
    oy = (ch - FB_H * scale) / 2;
    g_view_scale = scale;
    g_view_x = ox;
    g_view_y = oy;
    dc = GetDC(g_wnd);
    PatBlt(dc, 0, 0, cw, oy, BLACKNESS);
    PatBlt(dc, 0, oy + FB_H * scale, cw, ch, BLACKNESS);
    PatBlt(dc, 0, 0, ox, ch, BLACKNESS);
    PatBlt(dc, ox + FB_W * scale, 0, cw, ch, BLACKNESS);
    StretchDIBits(dc, ox, oy, FB_W * scale, FB_H * scale, 0, 0, FB_W, FB_H,
                  g_rgb, &bi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(g_wnd, dc);
    pump_messages();
}

uint32_t plat_time_us(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (uint32_t)((uint64_t)(t.QuadPart - g_t0.QuadPart) * 1000000u / (uint64_t)g_freq.QuadPart);
}

void plat_wait(void)
{
    pump_messages();
    Sleep(1);
}

bool plat_quit_requested(void) { return g_quit; }

const char *plat_save_path(void) { return g_save_path; }
const char *plat_clock_desc(void) { return "CLOCK: QPC"; }
const char *plat_name(void) { return "WINDOWS DESKTOP SIMULATOR"; }

const char *const *plat_control_lines(void) { return NULL; }

static void init_save_path(void)
{
    char *slash;
    DWORD n = GetModuleFileNameA(NULL, g_save_path, MAX_PATH);
    if (n == 0 || n >= MAX_PATH - 20 || !(slash = strrchr(g_save_path, '\\'))) {
        strcpy(g_save_path, "AshenDepths.sav");
        return;
    }
    strcpy(slash + 1, "AshenDepths.sav");
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    int rc;
    (void)inst; (void)prev; (void)cmd; (void)show;
    init_save_path();
    if (!plat_init()) {
        MessageBoxA(NULL, "Could not create the game window.", "Ashen Depths", MB_ICONERROR);
        return 1;
    }
    rc = run_game();
    plat_shutdown();
    return rc;
}
