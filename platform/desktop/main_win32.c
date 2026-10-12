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
#include <wchar.h>
#include "../../src/core/platform.h"
#include "../../src/input/input.h"
#include "../../src/game/runner.h"

#include "../../src/gfx/gfx.h"
#include "audio_win32.h"
#include "crash_win32.h"
#include "music_win32.h"
#include "input_win32.h"

static HWND      g_wnd;
static int       g_view_scale = 1, g_view_x, g_view_y;  /* where the frame sits in the window */
static int       g_wheel;                               /* accumulated wheel delta */
static uint32_t  g_last_buttons;
static uint32_t  g_rgb[FB_W * FB_H];
static bool      g_quit;
static LARGE_INTEGER g_freq, g_t0;
static char      g_save_path[MAX_PATH];
static uint32_t  g_side_rgb[FB_W * FB_H];
static bool      g_side_ready;                          /* side panels arrived for this frame */
static bool      g_fullscreen;
static WINDOWPLACEMENT g_windowed;

#define SIDE_W 160                                      /* one side panel, in framebuffer pixels */

static int window_zoom(void);

/* Borderless fullscreen on the window's monitor, and back to the old place. */
static void toggle_fullscreen(void)
{
    DWORD style = (DWORD)GetWindowLongA(g_wnd, GWL_STYLE);
    MONITORINFO mi;
    mi.cbSize = sizeof mi;
    g_windowed.length = sizeof g_windowed;
    if (!g_fullscreen) {
        if (!GetWindowPlacement(g_wnd, &g_windowed)
            || !GetMonitorInfoA(MonitorFromWindow(g_wnd, MONITOR_DEFAULTTOPRIMARY), &mi))
            return;
        SetWindowLongA(g_wnd, GWL_STYLE, (LONG)(style & ~WS_OVERLAPPEDWINDOW));
        SetWindowPos(g_wnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        SetWindowLongA(g_wnd, GWL_STYLE, (LONG)(style | WS_OVERLAPPEDWINDOW));
        SetWindowPlacement(g_wnd, &g_windowed);
        SetWindowPos(g_wnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
    g_fullscreen = !g_fullscreen;
    input_set_setting("window", "fullscreen", g_fullscreen);
}

/* Where the window was when the game last closed (windowed size and place). */
static void restore_window(void)
{
    int w = input_setting("window", "width", 0), h = input_setting("window", "height", 0);
    if (w >= 320 && h >= 240)
        SetWindowPos(g_wnd, NULL, input_setting("window", "x", 0), input_setting("window", "y", 0), w, h,
                     SWP_NOZORDER | SWP_NOOWNERZORDER);
    if (input_setting("window", "fullscreen", 0))
        toggle_fullscreen();
}

static void remember_window(void)
{
    WINDOWPLACEMENT wp;
    const RECT *r;
    wp.length = sizeof wp;
    if (!GetWindowPlacement(g_wnd, &wp))
        return;
    r = g_fullscreen ? &g_windowed.rcNormalPosition : &wp.rcNormalPosition;
    input_set_setting("window", "x", r->left);
    input_set_setting("window", "y", r->top);
    input_set_setting("window", "width", r->right - r->left);
    input_set_setting("window", "height", r->bottom - r->top);
}

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
    case WM_KEYDOWN:
        if (wp == VK_F11 && !(lp & (1 << 30)))       /* bit 30: auto-repeat */
            toggle_fullscreen();
        return 0;
    case WM_SYSKEYDOWN:                               /* Alt+Enter, without the system beep */
        if (wp == VK_RETURN) {
            if (!(lp & (1 << 30)))
                toggle_fullscreen();
            return 0;
        }
        return DefWindowProcA(h, msg, wp, lp);
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
    int zoom = window_zoom();
    RECT r = { 0, 0, FB_W * zoom, FB_H * zoom };
    DWORD style = WS_OVERLAPPEDWINDOW;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIconA(wc.hInstance, MAKEINTRESOURCEA(1));   /* app.rc; NULL falls back to the default */
    wc.lpszClassName = "AshenDepthsSim";
    if (!RegisterClassA(&wc))
        return false;
    AdjustWindowRect(&r, style, FALSE);
    g_wnd = CreateWindowA(wc.lpszClassName, "Ashen Depths", style,
                          CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                          NULL, NULL, wc.hInstance, NULL);
    if (!g_wnd)
        return false;
    restore_window();
    ShowWindow(g_wnd, SW_SHOW);
    QueryPerformanceFrequency(&g_freq);
    QueryPerformanceCounter(&g_t0);
    timeBeginPeriod(1); /* 1 ms Sleep granularity for frame pacing */
    audio_init();       /* without a sound device the game simply stays silent */
    return true;
}

void plat_shutdown(void)
{
    if (g_wnd)
        remember_window();
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
    pump_messages();
    if (GetForegroundWindow() != g_wnd)
        return g_last_buttons = 0;
    return g_last_buttons = input_keys() | input_pad() | mouse_bits();   /* settings.ini bindings, gamepad */
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
void plat_music(int track, int volume) { music_set(track, volume); }

/* RGB565 -> 32-bit for GDI, the first 'cols' columns of each row. */
static void to_rgb32(const uint16_t *fb, uint32_t *out, int cols)
{
    int x, y;
    for (y = 0; y < FB_H; y++)
        for (x = 0; x < cols; x++) {
            uint16_t c = fb[y * FB_W + x];
            uint32_t r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
            out[y * FB_W + x] = ((r * 255 / 31) << 16) | ((g * 255 / 63) << 8) | (b * 255 / 31);
        }
}

/* Largest integer scale that fits keeps pixels crisp; the frame is centred. */
static void layout(int cw, int ch, int *scale, int *ox, int *oy)
{
    *scale = MAX(1, MIN(cw / FB_W, ch / FB_H));
    *ox = (cw - FB_W * *scale) / 2;
    *oy = (ch - FB_H * *scale) / 2;
}

bool plat_side_panels(void)
{
    RECT rc;
    int scale, ox, oy;
    GetClientRect(g_wnd, &rc);
    layout(rc.right, rc.bottom, &scale, &ox, &oy);
    return ox >= SIDE_W;
}

void plat_present_side(const uint16_t *fb)
{
    to_rgb32(fb, g_side_rgb, SIDE_W * 2);
    g_side_ready = true;
}

/* Each panel takes the widest whole scale that fits beside the frame. */
static void blit_side(HDC dc, const BITMAPINFO *bi, int ox, int oy, int scale, int cw)
{
    int ps = MIN(scale, ox / SIDE_W), ph = FB_H * ps, py = oy + (FB_H * scale - ph) / 2;
    StretchDIBits(dc, ox - SIDE_W * ps, py, SIDE_W * ps, ph, 0, 0, SIDE_W, FB_H,
                  g_side_rgb, bi, DIB_RGB_COLORS, SRCCOPY);
    StretchDIBits(dc, cw - ox, py, SIDE_W * ps, ph, SIDE_W, 0, SIDE_W, FB_H,
                  g_side_rgb, bi, DIB_RGB_COLORS, SRCCOPY);
}

void plat_present(const uint16_t *fb)
{
    BITMAPINFO bi;
    RECT rc;
    HDC dc;
    int cw, ch, scale, ox, oy;
    to_rgb32(fb, g_rgb, FB_W);
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
    layout(cw, ch, &scale, &ox, &oy);
    g_view_scale = scale;
    g_view_x = ox;
    g_view_y = oy;
    dc = GetDC(g_wnd);
    PatBlt(dc, 0, 0, cw, oy, BLACKNESS);
    PatBlt(dc, 0, oy + FB_H * scale, cw, ch, BLACKNESS);
    PatBlt(dc, 0, 0, ox, ch, BLACKNESS);
    PatBlt(dc, ox + FB_W * scale, 0, cw, ch, BLACKNESS);
    StretchDIBits(dc, ox, oy, FB_W * scale, FB_H * scale, 0, 0, FB_W, FB_H, g_rgb, &bi, DIB_RGB_COLORS, SRCCOPY);
    if (g_side_ready && ox >= SIDE_W)
        blit_side(dc, &bi, ox, oy, scale, cw);
    g_side_ready = false;
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

/* Saves live next to the exe. The core opens files with narrow fopen, which
 * mangles folder names outside the ANSI code page (e.g. a Chinese folder on
 * an English Windows), so move into the exe's folder with the wide API and
 * hand the core a plain relative file name. */
static void init_save_path(void)
{
    wchar_t dir[MAX_PATH];
    wchar_t *slash;
    DWORD n = GetModuleFileNameW(NULL, dir, MAX_PATH);
    strcpy(g_save_path, "AshenDepths.sav");
    if (n == 0 || n >= MAX_PATH || !(slash = wcsrchr(dir, L'\\')))
        return;
    *slash = 0;
    SetCurrentDirectoryW(dir);
}

/* Largest integer zoom whose window still fits the screen with its frame. */
static int window_zoom(void)
{
    int zw = (GetSystemMetrics(SM_CXSCREEN) - 40) / FB_W;
    int zh = (GetSystemMetrics(SM_CYSCREEN) - 120) / FB_H;
    return MAX(1, MIN(zw, zh));
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmd, int show)
{
    int rc;
    (void)inst; (void)prev; (void)cmd; (void)show;
    /* Without this, Windows bitmap-stretches the window at 125%/150% display
     * scaling and the pixel art turns blurry. */
    SetProcessDPIAware();
    init_save_path();
    crash_install();
    input_load_settings();      /* settings.ini next to the exe: key bindings, window */            /* after init_save_path, so crash.txt lands next to the exe */
    if (!plat_init()) {
        MessageBoxA(NULL, "Could not create the game window.", "Ashen Depths", MB_ICONERROR);
        return 1;
    }
    rc = run_game();
    plat_shutdown();
    return rc;
}
