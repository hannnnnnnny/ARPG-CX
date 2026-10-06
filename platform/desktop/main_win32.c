/*
 * main_win32.c - Windows desktop simulator (plain Win32 + GDI, no SDL).
 *
 * Runs the exact same game code as the calculator at the same 320x240
 * resolution, scaled up by an integer factor. Used to iterate on movement,
 * levels and enemies without copying to the calculator after every change.
 * Desktop performance says nothing about calculator performance; use the
 * in-game FPS overlay (F key) on hardware for that.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <string.h>
#include "../../src/core/platform.h"
#include "../../src/input/input.h"
#include "../../src/game/runner.h"

#define SCALE 3

static HWND      g_wnd;
static uint32_t  g_rgb[SCREEN_W * SCREEN_H];
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
    RECT r = { 0, 0, SCREEN_W * SCALE, SCREEN_H * SCALE };
    DWORD style = WS_OVERLAPPEDWINDOW;
    memset(&wc, 0, sizeof wc);
    wc.lpfnWndProc = wnd_proc;
    wc.hInstance = GetModuleHandleA(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "AshenDepthsSim";
    if (!RegisterClassA(&wc))
        return false;
    AdjustWindowRect(&r, style, FALSE);
    g_wnd = CreateWindowA(wc.lpszClassName, "Ashen Depths - desktop simulator (320x240)", style,
                          CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                          NULL, NULL, wc.hInstance, NULL);
    if (!g_wnd)
        return false;
    ShowWindow(g_wnd, SW_SHOW);
    QueryPerformanceFrequency(&g_freq);
    QueryPerformanceCounter(&g_t0);
    timeBeginPeriod(1); /* 1 ms Sleep granularity for frame pacing */
    return true;
}

void plat_shutdown(void)
{
    timeEndPeriod(1);
    if (g_wnd)
        DestroyWindow(g_wnd);
}

static bool key(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

uint32_t plat_read_buttons(void)
{
    uint32_t b = 0;
    pump_messages();
    if (GetForegroundWindow() != g_wnd)
        return 0;
    if (key(VK_LEFT) || key('A'))              b |= BTN_LEFT;
    if (key(VK_RIGHT) || key('D'))             b |= BTN_RIGHT;
    if (key(VK_UP) || key('W'))                b |= BTN_UP;
    if (key(VK_DOWN) || key('S'))              b |= BTN_DOWN;
    if (key(VK_RETURN) || key(VK_SPACE))       b |= BTN_OK;
    if (key(VK_ESCAPE))                        b |= BTN_BACK;
    if (key(VK_TAB) || key('M'))               b |= BTN_TAB;
    if (key(VK_DELETE) || key('X'))            b |= BTN_ALT;
    if (key(VK_CONTROL) || key('L'))           b |= BTN_LOCK;
    if (key('F') || key(VK_F3))                b |= BTN_DEBUG;
    return b;
}

void plat_present(const uint16_t *fb)
{
    BITMAPINFO bi;
    RECT rc;
    HDC dc;
    int i, cw, ch, scale, ox, oy;
    for (i = 0; i < SCREEN_W * SCREEN_H; i++) {
        uint16_t c = fb[i];
        uint32_t r = (c >> 11) & 31, g = (c >> 5) & 63, b = c & 31;
        g_rgb[i] = ((r * 255 / 31) << 16) | ((g * 255 / 63) << 8) | (b * 255 / 31);
    }
    memset(&bi, 0, sizeof bi);
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = SCREEN_W;
    bi.bmiHeader.biHeight = -SCREEN_H; /* top-down */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    GetClientRect(g_wnd, &rc);
    cw = rc.right;
    ch = rc.bottom;
    /* Largest integer scale that fits keeps pixels crisp. */
    scale = MAX(1, MIN(cw / SCREEN_W, ch / SCREEN_H));
    ox = (cw - SCREEN_W * scale) / 2;
    oy = (ch - SCREEN_H * scale) / 2;
    dc = GetDC(g_wnd);
    PatBlt(dc, 0, 0, cw, oy, BLACKNESS);
    PatBlt(dc, 0, oy + SCREEN_H * scale, cw, ch, BLACKNESS);
    PatBlt(dc, 0, 0, ox, ch, BLACKNESS);
    PatBlt(dc, ox + SCREEN_W * scale, 0, cw, ch, BLACKNESS);
    StretchDIBits(dc, ox, oy, SCREEN_W * scale, SCREEN_H * scale, 0, 0, SCREEN_W, SCREEN_H,
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
