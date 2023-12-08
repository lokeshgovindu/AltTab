// AltTab.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "AltTab.h"
#include "Logger.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include "Utils.h"
#include "AltTabWindow.h"
#include <string>
#include <format>
#include "AltTabSettings.h"
#include <unordered_set>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE      g_hInstance;                              // Current instance
WCHAR          szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR          szWindowClass[MAX_LOADSTRING];            // The main window class name
HHOOK          kbdhook;                                  // Keyboard Hook
HWND           g_hAltTabWnd      = nullptr;              // AltTab window handle
bool           g_IsAltTab        = false;                // Is Alt+Tab pressed
bool           g_IsAltBacktick   = false;                // Is Alt+Backtick pressed

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK    About           (HWND, UINT, WPARAM, LPARAM);

void ATLoadSettings() {
    AT_LOG_TRACE;
    std::wstring settingsFilePath = L"AltTabSettings.ini";
    auto vs = Split(g_Settings.SimilarProcessGroups, L"|");
    for (auto& item : vs) {
        auto processes = Split(item, L"/");
        g_Settings.ProcessGroupsList.emplace_back(processes.begin(), processes.end());
    }
}

int APIENTRY wWinMain(
    _In_        HINSTANCE   hInstance,
    _In_opt_    HINSTANCE   hPrevInstance,
    _In_        LPWSTR      lpCmdLine,
    _In_        int         nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _AT_LOGGER
    CreateLogger();
    gLogger->info("createLogger done.");
#endif // _AT_LOGGER

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle,       MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ALTTAB,    szWindowClass, MAX_LOADSTRING);
    //MyRegisterClass(hInstance);

    ATLoadSettings();

    g_hInstance = hInstance; // Store instance handle in our global variable

    kbdhook = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, hInstance, NULL);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ALTTAB));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void ActivateWindow(HWND hWnd) {
    // Bring the window to the foreground
    if (!BringWindowToTop(hWnd)) {
        // Failed to bring an elevated window to the top from a non-elevated process.
        AT_LOG_INFO("BringWindowToTop(hWnd) failed!");

        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
    }
}

void DestoryAltTabWindow() {
    AT_LOG_TRACE;

    DestroyWindow(g_hAltTabWnd);

    // CleanUp
    g_hAltTabWnd    = nullptr;
    g_IsAltTab      = false;
    g_IsAltBacktick = false;
    g_AltTabWindows.clear();
}

LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    //AT_LOG_TRACE;
    //AT_LOG_INFO(std::format("hAltTabWnd is nullptr: {}", hAltTabWnd == nullptr).c_str());
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* pKeyboard = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        
        //AT_LOG_INFO(std::format("wParam: {}, vkCode: {}", wParam, pKeyboard->vkCode).c_str());

        // Check if Alt key is pressed
        bool isAltPressed = GetAsyncKeyState(VK_MENU) & 0x8000;
        //AT_LOG_INFO(std::format("isAltPressed: {}", isAltPressed).c_str());

        if (isAltPressed) {
            // Check if Shift key is pressed
            bool isShiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
            int  direction      = isShiftPressed ? -1 : 1;

            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                if (g_hAltTabWnd == nullptr) {
                    if (pKeyboard->vkCode == VK_TAB) {
                        g_IsAltTab = true;
                        g_IsAltBacktick = false;
                        if (isShiftPressed) {
                            // Alt+Shift+Tab is pressed
                            AT_LOG_INFO("Alt+Shift+Tab Pressed!");
                            ShowAltTabWindow(g_hAltTabWnd, -1);
                        } else {
                            // Alt+Tab is pressed
                            AT_LOG_INFO("Alt+Tab Pressed!");
                            ShowAltTabWindow(g_hAltTabWnd, 1);
                        }
                        return TRUE;
                    } else if (pKeyboard->vkCode == 0xC0) {
                        g_IsAltTab = false;
                        g_IsAltBacktick = true;
                        if (isShiftPressed) {
                            // Alt+Shift+Backtick is pressed
                            AT_LOG_INFO("Alt+Shift+Backtick Pressed!");
                            ShowAltTabWindow(g_hAltTabWnd, -1);
                        } else {
                            // Alt+Backtick is pressed
                            AT_LOG_INFO("Alt+Backtick Pressed!");
                            ShowAltTabWindow(g_hAltTabWnd, 1);
                        }
                        return TRUE;
                    }
                }
                else {
                    if (pKeyboard->vkCode == VK_TAB) {
                        AT_LOG_INFO("Tab Pressed!");
                        ShowAltTabWindow(g_hAltTabWnd, direction);
                        return TRUE;
                    }
                    else if (pKeyboard->vkCode == VK_DOWN) {
                        AT_LOG_INFO("Down Pressed!");
                        ShowAltTabWindow(g_hAltTabWnd, 1);
                        return TRUE;
                    }
                    else if (pKeyboard->vkCode == VK_UP) {
                        AT_LOG_INFO("Up Pressed!");
                        ShowAltTabWindow(g_hAltTabWnd, -1);
                        return TRUE;
                    }
                    else if (pKeyboard->vkCode == VK_ESCAPE) {
                        AT_LOG_INFO("Escape Pressed!");
                        DestoryAltTabWindow();
                        return TRUE;
                    }
                    else if (pKeyboard->vkCode == VK_HOME || pKeyboard->vkCode == VK_PRIOR) {
                        AT_LOG_INFO("Home/PageUp Pressed!");
                        if (!g_AltTabWindows.empty()) {
                            ATWListViewSelectItem(0);
                        }
                        return TRUE;
                    }
                    else if (pKeyboard->vkCode == VK_END || pKeyboard->vkCode == VK_NEXT) {
                        AT_LOG_INFO("End/PageDown Pressed!");
                        //ATWListViewPageDown();
                        if (!g_AltTabWindows.empty()) {
                            ATWListViewSelectItem((int)g_AltTabWindows.size() - 1);
                        }
                        return TRUE;
                    }
                    else if (pKeyboard->vkCode == VK_DELETE) {
                        AT_LOG_INFO("Delete Pressed!");
                        // Send the SC_CLOSE command to the window
                        int  ind  = ATWListViewGetSelectedItem();
                        HWND hWnd = g_AltTabWindows[ind].hWnd;
                        g_AltTabWindows.erase(g_AltTabWindows.begin() + ind);
                        SendMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                        ATWListViewDeleteItem(ind);
                        return TRUE;
                    }
                    else if (pKeyboard->vkCode == 0xC0) {   // '`~' for US
                        AT_LOG_INFO("Backtick Pressed!");

                        // Move to next / previous same item based on the direction
                        int selectedInd = ATWListViewGetSelectedItem();
                        int N           = (int)g_AltTabWindows.size();
                        int nextInd     = -1;

                        for (int i = 1; i < N; ++i) {
                            nextInd = (selectedInd + N + i * direction) % N;
                            if (g_AltTabWindows[selectedInd].ProcessName == g_AltTabWindows[nextInd].ProcessName) {
                                break;
                            }
                            nextInd = -1;
                        }

                        if (nextInd != -1) ATWListViewSelectItem(nextInd);
                        return TRUE;
                    }
                    else {
                        AT_LOG_WARN(std::format("NotHandled: wParam: {:#x}, vkCode: {:#x}", wParam, pKeyboard->vkCode).c_str());
                    }
                }
            }
        }

#if 1
        // Check for Alt key released event
        if (isAltPressed &&
            g_hAltTabWnd &&
            (pKeyboard->vkCode == VK_MENU || pKeyboard->vkCode == VK_LMENU || pKeyboard->vkCode == VK_RMENU)) {
            if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                // Alt key released, destroy your window
                AT_LOG_INFO("Alt key released!");
                if (g_hAltTabWnd) {
                    int selectedInd = ATWListViewGetSelectedItem();
                    if (selectedInd != -1) {
                        HWND hWnd = g_AltTabWindows[selectedInd].hWnd;
                        ActivateWindow(hWnd);
                    }
                    DestoryAltTabWindow();
                }
            }
        }
#endif // 0
    }

    // Call the next hook in the chain
    return CallNextHookEx(kbdhook, nCode, wParam, lParam);
}
