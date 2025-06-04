// AltTab.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "AltTab.h"
#include "Logger.h"
#include "Utils.h"
#include "AltTabWindow.h"
#include <string>
#include <format>
#include "AltTabSettings.h"
#include <shellapi.h>
#include "Resource.h"
#include "version.h"
#include <WinUser.h>
#include <process.h>
#include <CommCtrl.h>
#include "GlobalData.h"
#include <filesystem>
#include "CheckForUpdates.h"
#include <thread>
//#include <sysinfoapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "taskschd.lib")
#pragma comment(lib, "comsuppw.lib")

#pragma comment(                                         \
        linker,                                          \
            "/manifestdependency:\"type='win32' "        \
            "name='Microsoft.Windows.Common-Controls' "  \
            "version='6.0.0.0' "                         \
            "processorArchitecture='*' "                 \
            "publicKeyToken='6595b64144ccf1df' "         \
            "language='*' "                              \
            "\"")

// ----------------------------------------------------------------------------
// Global Variables:
// ----------------------------------------------------------------------------
HINSTANCE       g_hInstance;                                   // Current instance
HHOOK           g_KeyboardHook;                                // Keyboard Hook
HWND            g_hAltTabWnd           = nullptr;              // AltTab window handle
bool            g_hAltTabIsbeingClosed = false;                // Is AltTab window being closed
HWND            g_hFGWnd               = nullptr;              // Foreground window handle
HWND            g_hMainWnd             = nullptr;              // AltTab main window handle
HWND            g_hSetingsWnd          = nullptr;              // AltTab settings window handle
HWND            g_hCustomToolTip       = nullptr;              // Custom tool tip
UINT_PTR        g_TooltipTimerId;
bool            g_TooltipVisible       = false;                // Is tooltip visible or not
TOOLINFO        g_ToolInfo             = {};                   // Custom tool tip
bool            g_IsAltKeyPressed      = false;                // Is Alt key pressed
DWORD           g_LastAltKeyPressTime  = 0;                    // Last Alt key press time
bool            g_IsAltTab             = false;                // Is Alt+Tab pressed
bool            g_IsAltCtrlTab         = false;                // Is Alt+Ctrl+Tab pressed
bool            g_IsAltBacktick        = false;                // Is Alt+Backtick pressed
DWORD           g_MainThreadID         = GetCurrentThreadId(); // Main thread ID
DWORD           g_idThreadAttachTo     = 0;
GeneralSettings g_GeneralSettings; // General settings

IsHungAppWindowFunc g_pfnIsHungAppWindow = nullptr;

UINT const WM_USER_ALTTAB_TRAYICON = WM_APP + 1;

HWND CreateMainWindow(HINSTANCE hInstance);
BOOL AddNotificationIcon(HWND hWndTrayIcon);
void CALLBACK CheckAltKeyIsReleased(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void CALLBACK CheckForUpdatesTimerCB(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
int  GetCurrentYear();

namespace {
    GeneralSettings GetGeneralSettings() {
        GeneralSettings settings;
        settings.IsProcessElevated = IsProcessElevated();
        settings.IsTaskElevated = IsTaskRunWithHighestPrivileges();
        settings.IsRunAtStartup = IsRunAtStartup();
        return settings;
    }

    void RemoveStartupFromRegistry() {
        HKEY hKey;
        LONG result = RegOpenKeyEx(
            HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);

        if (result == ERROR_SUCCESS) {
            result = RegDeleteValue(hKey, AT_PRODUCT_NAMEW);
        }
        RegCloseKey(hKey);
    }
}

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------
int APIENTRY wWinMain(
    _In_        HINSTANCE   hInstance,
    _In_opt_    HINSTANCE   hPrevInstance,
    _In_        LPWSTR      lpCmdLine,
    _In_        int         /*nCmdShow*/)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Set process DPI awareness
    if (!SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE)) {
        // Handle error if setting DPI awareness fails
        // For simplicity, you may just display a message box
        AT_LOG_ERROR("Failed to set DPI awareness!");
    }

    // Make sure only one instance is running
    HANDLE hMutex = CreateMutex(nullptr, TRUE, AT_PRODUCT_NAMEW);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is running, handle it here
        std::wstring info = std::format(L"Another instance of {} is running!", AT_PRODUCT_NAMEW);
        MessageBoxW(nullptr, info.c_str(), AT_PRODUCT_NAMEW, MB_OK | MB_ICONEXCLAMATION);
        if (hMutex != nullptr) {
            CloseHandle(hMutex);
        }
        return 1;
    }

#ifdef _AT_LOGGER
    CreateLogger();
    AT_LOG_INFO("-------------------------------------------------------------------------------");
    AT_LOG_INFO("CreateLogger done.");
#endif // _AT_LOGGER

    // Log application module path
    const std::wstring applicationPath = GetApplicationPath();
    AT_LOG_INFO("Application Info");
    AT_LOG_INFO("  - Path      : %ls", applicationPath.c_str());
    AT_LOG_INFO("  - Version   : %s", AT_FULL_VERSIONA);
    AT_LOG_INFO("  - ProcessID : %d", GetCurrentProcessId());

    InitializeCOM();

    // Load GeneralSettings
    g_GeneralSettings = GetGeneralSettings();
    AT_LOG_INFO(
        "GeneralSettings: IsProcessElevated = %d, IsTaskElevated = %d, IsRunAtStartup = %d",
        g_GeneralSettings.IsProcessElevated,
        g_GeneralSettings.IsTaskElevated,
        g_GeneralSettings.IsRunAtStartup);

    g_hInstance = hInstance; // Store instance handle in our global variable

    CreateCustomToolTip();

    // ----------------------------------------------------------------------------
    // Start writing your code from here...
    // ----------------------------------------------------------------------------
    ShowCustomToolTip(L"Initializing AltTab...", 1000);

    // Load settings from AltTabSettings.ini file
    ATLoadSettings();

    // If we're relaunching for elevation, allow it
    if (g_GeneralSettings.IsProcessElevated && !g_GeneralSettings.IsTaskElevated && wcsstr(GetCommandLineW(), L"--elevated")) {
        AT_LOG_INFO("Relaunching with elevated privileges (--elevated argument).");
        g_GeneralSettings.IsTaskElevated = true;
        if (g_GeneralSettings.IsRunAtStartup) {
            RunAtStartup(true, true);
        }
    }

    // From 2025.1.0.0, we are not going to create a registry key for RunAtStartup.
    // We always create a task in task scheduler to run AltTab at startup. So, we can simply remove the registry key if
    // it exists.
    RemoveStartupFromRegistry();

#if 0
    // Run At Startup
    if (!g_GeneralSettings.IsRunAtStartup) {
        RunAtStartup(true, g_GeneralSettings.IsElevated);
    }
#endif // _DEBUG

    // Register AltTab window class
    if (!RegisterAltTabWindow()) {
        std::wstring info = L"Failed to register AltTab window class.";
        AT_LOG_ERROR("Failed to register AltTab window class.");
        MessageBox(nullptr, info.c_str(), AT_PRODUCT_NAMEW, MB_OK | MB_ICONERROR);
        return 1;
    }

    // System tray
    // Create a hidden window for tray icon handling
    g_hMainWnd = CreateMainWindow(hInstance);

	 HINSTANCE hinstUser32 = LoadLibrary(L"user32.dll");
    if (hinstUser32) {
       g_pfnIsHungAppWindow = (IsHungAppWindowFunc)GetProcAddress(hinstUser32, "IsHungAppWindow");
    }

    // Add the tray icon
    if (g_Settings.SystemTrayIconEnabled && !AddNotificationIcon(g_hMainWnd)) {
        std::wstring info = L"Failed to add AltTab tray icon.";
        AT_LOG_ERROR("Failed to add AltTab tray icon.");
        ShowCustomToolTip(info, 3000);
    }

    g_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, hInstance, NULL);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALTTAB));

    // Check for updates
    if (g_Settings.CheckForUpdatesOpt == L"Startup") {
        std::thread thr(CheckForUpdates, true);
        thr.detach();
    } else if (g_Settings.CheckForUpdatesOpt != L"Never") {
        // Check for every 1 hour
        UINT elapse = 3600000;
        SetTimer(g_hMainWnd, TIMER_CHECK_FOR_UPDATES, elapse, CheckForUpdatesTimerCB);
    }

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnhookWindowsHookEx(g_KeyboardHook);

    return (int) msg.wParam;
}

/**
 * AltTab system tray icon procedure
 * 
 * \param hWnd      hWnd
 * \param message   message
 * \param wParam    wParam
 * \param lParam    lParam
 * 
 * \return 
 */
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND: {
        int const wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case ID_TRAYCONTEXTMENU_ABOUTALTTAB:
            AT_LOG_INFO("ID_TRAYCONTEXTMENU_ABOUTALTTAB");
            DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, ATAboutDlgProc);
            break;

        case ID_TRAYCONTEXTMENU_README:
            AT_LOG_INFO("ID_TRAYCONTEXTMENU_README");
            break;

        case ID_TRAYCONTEXTMENU_HELP:
            AT_LOG_INFO("ID_TRAYCONTEXTMENU_HELP");
            break;

        case ID_TRAYCONTEXTMENU_RELEASENOTES:
            AT_LOG_INFO("ID_TRAYCONTEXTMENU_RELEASENOTES");
            break;

        case ID_TRAYCONTEXTMENU_SETTINGS:
            AT_LOG_INFO("ID_TRAYCONTEXTMENU_SETTINGS");
            DialogBoxW(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), g_hMainWnd, ATSettingsDlgProc);
            break;

        case ID_TRAYCONTEXTMENU_DISABLEALTTAB:
            AT_LOG_INFO("ID_TRAYCONTEXTMENU_DISABLEALTTAB");
            break;

        case ID_TRAYCONTEXTMENU_CHECKFORUPDATES:
            AT_LOG_INFO("ID_TRAYCONTEXTMENU_CHECKFORUPDATES");
            break;

        case ID_TRAYCONTEXTMENU_RUNATSTARTUP:
            AT_LOG_INFO("ID_TRAYCONTEXTMENU_RUNATSTARTUP");
            break;

        case ID_TRAYCONTEXTMENU_EXIT:
            AT_LOG_INFO("ID_TRAYCONTEXTMENU_EXIT");
            PostQuitMessage(0);
            break;
        }

    } break;

    case WM_USER_ALTTAB_TRAYICON:
        switch (LOWORD(lParam)) {
            case WM_RBUTTONUP: {
                POINT pt;
                GetCursorPos(&pt);
                ShowTrayContextMenu(hWnd, pt);
            }
            break;

            case WM_LBUTTONDBLCLK: {
                // Set g_IsAltCtrlTab to true to add the windows to AltTab window, otherwise no windows will be added.
                g_IsAltCtrlTab = true;
                ShowAltTabWindow(g_hAltTabWnd, 0);
            }
            break;
        }
        break;
    
    case WM_DESTROY:
        AT_LOG_INFO("WM_DESTROY");
        KillTimer(hWnd, TIMER_CHECK_FOR_UPDATES);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Function to create a hidden window for tray icon handling
HWND CreateMainWindow(HINSTANCE hInstance) {
    HWND hwndFrgnd = GetForegroundWindow();
    DWORD idThreadAttachTo = hwndFrgnd ? GetWindowThreadProcessId(hwndFrgnd, NULL) : 0;
    if (idThreadAttachTo) {
        AttachThreadInput(GetCurrentThreadId(), idThreadAttachTo, TRUE);
    }
    WNDCLASS wc      = { 0 };
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"__AltTab_MainWndCls__";
    wc.hIcon         = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ALTTAB));
    RegisterClass(&wc);

    return CreateWindowW(
        wc.lpszClassName,       // Class Name
        L"AltTabMainWindow",    // Window Name
        0,                      // Style
        0,                      // X
        0,                      // Y
        1,                      // Width
        1,                      // Height
        nullptr,                // Parent
        nullptr,                // Menu
        hInstance,              // Instance
        nullptr                 // Extra
    );
}

BOOL AddNotificationIcon(HWND hWndTrayIcon) {
    // Set up the NOTIFYICONDATA structure
    NOTIFYICONDATA nid   = { 0 };
    nid.cbSize           = sizeof(NOTIFYICONDATA);
    nid.hWnd             = hWndTrayIcon;
    nid.uID              = 1;
    nid.uFlags           = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_USER_ALTTAB_TRAYICON;
    nid.hIcon            = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ALTTAB));
    wcscpy_s(nid.szTip, AT_PRODUCT_NAMEW);

    return Shell_NotifyIcon(NIM_ADD, &nid);
}

// ----------------------------------------------------------------------------
// Message handler for about box.
// ----------------------------------------------------------------------------
INT_PTR CALLBACK ATAboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: {
        HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ALTTAB));
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

        // Center the dialog on the screen
        int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        RECT dlgRect;
        GetWindowRect(hDlg, &dlgRect);

        int dlgWidth  = dlgRect.right - dlgRect.left;
        int dlgHeight = dlgRect.bottom - dlgRect.top;

        int posX = (screenWidth  - dlgWidth ) / 2;
        int posY = (screenHeight - dlgHeight) / 2;

        SetWindowPos(hDlg, HWND_TOP, posX, posY, 0, 0, SWP_NOSIZE);
        // Set the dialog as an app window, otherwise not displayed in task bar
        SetWindowLong(hDlg, GWL_EXSTYLE, GetWindowLong(hDlg, GWL_EXSTYLE) | WS_EX_APPWINDOW);

        std::wstring productInfo = std::format(L"<a href=\"{}\">{}</a> v{}", AT_PRODUCT_PAGE, AT_PRODUCT_NAMEW, AT_VERSION_TEXTW);
        std::wstring copyright   = std::format(L"Copyright © {} <a href=\"{}\">{}</a>", AT_PRODUCT_YEARW, AT_PRODUCT_PAGE, AT_AUTHOR_NAME);

        SetDlgItemTextW(hDlg, IDC_SYSLINK_ABOUT_PRODUCT_NAME, productInfo.c_str());
        SetDlgItemTextW(hDlg, IDC_SYSLINK_ABOUT_COPYRIGHT   , copyright.c_str());
    }
    return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;

    case WM_NOTIFY:
        if (wParam == IDC_SYSLINK_ABOUT_PRODUCT_NAME) {
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->code == NM_CLICK) {
                ShellExecute(nullptr, L"open", AT_PRODUCT_PAGE, nullptr, nullptr, SW_SHOWNORMAL);
            }
        } else if (wParam == IDC_SYSLINK_ABOUT_COPYRIGHT) {
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->code == NM_CLICK) {
                ShellExecute(nullptr, L"open", AT_AUTHOR_PAGE, nullptr, nullptr, SW_SHOWNORMAL);
            }
        }
        break;

    case WM_DESTROY:
        DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_SMALL, 0));
        DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, ICON_BIG, 0));
        break;
    }
    return (INT_PTR)FALSE;
}

// ----------------------------------------------------------------------------
// Activate window of the given window handle
// ----------------------------------------------------------------------------
void ActivateWindow(HWND hTargetWnd) {
    AT_LOG_TRACE;

	 HWND hForegroundWnd = GetForegroundWindow();
    if (hTargetWnd == hForegroundWnd) {
        return;
    }

    // Bring the window to the foreground
    // Determines whether the specified window is minimized (iconic).
    if (IsIconic(hTargetWnd)) {
        //ShowWindow(hWnd, SW_RESTORE);
        PostMessage(hTargetWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    } else {
        BOOL result = SetForegroundWindow(hTargetWnd);
        if (!result && hForegroundWnd != hTargetWnd) {
            // Failed to bring an elevated window to the top from a non-elevated process.
            AT_LOG_ERROR("SetForegroundWindow(hWnd) failed!");

            ShowWindow(hTargetWnd, SW_SHOW);
            result = BringWindowToTop(hTargetWnd);
            HWND hFGWnd = GetForegroundWindow();
            if (!result && hFGWnd != hTargetWnd) {
                AT_LOG_ERROR("BringWindowToTop(hWnd) failed!");
            } else {
                SetActiveWindow(hTargetWnd);
                AT_LOG_INFO("BringWindowToTop(hWnd) succeeded!");
                //return;
            }

            // It seems it is always better to use AttachThreadInput than 
            // SetForegroundWindow even the BringWindowToTop succeeded. So not
            // going to comment the below piece of code.
            DWORD idForeground = GetWindowThreadProcessId(hForegroundWnd, nullptr);
            DWORD idTarget     = GetWindowThreadProcessId(hTargetWnd    , nullptr);

            if (hFGWnd && !IsHungAppWindowEx(hFGWnd))
                AttachThreadInput(idForeground, idTarget, TRUE);
            
            if (!SetForegroundWindow(hTargetWnd)) {
                INPUT inp[4];
                ZeroMemory(&inp, sizeof(inp));
                inp[0].type       = inp[1].type       = inp[2].type   = inp[3].type   = INPUT_KEYBOARD;
                inp[0].ki.wVk     = inp[1].ki.wVk     = inp[2].ki.wVk = inp[3].ki.wVk = VK_MENU;
                inp[0].ki.dwFlags = inp[2].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
                inp[1].ki.dwFlags = inp[3].ki.dwFlags = KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP;
                SendInput(4, inp, sizeof(INPUT));

                SetForegroundWindow(hTargetWnd);
            }

            if (hFGWnd && !IsHungAppWindowEx(hFGWnd))
                AttachThreadInput(idForeground, idTarget, FALSE);
        }
    }
    SetActiveWindow(hTargetWnd);
}

/**
 * \brief Destroy AltTab Window and do necessary cleanup here
 * 
 * \param activate   Input parameter to activate the selected window or not
 */
void DestoryAltTabWindow(bool activate) {
    if (g_hAltTabWnd == nullptr || g_hAltTabIsbeingClosed) {
        return;
    }

    AT_LOG_TRACE;

    // Set flag to true to avoid re-entry from WM_ACTIVATEAPP
    g_hAltTabIsbeingClosed = true;

    // Hide custom tooltip
    HideCustomToolTip();

    // Kill timer
    KillTimer(g_hMainWnd, TIMER_CHECK_ALT_KEYUP);

    if (g_idThreadAttachTo) {
        AttachThreadInput(GetCurrentThreadId(), g_idThreadAttachTo, FALSE);
        g_idThreadAttachTo = 0;
    }

    if (activate) {
        int selectedInd = ATWListViewGetSelectedItem();
        HWND hWnd = nullptr;
        if (selectedInd != -1) {
            hWnd = g_AltTabWindows[selectedInd].hWnd;
            AT_LOG_INFO("hWnd = [%#x], title = [%s]", hWnd, GetWindowTitleExA(hWnd).c_str());
        }
        DestroyWindow(g_hAltTabWnd);
        PostMessage(g_hAltTabWnd, WM_CLOSE, 0, 0);
        if (hWnd && !IsHungAppWindowEx(hWnd)) {
            ActivateWindow(hWnd);
        }
    } else {
        DestroyWindow(g_hAltTabWnd);
    }
    
    // CleanUp
    g_hAltTabWnd           = nullptr;
    g_IsAltTab             = false;
    g_IsAltCtrlTab         = false;
    g_IsAltBacktick        = false;
    g_SelectedIndex        = -1;
    g_AltTabWindows.clear();
    g_SearchString .clear();
    g_AltBacktickWndInfo   = {};
    g_hAltTabIsbeingClosed = false;
}

// ----------------------------------------------------------------------------
// Low level keyboard procedure
// 
// If nCode is less than zero:
//   the hook procedure must return the value returned by CallNextHookEx.
// 
// If nCode is greater than or equal to zero:
//   and the hook procedure did not process the message, it is highly
//   recommended that you call CallNextHookEx and return the value it returns;
//   otherwise, other applications that have installed WH_KEYBOARD_LL hooks 
//   will not receive hook notifications and may behave incorrectly as a result.
// 
// If the hook procedure processed the message:
//   it may return a nonzero value to prevent the system from passing the
//   message to the rest of the hook chain or the target window procedure.
// ----------------------------------------------------------------------------
LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    //AT_LOG_TRACE;

    // Call the next hook in the chain
    if (nCode != HC_ACTION) {
        return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
    }
    
    //AT_LOG_INFO(std::format("hAltTabWnd is nullptr: {}", hAltTabWnd == nullptr).c_str());
    auto* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    DWORD vkCode    = pKeyboard->vkCode;
        
    // Check if Alt key is pressed
    bool isAltPressed  = GetAsyncKeyState(VK_MENU   ) & 0x8000;
    bool isCtrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;

    // When Alt is pressed down WM_SYSKEYDOWN is sent and when Alt is released WM_KEYUP is sent.
    // But, GetAsyncKeyState return 0 when Alt is pressed down first time and returns 1 while holding down.
    // So, we need to check vkCode also to make it work for the first time also.
    isAltPressed = isAltPressed || vkCode == VK_LMENU || vkCode == VK_RMENU || vkCode == VK_MENU;

    //AT_LOG_DEBUG("wParam: %#x, vkCode: %0#4x, isAltPressed: %d", wParam, vkCode, isAltPressed);

    // ----------------------------------------------------------------------------
    // Alt key is pressed
    // 20240316: Now we are handling Alt+Ctrl+Tab also. Alt key will be released
    // when user wants Alt+Ctrl+Tab window. So, isAltPressed is not always true.
    // Hence, check for g_hAltTabWnd also whether the AltTab window is displayed.
    // ----------------------------------------------------------------------------
    if (isAltPressed && (!isCtrlPressed || g_Settings.HKAltCtrlTabEnabled) || g_hAltTabWnd != nullptr) {
        //AT_LOG_INFO("Alt key pressed!, wParam: %#x", wParam);

        // Check if Shift key is pressed
        bool isShiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
        int  direction      = isShiftPressed ? -1 : 1;

        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (g_hAltTabWnd == nullptr) {
                // Check if windows native Alt+Tab window is displayed. If so, do not process the message.
                // Otherwise, both native Alt+Tab window and AltTab window will be displayed.
                bool isNativeATWDisplayed = IsNativeATWDisplayed();
                //AT_LOG_INFO("isNativeATWDisplayed: %d", isNativeATWDisplayed);

                // ----------------------------------------------------------------------------
                // Alt + Tab / Alt + Ctrl + Tab
                // ----------------------------------------------------------------------------
                bool showAltTabWindow =
                   (g_Settings.HKAltTabEnabled     && !isCtrlPressed) ||
                   (g_Settings.HKAltCtrlTabEnabled &&  isCtrlPressed);
                if (showAltTabWindow && vkCode == VK_TAB) {
                    if (isNativeATWDisplayed) {
                        //AT_LOG_INFO("isNativeATWDisplayed: %d", isNativeATWDisplayed);
                        return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
                    }

                    g_IsAltTab      = true;
                    g_IsAltBacktick = false;

                    if (!isShiftPressed) {
                        // Alt+Tab is pressed
                        AT_LOG_INFO("--------- Alt+Tab Pressed! ---------");
                        ShowAltTabWindow(g_hAltTabWnd, 1);
                    } else {
                        // Alt+Shift+Tab is pressed
                        AT_LOG_INFO("--------- Alt+Shift+Tab Pressed! ---------");
                        ShowAltTabWindow(g_hAltTabWnd, -1);
                    }
                }
                // ----------------------------------------------------------------------------
                // Alt + Backtick
                // ----------------------------------------------------------------------------
                else if (g_Settings.HKAltBacktickEnabled && vkCode == VK_OEM_3) { // 0xC0
                    g_IsAltTab      = false;
                    g_IsAltBacktick = true;

                    if (!isShiftPressed) {
                        // Alt+Backtick is pressed
                        AT_LOG_INFO("--------- Alt+Backtick Pressed! ---------");
                        ShowAltTabWindow(g_hAltTabWnd, 1);
                    } else {
                        // Alt+Shift+Backtick is pressed
                        AT_LOG_INFO("--------- Alt+Shift+Backtick Pressed! ---------");
                        ShowAltTabWindow(g_hAltTabWnd, -1);
                    }
                }
                // ----------------------------------------------------------------------------
                // Check if Alt key is pressed twice within 500ms
                // ----------------------------------------------------------------------------
                //else if (vkCode == VK_LMENU || vkCode == VK_RMENU) {
                //    DWORD currentTime = GetTickCount();
                //    AT_LOG_INFO("currentTime: %u, g_LastAltKeyPressTime: %u", currentTime, g_LastAltKeyPressTime);

                //    if (!g_IsAltKeyPressed || (currentTime - g_LastAltKeyPressTime) > 500) {
                //        g_IsAltKeyPressed = true;
                //        g_LastAltKeyPressTime = currentTime;
                //    } else {
                //        // Alt key pressed twice quickly!
                //        AT_LOG_INFO("Alt key pressed twice within 500ms!");
                //        g_IsAltKeyPressed = false;
                //        g_LastAltKeyPressTime = 0;
                //    }
                //}

                // ----------------------------------------------------------------------------
                // Create timer here to check the Alt key is released.
                // ----------------------------------------------------------------------------
                if (g_IsAltTab || g_IsAltBacktick) {
                    // Do NOT start the timer if Ctrl key is pressed.
                    // Here, when user presses Alt+Ctrl+Tab, AltTab window remains open.
                    if (!g_Settings.HKAltCtrlTabEnabled || !isCtrlPressed) {
                        SetTimer(g_hMainWnd, TIMER_CHECK_ALT_KEYUP, 50, CheckAltKeyIsReleased);
                    } else {
                        g_IsAltCtrlTab = true;
                    }
                    return TRUE;
                }
            } else {
                // ----------------------------------------------------------------------------
                // AltTab window is displayed.
                // ----------------------------------------------------------------------------
                if (vkCode == VK_TAB) {
                    //AT_LOG_INFO("Tab Pressed!");
                    ShowAltTabWindow(g_hAltTabWnd, direction);
                    return TRUE;
                } 
                
                if (vkCode == VK_ESCAPE) {
                    AT_LOG_INFO("Escape Pressed!");
                    DestoryAltTabWindow();
                    return TRUE;
                }

                if (vkCode == VK_APPS && g_SelectedIndex != -1) {
                    AT_LOG_INFO("Apps Pressed!");
                    ShowContextMenuAtItemCenter();
                    return TRUE;
                }

                // If any other application define the HotKey Alt + ~, we need
                // to stop sending WM_KEYDOWN on ~ to that application.
                // For example. If AltTabAlternative is running, ATA's window
                // gets opened.
                //
                // Now, send WM_KEYDOWN to g_hAltTabWnd, there handle.
                if (vkCode == VK_OEM_3) { // 0xC0
                    //AT_LOG_INFO("Backtick Pressed!");
                    PostMessage(g_hListView, WM_KEYDOWN, vkCode, 0);
                    return TRUE;
                }
                
                if (vkCode == VK_F4) {
                    // DO NOT DESTROY ALT TAB
                    return TRUE;
                } 

                //AT_LOG_WARN("Not Handled: wParam: %u, vkCode: %0#4x, isprint: %d", wParam, vkCode, iswprint(vkCode));
            }
        //} else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
        //   if (vkCode == VK_LMENU || vkCode == VK_RMENU) {
        //        g_IsAltKeyPressed = false;
        //        g_LastAltKeyPressTime = 0;
        //    }
        }
    } // if (isAltPressed || g_hAltTabWnd != nullptr)

    //AT_LOG_DEBUG("CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);");
    return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
}

// Timer callback function
void CALLBACK CheckForUpdatesTimerCB(HWND /*hWnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/) {
    AT_LOG_TRACE;
    std::wstring frequency  = g_Settings.CheckForUpdatesOpt;
    auto lastCheckTimestamp = ReadLastCheckForUpdatesTS();
    auto currentTimeStamp   = std::chrono::system_clock::now();
    auto timeDiff           = currentTimeStamp - lastCheckTimestamp;

    if ((frequency == L"Daily"  && timeDiff >= std::chrono::hours(24    )) ||
        (frequency == L"Weekly" && timeDiff >= std::chrono::hours(24 * 7)))
    {
        // Perform the update check logic here
        CheckForUpdates(true);

        // Update the last check timestamp
        WriteCheckForUpdatesTS(currentTimeStamp);
    } else {
        AT_LOG_INFO("Update check not required at this time.");
    }
}

void CALLBACK CheckAltKeyIsReleased(HWND /*hWnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/) {
    //AT_LOG_TRACE;
    bool isAltPressed = GetAsyncKeyState(VK_MENU) & 0x8000;
    if (g_hAltTabWnd && !isAltPressed) {
        // Alt key released, destroy your window
        AT_LOG_INFO("--------- Alt key released! ---------");
        DestoryAltTabWindow(true);
    }
}

void TrayContextMenuItemHandler(HWND hWnd, HMENU hSubMenu, UINT menuItemId) {
    switch (menuItemId) {
    case ID_TRAYCONTEXTMENU_ABOUTALTTAB:
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_ABOUTALTTAB");
        DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, ATAboutDlgProc);
        break;

    case ID_TRAYCONTEXTMENU_README:
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_README");
        ShowReadMeWindow();
        break;

    case ID_TRAYCONTEXTMENU_HELP:
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_HELP");
        ShowHelpWindow();
        break;

    case ID_TRAYCONTEXTMENU_RELEASENOTES:
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_RELEASENOTES");
        ShowReleaseNotesWindow();
        break;

    case ID_TRAYCONTEXTMENU_SETTINGS:
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_SETTINGS");
        DialogBoxW(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), g_hMainWnd, ATSettingsDlgProc);
        break;

    case ID_TRAYCONTEXTMENU_DISABLEALTTAB: {
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_DISABLEALTTAB");
        UINT checkState          = GetCheckState(hSubMenu, menuItemId);
        bool disableAltTab       = !(checkState == MF_CHECKED);
        g_Settings.DisableAltTab = disableAltTab;

        if (disableAltTab) {
            UnhookWindowsHookEx(g_KeyboardHook);
        } else {
            g_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, g_hInstance, NULL);
        }
    }
    break;

    case ID_TRAYCONTEXTMENU_CHECKFORUPDATES: {
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_CHECKFORUPDATES");
        ShowCustomToolTip(L"Checking for updates..., please wait.");

        // Had to run CheckForUpdates in a thread to display the tooltip... :-(
        std::thread thr(CheckForUpdates, false); thr.detach();
    }
    //DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CHECK_FOR_UPDATES), nullptr, ATCheckForUpdatesDlgProc);
    break;

    case ID_TRAYCONTEXTMENU_RUNATSTARTUP: {
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_RUNATSTARTUP");
        UINT checkState = GetCheckState(hSubMenu, menuItemId);
        const bool checked    = (checkState == MF_CHECKED);
        RunAtStartup(!checked, g_GeneralSettings.IsProcessElevated);
        ToggleCheckState(hSubMenu, menuItemId);
    }
    break;

    case ID_TRAYCONTEXTMENU_RUNASADMIN: {
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_RUNASADMIN");
        const UINT checkState = GetCheckState(hSubMenu, menuItemId);
        const bool checked = (checkState == MF_CHECKED);
        AT_LOG_INFO("Run as admin: %d", checked);

        // Check if the current process is elevated
        // If the current process is elevated, then we don't need to relaunch AltTab with administrator privileges.
        //if (checked && g_GeneralSettings.IsProcessElevated) {
        //    AT_LOG_INFO("AltTab is already running with administrator privileges.");
        //    ShowCustomToolTip(L"AltTab is already running with administrator privileges.", 3000);
        //    return;
        //}

        const bool isRunAtStartup = IsRunAtStartup();
        AT_LOG_INFO("IsChecked: %d, Run at startup: %d", checked, isRunAtStartup);
        if (!checked ) {
            // Also check if `Run at Startup` is enabled. Then, we need to create a task in `Task Scheduler` to run
            // AltTab at windows log on to run `AltTab` with highest privileges.
            if (isRunAtStartup) {
                // If the process is already elevated, no need to relaunch as administrator privileges.
                if (g_GeneralSettings.IsProcessElevated) {
                    AT_LOG_INFO("AltTab is already running with administrator privileges.");
                    DeleteAutoStartTask();
                    CreateAutoStartTask(true);
                    return;
                }
                
                // Ask user for confirmation to always run as administrator since `Run at Startup` is enabled.
                const int result = MessageBoxW(
                    nullptr,
                    L"Do you want to always run AltTab as Administrator since 'Run at Startup' is enabled?\n\n"
                    L"This will change the AltTab startup task to 'Run with highest privileges' option in Task Scheduler.\n"
                    L"Going to relaunch AltTab with admin privileges...",
                    AT_PRODUCT_NAMEW,
                    MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);
                  AT_LOG_INFO("Relaunching AltTab as admin, elevated: %d", result);
                if (result == IDYES) {
                    RelaunchAsAdminAndExit(true, true);
                } else {
                    //RelaunchAsAdminAndExit(false, false);
                    // Do NOTHING
                }
            } else {
                RelaunchAsAdminAndExit(true, false);
            }
        } else {
            if (isRunAtStartup) {
                AT_LOG_INFO("Delete task for Run as admin...");
                DeleteAutoStartTask();
                AT_LOG_INFO("Creating task for without admin...");
                CreateAutoStartTask(false);
            } else {
                RelaunchAsAdminAndExit(false, false);
            }
        }
        ToggleCheckState(hSubMenu, menuItemId);
    } break;

    case ID_TRAYCONTEXTMENU_RELOADALTTABSETTINGS: {
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_RELOADALTTABSETTINGS");
        ATLoadSettings();
        ShowCustomToolTip(L"Settings reloaded successfully.", 3000);
    } break;

    case ID_TRAYCONTEXTMENU_RESTART: {
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_RESTART");

        // FIXME: Still this is not working :-(
#ifdef _DEBUG
        // Close/detach the console window of the current process
        // This is not needed in release build, as we are not using console window.
        // If you want to keep the console window in debug mode, comment the below lines.
        // If you want to keep the console window in debug mode, comment the below lines.
        FreeConsole(); // Detach the console window from the current process

        //bool result = FreeConsole();
        //if (!result) {
        //    AT_LOG_ERROR("Failed to free console!");
        //}
        // Attach to an existing console (if any)
        //if (AttachConsole(ATTACH_PARENT_PROCESS) || AttachConsole(GetCurrentProcessId())) {
        //    // Close the console window
        //    FreeConsole();
        //}
#endif // _DEBUG

        if (g_GeneralSettings.IsProcessElevated) {
            // Relaunch AltTab with administrator privileges
            RelaunchAsAdminAndExit(true, false);
        } else {
            RestartApplication();
        }
    }
    break;

    case ID_TRAYCONTEXTMENU_EXIT:
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_EXIT");
        PostQuitMessage(0);
        //int result = MessageBoxW(
        //    hWnd,
        //    L"Are you sure you want to exit?",
        //    AT_PRODUCT_NAMEW,
        //    MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2);
        //if (result == IDOK) {
        //    PostQuitMessage(0);
        //}
        break;
    }
}

// ----------------------------------------------------------------------------
// Show AltTab system tray context menu
// ----------------------------------------------------------------------------
void ShowTrayContextMenu(HWND hWnd, POINT pt) {
    // Update general settings
    // Note: The current process is elevated but `RunAtStartup` is not enabled then IsRunElevated will be false.
    g_GeneralSettings = GetGeneralSettings();
    AT_LOG_INFO(
        "GeneralSettings: IsProcessElevated = %d, IsTaskElevated = %d, IsRunAtStartup = %d",
        g_GeneralSettings.IsProcessElevated,
        g_GeneralSettings.IsTaskElevated,
        g_GeneralSettings.IsRunAtStartup);

    HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDC_TRAY_CONTEXTMENU));
    if (hMenu) {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu) {
            // Our window must be foreground before calling TrackPopupMenu or
            // the menu will not disappear when the user clicks away
            SetForegroundWindow(hWnd);
            const bool isRunAtStartup = IsRunAtStartup();
            if (isRunAtStartup) {
                AT_LOG_INFO("Run at startup is enabled.");
                SetCheckState(hSubMenu, ID_TRAYCONTEXTMENU_RUNATSTARTUP, MF_CHECKED);
            }

            // Change the `Run as Administrator` menu item text to `Always run as Administrator` if AltTab is running at
            // startup.
            if (isRunAtStartup) {
                MENUITEMINFOW mii = { sizeof(mii) };
                mii.fMask = MIIM_STRING;
                mii.dwTypeData = (LPWSTR)L"Always run as Administrator";
                SetMenuItemInfoW(hMenu, ID_TRAYCONTEXTMENU_RUNASADMIN, FALSE, &mii);
            } else {
                MENUITEMINFOW mii = { sizeof(mii) };
                mii.fMask = MIIM_STRING;
                mii.dwTypeData = (LPWSTR)L"Run as Administrator";
                SetMenuItemInfoW(hMenu, ID_TRAYCONTEXTMENU_RUNASADMIN, FALSE, &mii);
            }

            if (g_GeneralSettings.IsProcessElevated) {
                if (isRunAtStartup) {
                    if (IsTaskRunWithHighestPrivileges()) {
                        SetCheckState(hSubMenu, ID_TRAYCONTEXTMENU_RUNASADMIN, MF_CHECKED);
                    }
                    else {
                        SetCheckState(hSubMenu, ID_TRAYCONTEXTMENU_RUNASADMIN, MF_UNCHECKED);
                    }
                } else {
                    SetCheckState(hSubMenu, ID_TRAYCONTEXTMENU_RUNASADMIN, MF_CHECKED);
                }

                // If the current process is elevated and RunAtStartup is NOT checked then disable
                // `Run as Administrator`, because we can't run a non-elevated process from the elevated process.
                if (!isRunAtStartup) {
                    EnableMenuItem(hSubMenu, ID_TRAYCONTEXTMENU_RUNASADMIN, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
                } else {
                    EnableMenuItem(hSubMenu, ID_TRAYCONTEXTMENU_RUNASADMIN, MF_BYCOMMAND | MF_ENABLED);
                }
            }

            if (g_Settings.DisableAltTab) {
                SetCheckState(hSubMenu, ID_TRAYCONTEXTMENU_DISABLEALTTAB, MF_CHECKED);
            }

            // Respect menu drop alignment
            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0) {
                uFlags |= TPM_RIGHTALIGN;
            } else {
                uFlags |= TPM_LEFTALIGN;
            }

            // Use TPM_RETURNCMD flag let TrackPopupMenuEx function return the 
            // menu item identifier of the user's selection in the return value.
            uFlags |= TPM_RETURNCMD;
            UINT menuItemId = TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hWnd, nullptr);

            TrayContextMenuItemHandler(hWnd, hSubMenu, menuItemId);
        }
        DestroyMenu(hMenu);
    }
}

void ToggleCheckState(HMENU hMenu, UINT menuItemID) {
    MENUITEMINFO menuItemInfo = { sizeof(MENUITEMINFO) };
    menuItemInfo.cbSize       = sizeof(MENUITEMINFO);
    menuItemInfo.fMask        = MIIM_STATE;

    // Get the current state of the menu item
    GetMenuItemInfo(hMenu, menuItemID, FALSE, &menuItemInfo);

    // Toggle the check mark state
    menuItemInfo.fState ^= MF_CHECKED;

    // Set the updated state
    SetMenuItemInfo(hMenu, menuItemID, FALSE, &menuItemInfo);
}

UINT GetCheckState(HMENU hMenu, UINT menuItemID) {
    MENUITEMINFO menuItemInfo = { sizeof(MENUITEMINFO) };
    menuItemInfo.cbSize       = sizeof(MENUITEMINFO);
    menuItemInfo.fMask        = MIIM_STATE;

    // Get the current state of the menu item
    GetMenuItemInfo(hMenu, menuItemID, FALSE, &menuItemInfo);

    // Toggle the check mark state
    return menuItemInfo.fState & MF_CHECKED;
}

void SetCheckState(HMENU hMenu, UINT menuItemID, UINT fState) {
    MENUITEMINFO menuItemInfo = { sizeof(MENUITEMINFO) };
    menuItemInfo.cbSize       = sizeof(MENUITEMINFO);
    menuItemInfo.fMask        = MIIM_STATE;
    menuItemInfo.fState       = fState;

    // Set the updated state
    SetMenuItemInfo(hMenu, menuItemID, FALSE, &menuItemInfo);
}

bool RunAtStartup(const bool runAtStartup, const bool withHighestPrivileges) {
    const bool isElevated = g_GeneralSettings.IsProcessElevated;
    AT_LOG_INFO("runAtStartup: %d, IsProcessElevated: %d", runAtStartup, isElevated);

    HKEY hKey;
    LONG result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_SET_VALUE,
        &hKey);

    // First, always delete the registry entry if it exists. And, we are going to use Task Scheduler to run AltTab at
    // startup.
    if (result == ERROR_SUCCESS) {
         result = RegDeleteValue(hKey, AT_PRODUCT_NAMEW);
         RegCloseKey(hKey);
    }

    bool succeeded = false;
    if (runAtStartup) {
        DeleteAutoStartTask();
        if (isElevated && withHighestPrivileges) {
            succeeded = CreateAutoStartTask(true);
            if (succeeded) {
                AT_LOG_INFO("Run at startup enabled with highest privileges.");
            } else {
                AT_LOG_INFO("Failed to create task for Run at startup with highest privileges.");
            }
        } else {
            succeeded = CreateAutoStartTask(false);
            if (succeeded) {
                AT_LOG_INFO("Run at startup enabled without highest privileges.");
            } else {
                AT_LOG_INFO("Failed to create task for Run at startup without highest privileges.");
            }
        }
    } else {
        // If runAtStartup is false, delete the task in Task Scheduler if it exists.
        if (IsAutoStartTaskActive()) {
            succeeded = DeleteAutoStartTask();
        } else {
            AT_LOG_INFO("No task found in Task Scheduler for Run at startup.");
            succeeded = true; // No task to delete, so return true.
        }
    }

    return succeeded;
}

bool IsRunAtStartup() {
    // First check if there is an task in Task Scheduler
    if (IsAutoStartTaskActive()) {
        AT_LOG_INFO("Auto start task exists for this user.");
        return true;
    }

    HKEY hKey;
    LONG result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_READ,
        &hKey);

    if (result == ERROR_SUCCESS) {
       // Get AltTab value from the registry
        AT_LOG_INFO("Checking registry for Run at startup...");

        // Check if the registry entry exists

        DWORD dataType = 0;
        DWORD dataSize = 0;

        // First call to get the required buffer size
        result = RegQueryValueExW(hKey, AT_PRODUCT_NAMEW, nullptr, &dataType, nullptr, &dataSize);
        if (result != ERROR_SUCCESS || dataType != REG_SZ) {
            RegCloseKey(hKey);
            return false;
        }

        std::wstring value(dataSize / sizeof(wchar_t), L'\0');
        result =
            RegQueryValueExW(hKey, AT_PRODUCT_NAMEW, nullptr, nullptr, reinterpret_cast<LPBYTE>(&value[0]), &dataSize);
        RegCloseKey(hKey);

        if (result == ERROR_SUCCESS && dataSize > 0) {
            // The registry entry exists, so AltTab is set to run at startup
            AT_LOG_INFO("Run at startup is enabled in registry.");

            // This is old mechanism, so going to delete the registry entry and create a task in Task Scheduler
            if (g_GeneralSettings.IsProcessElevated) {
                // Create a task in Task Scheduler to run AltTab with highest privileges
                if (CreateAutoStartTask(true)) {
                    AT_LOG_INFO("Run at startup enabled with highest privileges.");
                } else {
                    AT_LOG_ERROR("Failed to create task for Run at startup with highest privileges.");
                }
            } else {
                if (CreateAutoStartTask(false)) {
                    AT_LOG_INFO("Run at startup enabled without highest privileges.");
                } else {
                    AT_LOG_ERROR("Failed to create task for Run at startup without highest privileges.");
                }
            }

            return true;
        }

        if (result == ERROR_FILE_NOT_FOUND) {
            // The registry entry does not exist, so AltTab is not set to run at startup
            AT_LOG_INFO("Run at startup is NOT enabled.");
        } else {
            AT_LOG_ERROR("Failed to query the registry value for Run at startup.");
        }
    }
    return false;
}

// ----------------------------------------------------------------------------
// This function is used to check if the Alt+Tab window is displayed
// On Windows 10 Alt+Tab window
//   className = "MultitaskingViewFrame" and Title = "Task Switching"
// I didn't check on other OS
// ----------------------------------------------------------------------------
BOOL CALLBACK EnumWindowsProcNAT(HWND hwnd, LPARAM lParam) {
    char className[256] = { 0 };
    GetClassNameA(hwnd, className, sizeof(className));

    if (strcmp(className, "TaskSwitcherWnd") == 0 || strcmp(className, "MultitaskingViewFrame") == 0) {
        *reinterpret_cast<bool*>(lParam) = true;
        return FALSE; // Stop enumerating
    }

    return TRUE; // Continue enumerating
}

bool IsNativeATWDisplayed() {
    HWND hWnd = GetForegroundWindow();
    char className[256] = { 0 };
    GetClassNameA(hWnd, className, 256);
    return
       strcmp(className, "TaskSwitcherWnd") == 0 ||
       strcmp(className, "MultitaskingViewFrame") == 0;
}

BOOL IsHungAppWindowEx(HWND hwnd) {
    if (g_pfnIsHungAppWindow && g_pfnIsHungAppWindow(hwnd)) {
        std::string title = GetWindowTitleExA(hwnd);
        AT_LOG_INFO("IsHungWnd: [%s]", title.c_str());
        return (TRUE);
    }

    LRESULT lResult = SendMessageTimeoutW(hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 249, nullptr);
    if (lResult)
        return (FALSE);

    DWORD dwErr = GetLastError();
    return (dwErr == 0 || dwErr == 1460);
}

/*!
 * Open the file with the default associated program
 * 
 * \param fileName   File name
 */
void ShellExecuteEx(const std::wstring& fileName) {
    std::filesystem::path filePath = GetAppDirPath();
    filePath.append(fileName);

    // Use ShellExecute to open the file with the default associated program
    HINSTANCE hInstance = ShellExecuteW(nullptr, L"open", filePath.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    if ((INT_PTR)hInstance > 32) {
        // ShellExecute returns a value greater than 32 if successful
        AT_LOG_INFO("File opened successfully!");
    } else {
        // Otherwise, it indicates an error
        AT_LOG_ERROR("Failed to open file!");
        LogLastErrorInfo();
    }
}

void ShowHelpWindow() {
    ShellExecuteEx(L"AltTab.chm");
}

void ShowReadMeWindow() {
    ShellExecuteEx(L"ReadMe.txt");
}

void ShowReleaseNotesWindow() {
    ShellExecuteEx(L"ReleaseNotes.txt");
}

std::wstring GetAppDirPath() {
    wchar_t szPath[MAX_PATH] = { 0 };
    GetModuleFileNameW(g_hInstance, szPath, MAX_PATH);
    std::filesystem::path dirPath = szPath;
    return dirPath.parent_path().wstring();
}

void LogLastErrorInfo() {
    // Get the last error code
    DWORD errorCode = GetLastError();

    // Get the error message
    LPVOID errorMessage;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        errorCode,
        0,
        reinterpret_cast<LPWSTR>(&errorMessage),
        0,
        nullptr);
    std::wstring ret = (errorMessage ? reinterpret_cast<LPCWSTR>(errorMessage) : L"");
    AT_LOG_ERROR("  Error Code   : %d", errorCode);
    AT_LOG_ERROR("  Error Message: %s", WStrToUTF8(ret).c_str());
}

// Function to create and show a custom tooltip at the mouse location
void CreateCustomToolTip() {
    AT_LOG_TRACE;
    // Create a tooltip window
    g_hCustomToolTip = CreateWindowExW(
        WS_EX_TOPMOST,
        TOOLTIPS_CLASSW,
        nullptr,
        TTS_NOPREFIX | TTS_ALWAYSTIP,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        g_hInstance,
        nullptr);

    if (!g_hCustomToolTip) {
        AT_LOG_ERROR("Failed to create tooltip window.");
        LogLastErrorInfo();
        return;
    }

	 // Initialize members of the toolinfo structure
    g_ToolInfo.cbSize      = sizeof(TOOLINFO);
    g_ToolInfo.uFlags      = TTF_TRACK;
    g_ToolInfo.hwnd        = nullptr;
    g_ToolInfo.hinst       = nullptr;
    g_ToolInfo.uId         = 0;
    g_ToolInfo.lpszText    = (LPWSTR)L"Creating tooltip...";

    // ToolTip control will cover the whole window
    g_ToolInfo.rect.left   = 0;
    g_ToolInfo.rect.top    = 0;
    g_ToolInfo.rect.right  = 0;
    g_ToolInfo.rect.bottom = 0;

	 // Send an add tool message to the tooltip control window
    SendMessage(g_hCustomToolTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&g_ToolInfo);

    // Enable multiple lines
    SendMessage(g_hCustomToolTip, TTM_SETMAXTIPWIDTH, 0, MAXINT);

    SendMessage(g_hCustomToolTip, TTM_SETTIPBKCOLOR, RGB(255, 255, 0), 0);
}

DWORD WINAPI ShowCustomToolTipThread(LPVOID pvParam) {
    AT_LOG_TRACE;

    // Get mouse coordinates
    POINT pt;
    GetCursorPos(&pt);

    ToolTipInfo* tti    = (ToolTipInfo*)pvParam;
    AT_LOG_INFO("tooltip: %s, duration: %d", WStrToUTF8(tti->ToolTipText).c_str(), tti->Duration);
    int duration        = tti->Duration;
    g_ToolInfo.lpszText = (LPWSTR)tti->ToolTipText.c_str();

    SendMessage(g_hCustomToolTip, TTM_SETTOOLINFO  ,    0, (LPARAM)&g_ToolInfo);
    SendMessage(g_hCustomToolTip, TTM_TRACKPOSITION,    0, (LPARAM)(DWORD)MAKELONG(pt.x + 12, pt.y + 12));
    SendMessage(g_hCustomToolTip, TTM_TRACKACTIVATE, true, (LPARAM)(LPTOOLINFO)&g_ToolInfo);
    if (duration != -1) {
        AT_LOG_INFO("Start TIMER_CUSTOM_TOOLTIP timer");
        g_TooltipTimerId = SetTimer(nullptr, TIMER_CUSTOM_TOOLTIP, duration, HideCustomToolTip);
    }
    return 0;
}

void ShowCustomToolTip(const std::wstring& tooltipText, int duration /*= 3000*/) {
    AT_LOG_TRACE;
#if 0
    // TODO: Still this is not working properly so going with alternative
    ToolTipInfo* tti = new ToolTipInfo { tooltipText, duration };
    //std::thread tooltipThread(ShowCustomToolTipThread, (LPVOID)&tti);
    //tooltipThread.detach();
    CreateThread(nullptr, 0, ShowCustomToolTipThread, (LPVOID)tti, 0, nullptr);
#else
    if (!g_TooltipVisible) {
       // Get mouse coordinates
       POINT pt;
       GetCursorPos(&pt);

       g_ToolInfo.lpszText = (LPWSTR)(LPCWSTR)tooltipText.c_str();
       SendMessage(g_hCustomToolTip, TTM_SETTOOLINFO,      0, (LPARAM)&g_ToolInfo);
       SendMessage(g_hCustomToolTip, TTM_TRACKPOSITION,    0, (LPARAM)(DWORD)MAKELONG(pt.x + 12, pt.y + 12));
       SendMessage(g_hCustomToolTip, TTM_TRACKACTIVATE, true, (LPARAM)(LPTOOLINFO)&g_ToolInfo);
   
       g_TooltipTimerId = SetTimer(nullptr, TIMER_CUSTOM_TOOLTIP, duration, HideCustomToolTip);
       g_TooltipVisible = true;
    }

    //// Get mouse coordinates
    //POINT pt;
    //GetCursorPos(&pt);

    //g_ToolInfo.lpszText = (LPWSTR)(LPCWSTR)tooltipText.c_str();
    //SendMessage(g_hCustomToolTip, TTM_SETTOOLINFO,      0, (LPARAM)&g_ToolInfo);
    //SendMessage(g_hCustomToolTip, TTM_TRACKPOSITION,    0, (LPARAM)(DWORD)MAKELONG(pt.x + 12, pt.y + 12));
    //SendMessage(g_hCustomToolTip, TTM_TRACKACTIVATE, true, (LPARAM)(LPTOOLINFO)&g_ToolInfo);
   
    //g_TooltipTimerId = SetTimer(nullptr, TIMER_CUSTOM_TOOLTIP, duration, HideCustomToolTip);
#endif // 0
}

void CALLBACK HideCustomToolTip(HWND /*hWnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/) {
    AT_LOG_TRACE;
    KillTimer(nullptr, g_TooltipTimerId);
    SendMessage(g_hCustomToolTip, TTM_TRACKACTIVATE, false, (LPARAM)(LPTOOLINFO)&g_ToolInfo);
    g_TooltipVisible = false;
}

int GetCurrentYear() {
    // Get the current time
    std::time_t currentTime = std::time(nullptr);

    // Convert the current time to a std::tm structure
    std::tm* localTime = std::localtime(&currentTime);

    // Extract the year from the tm structure
    int currentYear = localTime->tm_year + 1900;

    return currentYear;
}
