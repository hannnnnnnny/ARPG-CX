/*
 * crash_win32.c - when the desktop build crashes, write crash.txt next to
 * the exe and tell the player where it is.
 *
 * The report holds what it took a player and WinDbg to work out for the
 * Intel crash: the exception code, the faulting address as an offset into
 * the exe (so it maps straight onto a disassembly), the CPU vendor and
 * model, and the Windows build. The handler runs on a possibly broken
 * process, so it only uses static buffers and plain Win32 calls.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cpuid.h>
#include <stdio.h>
#include <string.h>
#include "../../src/core/version.h"
#include "crash_win32.h"

static char g_report[2048];

static const char *code_name(DWORD code)
{
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:      return "ACCESS_VIOLATION";
    case EXCEPTION_ILLEGAL_INSTRUCTION:   return "ILLEGAL_INSTRUCTION (CPU lacks an instruction)";
    case EXCEPTION_STACK_OVERFLOW:        return "STACK_OVERFLOW";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:    return "INT_DIVIDE_BY_ZERO";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "ARRAY_BOUNDS_EXCEEDED";
    case EXCEPTION_PRIV_INSTRUCTION:      return "PRIV_INSTRUCTION";
    case EXCEPTION_IN_PAGE_ERROR:         return "IN_PAGE_ERROR";
    default:                              return "OTHER";
    }
}

/* "GenuineIntel / Intel(R) Core(TM) i7-14700HX" */
static void cpu_desc(char *out, size_t cap)
{
    unsigned r[12] = { 0 }, a, b, c, d, i;
    char vendor[13] = { 0 }, brand[49] = { 0 };
    if (__get_cpuid(0, &a, &b, &c, &d)) {
        memcpy(vendor, &b, 4);
        memcpy(vendor + 4, &d, 4);
        memcpy(vendor + 8, &c, 4);
    }
    for (i = 0; i < 3; i++)
        if (!__get_cpuid(0x80000002u + i, &r[i * 4], &r[i * 4 + 1], &r[i * 4 + 2], &r[i * 4 + 3]))
            break;
    memcpy(brand, r, 48);
    for (i = 47; i > 0 && (brand[i] == ' ' || brand[i] == 0); i--)   /* the brand string is space-padded */
        brand[i] = 0;
    snprintf(out, cap, "%s / %s", vendor, brand + strspn(brand, " "));
}

/* RtlGetVersion reports the real build; GetVersionEx lies to unmanifested apps. */
static void os_desc(char *out, size_t cap)
{
    typedef LONG (WINAPI *RtlGetVersionFn)(OSVERSIONINFOW *);
    OSVERSIONINFOW v;
    HMODULE nt = GetModuleHandleA("ntdll.dll");
    RtlGetVersionFn fn = nt ? (RtlGetVersionFn)(void *)GetProcAddress(nt, "RtlGetVersion") : NULL;
    memset(&v, 0, sizeof v);
    v.dwOSVersionInfoSize = sizeof v;
    if (fn && fn(&v) == 0)
        snprintf(out, cap, "Windows %s (%lu.%lu build %lu)",
                 v.dwMajorVersion == 10 && v.dwBuildNumber >= 22000 ? "11" : v.dwMajorVersion == 10 ? "10" : "",
                 v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber);
    else
        snprintf(out, cap, "Windows (version unknown)");
}

static int describe(const EXCEPTION_RECORD *er, char *out, size_t cap)
{
    uintptr_t base = (uintptr_t)GetModuleHandleA(NULL), at = (uintptr_t)er->ExceptionAddress;
    char cpu[96], os[64];
    int n;
    cpu_desc(cpu, sizeof cpu);
    os_desc(os, sizeof os);
    n = snprintf(out, cap,
                 "Ashen Depths %s crashed\r\n\r\n"
                 "Exception : 0x%08lX %s\r\n"
                 "Address   : AshenDepthsDesktop.exe+0x%llx\r\n"
                 "CPU       : %s\r\n"
                 "System    : %s\r\n",
                 GAME_VERSION_STR, (unsigned long)er->ExceptionCode, code_name(er->ExceptionCode),
                 (unsigned long long)(at - base), cpu, os);
    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2 && n > 0 && (size_t)n < cap)
        n += snprintf(out + n, cap - (size_t)n, "Access    : %s 0x%llx\r\n",
                      er->ExceptionInformation[0] == 8 ? "execute" : er->ExceptionInformation[0] ? "write" : "read",
                      (unsigned long long)er->ExceptionInformation[1]);
    return n;
}

static void save_report(const char *text, size_t len)
{
    HANDLE f = CreateFileA("crash.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD wrote;
    if (f == INVALID_HANDLE_VALUE)
        return;
    WriteFile(f, text, (DWORD)len, &wrote, NULL);
    CloseHandle(f);
}

static LONG WINAPI on_crash(EXCEPTION_POINTERS *ep)
{
    int n = describe(ep->ExceptionRecord, g_report, sizeof g_report);
    static const char tail[] =
        "\r\nPlease attach this file to an issue at\r\nhttps://github.com/hannnnnnnny/ARPG-CX/issues\r\n";
    if (n > 0 && (size_t)n + sizeof tail < sizeof g_report) {
        memcpy(g_report + n, tail, sizeof tail);
        n += (int)sizeof tail - 1;
    }
    if (n > 0)
        save_report(g_report, (size_t)n);
    MessageBoxW(NULL, L"Ashen Depths crashed. The details are saved in crash.txt next to the game.\n"
                L"Please attach that file to a GitHub issue so it can be fixed.\n\n"
                L"游戏崩溃了，详细信息已保存到游戏"
                L"目录下的 crash.txt，请把它附在 GitHub issue 里。",
                L"Ashen Depths", MB_ICONERROR | MB_OK);
    return EXCEPTION_EXECUTE_HANDLER;
}

void crash_install(void)
{
    SetUnhandledExceptionFilter(on_crash);
}
