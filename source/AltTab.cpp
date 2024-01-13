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

#define MAX_LOADSTRING 100

// ----------------------------------------------------------------------------
// Global Variables:
// ----------------------------------------------------------------------------
HINSTANCE   g_hInstance;                                 // Current instance
HHOOK       g_KeyboardHook;                              // Keyboard Hook
HWND        g_hAltTabWnd         = nullptr;              // AltTab window handle
HWND        g_hFGWnd             = nullptr;              // Foreground window handle
HWND        g_hMainWnd           = nullptr;              // AltTab main window handle
bool        g_IsAltTab           = false;                // Is Alt+Tab pressed
bool        g_IsAltBacktick      = false;                // Is Alt+Backtick pressed
DWORD       g_MainThreadID       = GetCurrentThreadId(); // Main thread ID
DWORD       g_idThreadAttachTo   = 0;

IsHungAppWindowFunc g_pfnIsHungAppWindow = nullptr;

UINT const  WM_USER_ALTTAB_TRAYICON = WM_APP + 1;

HWND CreateMainWindow(HINSTANCE hInstance);
BOOL AddNotificationIcon(HWND hWndTrayIcon);
void CALLBACK CheckAltKeyIsReleased(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

// ----------------------------------------------------------------------------
// Main
// ----------------------------------------------------------------------------
int APIENTRY wWinMain(
    _In_        HINSTANCE   hInstance,
    _In_opt_    HINSTANCE   hPrevInstance,
    _In_        LPWSTR      lpCmdLine,
    _In_        int         nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

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
    gLogger->info("-------------------------------------------------------------------------------");
    gLogger->info("CreateLogger done.");
#endif // _AT_LOGGER

    g_hInstance = hInstance; // Store instance handle in our global variable

    // Load settings from AltTabSettings.ini file
    ATLoadSettings();

    // Run At Startup
    //RunAtStartup(true);

    // System tray
    // Create a hidden window for tray icon handling
    g_hMainWnd = CreateMainWindow(hInstance);

    // TODO: This is a temporary solution to fix for application focus issue.
    // https://stackoverflow.com/questions/22422708/popup-window-unable-to-receive-focus-when-top-level-window-is-minimized
    //g_hAltTabWnd = CreateAltTabWindow();
    //DestoryAltTabWindow();

	 HINSTANCE hinstUser32 = LoadLibrary(L"user32.dll");
    if (hinstUser32) {
       g_pfnIsHungAppWindow = (IsHungAppWindowFunc)GetProcAddress(hinstUser32, "IsHungAppWindow");
    }


    // Add the tray icon
    if (!AddNotificationIcon(g_hMainWnd)) {
        AT_LOG_ERROR("Failed to add AltTab tray icon.");
    }

    g_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, hInstance, NULL);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALTTAB));

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

    } break;

    case WM_USER_ALTTAB_TRAYICON:
        switch (LOWORD(lParam)) {
            case WM_RBUTTONDOWN: {
                POINT pt;
                GetCursorPos(&pt);
                ShowTrayContextMenu(hWnd, pt);
            }
            break;
        }
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
        // Initialize the SysLink control
        HWND hSysLink = GetDlgItem(hDlg, IDC_SYSLINK1);

        // Set the link text and URL
        SendMessage(hSysLink, LM_SETITEM, 0, (LPARAM)L"<a href=\"https://lokeshgovindu.github.io/AltTabAlternative/\">Visit AltTab's website</a>");
   
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
        if (wParam == IDC_SYSLINK1) {
            // Handle SysLink notifications
            NMHDR* pnmh = (NMHDR*)lParam;
            if (pnmh->code == NM_CLICK) {
                ShellExecute(NULL, L"open", L"https://lokeshgovindu.github.io/AltTabAlternative/", NULL, NULL, SW_SHOWNORMAL);
            }
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// ----------------------------------------------------------------------------
// Activate window of the given window handle
// ----------------------------------------------------------------------------
void ActivateWindow(HWND hWnd) {
    AT_LOG_TRACE;

	 HWND hwndFrgnd = GetForegroundWindow();
    if (hWnd == hwndFrgnd) {
        return;
    }

    // Bring the window to the foreground
    // Determines whether the specified window is minimized (iconic).
    if (IsIconic(hWnd)) {
        //ShowWindow(hWnd, SW_RESTORE);
        PostMessage(hWnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    } else {
        BOOL result = SetForegroundWindow(hWnd);
        HWND hFGWnd = GetForegroundWindow();
        if (!result && hFGWnd != hWnd) {
            // Failed to bring an elevated window to the top from a non-elevated process.
            AT_LOG_ERROR("SetForegroundWindow(hWnd) failed!");

            ShowWindow(hWnd, SW_SHOW);
            result = BringWindowToTop(hWnd);
            HWND hFGWnd = GetForegroundWindow();
            if (!result && hFGWnd != hWnd) {
                AT_LOG_ERROR("BringWindowToTop(hWnd) failed!");
            }
        }
    }
    SetActiveWindow(hWnd);
}

// ----------------------------------------------------------------------------
// Destroy AltTab Window and do necessary cleanup here
// ----------------------------------------------------------------------------
void DestoryAltTabWindow(bool activate) {
    AT_LOG_TRACE;

    //// Wait for AltTab thread to finish
    //if (WaitForSingleObject(g_hAltTabThread, 0) != WAIT_OBJECT_0) {
    //    CloseHandle(g_hAltTabThread);
    //    g_hAltTabThread = nullptr;
    //}

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
        if (!IsHungAppWindowEx(hWnd)) {
            ActivateWindow(hWnd);
        }
    } else {
        DestroyWindow(g_hAltTabWnd);
    }
    
    // CleanUp
    g_hAltTabWnd    = nullptr;
    g_IsAltTab      = false;
    g_IsAltBacktick = false;
    g_SelectedIndex = -1;
    g_AltTabWindows.clear();
    g_SearchString.clear();
    g_AltBacktickWndInfo = {};
    AT_LOG_INFO("--------- DestoryAltTabWindow! ---------");
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
    KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    DWORD vkCode = pKeyboard->vkCode;
        
    // Check if Alt key is pressed
    bool isAltPressed  = GetAsyncKeyState(VK_MENU   ) & 0x8000;
    bool isCtrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;

    // ----------------------------------------------------------------------------
    // Alt key is pressed
    // ----------------------------------------------------------------------------
    if (isAltPressed && !isCtrlPressed && pKeyboard->flags & LLKHF_ALTDOWN) {
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
                // Alt + Tab
                // ----------------------------------------------------------------------------
                if (vkCode == VK_TAB) {
                    if (isNativeATWDisplayed) {
                        //AT_LOG_INFO("isNativeATWDisplayed: %d", isNativeATWDisplayed);
                        return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
                    }

                    g_IsAltTab = true;
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
                else if (vkCode == VK_OEM_3) { // 0xC0
                    g_IsAltTab = false;
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
                // Create timer here to check the Alt key is released.
                // ----------------------------------------------------------------------------
                if (g_IsAltTab || g_IsAltBacktick) {
                    SetTimer(g_hMainWnd, TIMER_CHECK_ALT_KEYUP, 50, CheckAltKeyIsReleased);
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
        }
    } // if (isAltPressed && !isCtrlPressed)

#if 0
    if (g_hAltTabWnd && (vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU)) {
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            // Alt key released, destroy your window
            AT_LOG_INFO("--------- Alt key released! ---------");
            DestoryAltTabWindow();
        }
        //return TRUE;
    }
#endif

#if 0
    // Check for Alt key released event
    if (g_hAltTabWnd && (vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU))
    {
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            // Alt key released, destroy your window
            AT_LOG_INFO("--------- Alt key released! ---------");
            Sleep(100);
            if (g_hAltTabWnd) {
                int selectedInd = ATWListViewGetSelectedItem();
                ShowWindow(g_hAltTabWnd, SW_HIDE);
                Sleep(10);
                HWND hWnd = nullptr;
                if (selectedInd != -1) {
                    hWnd = g_AltTabWindows[selectedInd].hWnd;
                    AT_LOG_INFO("hWnd = %#x", hWnd);
                }
                DestoryAltTabWindow();

                // Simulate Alt keyup event using keybd_event
                //keybd_event(VK_MENU, 0, 0, 0);
                //Sleep(10);
                AT_LOG_INFO("hWnd = %#x", hWnd);
                if (hWnd) {
                    ActivateWindow(hWnd);
                }
                //keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
                //keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
                //PostMessageW(hWnd, WM_KEYUP, VK_LMENU, 0);
                //Sleep(10);
                //return TRUE;
            }
        }
    }
#endif

    //AT_LOG_INFO("CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);");
    return CallNextHookEx(g_KeyboardHook, nCode, wParam, lParam);
}

// Timer callback function
void CALLBACK CheckAltKeyIsReleased(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
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

    case ID_TRAYCONTEXTMENU_CHECKFORUPDATES:
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_CHECKFORUPDATES");
        break;

    case ID_TRAYCONTEXTMENU_RUNATSTARTUP: {
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_RUNATSTARTUP");
        UINT checkState = GetCheckState(hSubMenu, menuItemId);
        bool checked    = (checkState == MF_CHECKED);
        RunAtStartup(!checked);
        ToggleCheckState(hSubMenu, menuItemId);
    }
    break;

    case ID_TRAYCONTEXTMENU_RESTART: {
        AT_LOG_INFO("ID_TRAYCONTEXTMENU_RESTART");

        // Close the console window of the old process
        //bool result = FreeConsole();
        // Attach to an existing console (if any)
        //if (AttachConsole(ATTACH_PARENT_PROCESS) || AttachConsole(GetCurrentProcessId())) {
        //    // Close the console window
        //    FreeConsole();
        //}

        wchar_t applicationPath[MAX_PATH];
        DWORD length = GetModuleFileName(nullptr, applicationPath, MAX_PATH);

        // Replace the existing process with a new instance of the same program
        _wexecl(applicationPath, applicationPath, nullptr);
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
    HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDC_TRAY_CONTEXTMENU));
    if (hMenu) {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu) {
            // our window must be foreground before calling TrackPopupMenu or
            // the menu will not disappear when the user clicks away
            SetForegroundWindow(hWnd);

            if (IsRunAtStartup()) {
                SetCheckState(hSubMenu, ID_TRAYCONTEXTMENU_RUNATSTARTUP, MF_CHECKED);
            }

            if (g_Settings.DisableAltTab) {
                SetCheckState(hSubMenu, ID_TRAYCONTEXTMENU_DISABLEALTTAB, MF_CHECKED);
            }

            // respect menu drop alignment
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

bool RunAtStartup(bool flag) {
    wchar_t applicationPath[MAX_PATH];
    DWORD length = GetModuleFileName(nullptr, applicationPath, MAX_PATH);

    if (length == 0) {
        AT_LOG_ERROR("Failed to retrieve the path of the currently running process.");
        return false;
    }

    HKEY hKey;
    LONG result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_SET_VALUE,
        &hKey);

    if (result == ERROR_SUCCESS) {
        if (flag) {
            result = RegSetValueExW(
                hKey,
                AT_PRODUCT_NAMEW,
                0,
                REG_SZ,
                (const BYTE*)applicationPath,
                (DWORD)(wcslen(applicationPath) + 1) * sizeof(wchar_t));
            RegCloseKey(hKey);
            return (result == ERROR_SUCCESS);
        } else {
            result = RegDeleteValue(hKey, AT_PRODUCT_NAMEW);
            RegCloseKey(hKey);
            return (result == ERROR_SUCCESS);
        }
    }

    return false;
}

bool IsRunAtStartup() {
    HKEY hKey;
    LONG result = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0,
        KEY_READ,
        &hKey);

    if (result == ERROR_SUCCESS) {
        // Check if the registry entry exists
        DWORD dataSize = 0;
        result = RegQueryValueExW(hKey, AT_PRODUCT_NAMEW, nullptr, nullptr, nullptr, &dataSize);
        RegCloseKey(hKey);

        return (result == ERROR_SUCCESS);
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
#if 1
        std::string title = GetWindowTitleExA(hwnd);
        AT_LOG_INFO("IsHungWnd: [%s]", title.c_str());
#endif
        return (TRUE);
   }

    LRESULT lResult = SendMessageTimeoutW(hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 249, nullptr);
    if (lResult)
        return (FALSE);

    DWORD dwErr = GetLastError();
    return (dwErr == 0 || dwErr == 1460);
}
