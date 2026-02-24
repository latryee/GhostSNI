#include <windows.h>
#include <shellapi.h>
#include <string.h>
#include "tray.h"
#include "windivert.h"

#define WM_TRAYICON    (WM_USER + 1)
#define ID_TRAY_EXIT    1001
#define ID_TRAY_STATUS  1002

extern volatile int g_running;
extern HANDLE       g_divert_handle;

static HWND            g_tray_hwnd   = NULL;
static NOTIFYICONDATAW g_nid         = {0};
static HANDLE          g_tray_thread = NULL;
static HICON           g_ghost_icon  = NULL;

static HICON create_ghost_icon(void)
{
    int size = 16;
    int pixels = size * size;

    BITMAPINFOHEADER bih = {0};
    bih.biSize        = sizeof(BITMAPINFOHEADER);
    bih.biWidth       = size;
    bih.biHeight      = -size;
    bih.biPlanes      = 1;
    bih.biBitCount    = 32;
    bih.biCompression = BI_RGB;
    bih.biSizeImage   = (DWORD)(pixels * 4);

    UINT32 *bits = NULL;
    HDC hdc = GetDC(NULL);
    HBITMAP hbmColor = CreateDIBSection(hdc, (BITMAPINFO *)&bih,
                                         DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    ReleaseDC(NULL, hdc);
    if (!hbmColor || !bits) return LoadIcon(NULL, IDI_APPLICATION);

    #define T  0x00000000u
    #define W  0xFFFFFFFFu
    #define B  0xFF1A1A2Eu
    #define G  0xFF00D97Eu
    #define D  0xFF00B86Bu

    static const UINT32 icon_data[256] = {
        T,T,T,T,T,G,G,G,G,G,G,T,T,T,T,T,
        T,T,T,G,G,G,G,G,G,G,G,G,G,T,T,T,
        T,T,G,G,G,G,G,G,G,G,G,G,G,G,T,T,
        T,G,G,G,G,G,G,G,G,G,G,G,G,G,G,T,
        T,G,G,G,W,W,G,G,G,W,W,G,G,G,G,T,
        T,G,G,G,B,W,G,G,G,B,W,G,G,G,G,T,
        T,G,G,G,G,G,G,G,G,G,G,G,G,G,G,T,
        T,G,G,G,G,D,G,G,G,D,G,G,G,G,G,T,
        T,G,G,G,G,G,D,D,D,G,G,G,G,G,G,T,
        T,G,G,G,G,G,G,G,G,G,G,G,G,G,G,T,
        T,G,G,G,G,G,G,G,G,G,G,G,G,G,G,T,
        T,G,G,G,G,G,G,G,G,G,G,G,G,G,G,T,
        T,G,G,G,G,G,G,G,G,G,G,G,G,G,G,T,
        T,G,G,T,G,G,G,T,G,G,G,T,G,G,T,T,
        T,G,T,T,T,G,T,T,T,G,T,T,T,G,T,T,
        T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,T,
    };

    #undef T
    #undef W
    #undef B
    #undef G
    #undef D

    memcpy(bits, icon_data, pixels * 4);

    HBITMAP hbmMask = CreateBitmap(size, size, 1, 1, NULL);
    if (!hbmMask) { DeleteObject(hbmColor); return LoadIcon(NULL, IDI_APPLICATION); }

    HDC hdcMask = CreateCompatibleDC(NULL);
    HBITMAP oldBm = SelectObject(hdcMask, hbmMask);
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++)
            SetPixel(hdcMask, x, y, (icon_data[y * size + x] >> 24) ? RGB(0,0,0) : RGB(255,255,255));
    SelectObject(hdcMask, oldBm);
    DeleteDC(hdcMask);

    ICONINFO ii  = {0};
    ii.fIcon     = TRUE;
    ii.hbmMask   = hbmMask;
    ii.hbmColor  = hbmColor;
    HICON icon   = CreateIconIndirect(&ii);

    DeleteObject(hbmColor);
    DeleteObject(hbmMask);

    return icon ? icon : LoadIcon(NULL, IDI_APPLICATION);
}

static LRESULT CALLBACK tray_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_TRAYICON:
        if (lp == WM_RBUTTONUP || lp == WM_LBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            HMENU menu = CreatePopupMenu();

            AppendMenuW(menu, MF_STRING | MF_DISABLED, ID_TRAY_STATUS,
                        L"GhostSNI v0.2.0 (aktif)");
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Durdur ve \x00C7\x0131k");

            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_RIGHTALIGN | TPM_BOTTOMALIGN,
                           pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(menu);
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == ID_TRAY_EXIT)
        {
            g_running = 0;
            if (g_divert_handle != INVALID_HANDLE_VALUE)
                WinDivertShutdown(g_divert_handle, WINDIVERT_SHUTDOWN_BOTH);
            PostQuitMessage(0);
        }
        return 0;

    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static DWORD WINAPI tray_thread_func(LPVOID param)
{
    (void)param;

    WNDCLASSW wc    = {0};
    wc.lpfnWndProc  = tray_wndproc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = L"GhostSNI_TrayClass";
    RegisterClassW(&wc);

    g_tray_hwnd = CreateWindowExW(0, wc.lpszClassName, L"GhostSNI Tray",
                                   0, 0, 0, 0, 0, HWND_MESSAGE, NULL,
                                   wc.hInstance, NULL);
    if (!g_tray_hwnd) return 1;

    g_ghost_icon = create_ghost_icon();

    memset(&g_nid, 0, sizeof(g_nid));
    g_nid.cbSize           = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd             = g_tray_hwnd;
    g_nid.uID              = 1;
    g_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon            = g_ghost_icon;
    wcscpy_s(g_nid.szTip, 128, L"GhostSNI \x2014 DPI Bypass Aktif");

    Shell_NotifyIconW(NIM_ADD, &g_nid);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    Shell_NotifyIconW(NIM_DELETE, &g_nid);
    if (g_ghost_icon) DestroyIcon(g_ghost_icon);
    return 0;
}

int tray_init(void)
{
    g_tray_thread = CreateThread(NULL, 0, tray_thread_func, NULL, 0, NULL);
    return g_tray_thread ? 0 : -1;
}

void tray_destroy(void)
{
    if (g_tray_hwnd)
    {
        PostMessage(g_tray_hwnd, WM_DESTROY, 0, 0);
        g_tray_hwnd = NULL;
    }
    if (g_tray_thread)
    {
        WaitForSingleObject(g_tray_thread, 2000);
        CloseHandle(g_tray_thread);
        g_tray_thread = NULL;
    }
}
