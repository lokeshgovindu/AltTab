#pragma once
#include <CommCtrl.h>

// ----------------------------------------------------------------------------
// Global declarations
// ----------------------------------------------------------------------------

#define TIMER_CHECK_ALT_KEYUP       1
#define TIMER_WINDOW_COUNT          2
#define TIMER_CHECK_FOR_UPDATES     3
#define TIMER_CUSTOM_TOOLTIP        4


#ifdef _DEBUG
#define TIMER_WINDOW_COUNT_ELAPSE   5000
#else
#define TIMER_WINDOW_COUNT_ELAPSE   100
#endif // _DEBUG

struct AltTabSettings;
struct AltTabWindowData;
typedef BOOL(WINAPI* IsHungAppWindowFunc)(HWND);

extern HINSTANCE                            g_hInstance;
extern HWND                                 g_hMainWnd;              // AltTab main window handle
extern HWND                                 g_hSetingsWnd;           // AltTab settings window handle
extern HWND                                 g_hAltTabWnd;            // AltTab window handle
extern HWND                                 g_hFGWnd;                // Foreground window handle
extern HWND                                 g_hStaticText;
extern HWND                                 g_hListView;
extern HWND                                 g_hToolTip;
extern HWND                                 g_hCustomTooltip;
extern TOOLINFO                             g_ToolInfo;
extern UINT_PTR                             g_TooltipTimerId;
extern bool                                 g_TooltipVisible;
extern HANDLE                               g_hAltTabThread;
extern DWORD                                g_idThreadAttachTo;
extern AltTabSettings                       g_Settings;
extern IsHungAppWindowFunc                  g_pfnIsHungAppWindow;

extern std::shared_ptr<log4cpp::Category>   g_Logger;

extern std::vector<AltTabWindowData>        g_AltTabWindows;
extern AltTabWindowData                     g_AltBacktickWndInfo; // TODO

extern bool                                 g_IsAltTab;
extern bool                                 g_IsAltBacktick;
extern bool                                 g_IsAltCtrlTab;
extern int                                  g_SelectedIndex;
extern int                                  g_MouseHoverIndex;
extern DWORD                                g_MainThreadID;
extern std::wstring                         g_SearchString;
