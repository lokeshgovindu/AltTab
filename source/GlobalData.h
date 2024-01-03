#pragma once

// ----------------------------------------------------------------------------
// Global declarations
// ----------------------------------------------------------------------------

#define TIMER_CHECK_ALT_KEYUP       1
#define TIMER_WINDOW_COUNT          2

struct AltTabSettings;
struct AltTabWindowData;

extern HINSTANCE                            g_hInstance;
extern HWND                                 g_hMainWnd;              // AltTab main window handle
extern HWND                                 g_hAltTabWnd;            // AltTab window handle

extern AltTabSettings                       g_Settings;

extern std::shared_ptr<log4cpp::Category>   gLogger;

extern std::vector<AltTabWindowData>        g_AltTabWindows;

extern bool                                 g_IsAltTab;
extern bool                                 g_IsAltBacktick;
extern int                                  g_SelectedIndex;
extern DWORD                                g_MainThreadID;

