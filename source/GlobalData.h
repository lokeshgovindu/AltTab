#pragma once

// ----------------------------------------------------------------------------
// Global declarations
// ----------------------------------------------------------------------------

extern HINSTANCE                            g_hInstance;
extern HWND                                 g_hAltTabWnd;           // AltTab window handle
extern HWND                                 g_hWndTrayIcon;         // AltTab tray icon

struct AltTabSettings;
struct AltTabWindowData;

extern AltTabSettings                       g_Settings;

extern std::shared_ptr<log4cpp::Category>   gLogger;

extern std::vector<AltTabWindowData>        g_AltTabWindows;

extern bool                                 g_IsAltTab;
extern bool                                 g_IsAltBacktick;
extern bool                                 g_IsAltBackShown;
