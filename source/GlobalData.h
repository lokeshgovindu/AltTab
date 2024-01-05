#pragma once

// ----------------------------------------------------------------------------
// Global declarations
// ----------------------------------------------------------------------------

#define TIMER_CHECK_ALT_KEYUP       1
#define TIMER_WINDOW_COUNT          2

struct AltTabSettings;
struct AltTabWindowData;
typedef BOOL(WINAPI* IsHungAppWindowFunc)(HWND);

extern HINSTANCE                            g_hInstance;
extern HWND                                 g_hMainWnd;              // AltTab main window handle
extern HWND                                 g_hAltTabWnd;            // AltTab window handle
extern HWND                                 g_hFGWnd;                // Foreground window handle
extern HANDLE                               g_hAltTabThread;
extern DWORD                                g_idThreadAttachTo;
extern AltTabSettings                       g_Settings;
extern IsHungAppWindowFunc                  g_pfnIsHungAppWindow;

extern std::shared_ptr<log4cpp::Category>   gLogger;

extern std::vector<AltTabWindowData>        g_AltTabWindows;

extern bool                                 g_IsAltTab;
extern bool                                 g_IsAltBacktick;
extern int                                  g_SelectedIndex;
extern DWORD                                g_MainThreadID;

