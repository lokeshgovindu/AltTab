// AltTabWindow.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "AltTabWindow.h"

#include "Logger.h"

#include <CommCtrl.h>
#include <Psapi.h>
#include <WinUser.h>
#include <Windows.h>
#include <dwmapi.h>
#include <filesystem>
#include <iostream>
#include <shellapi.h>
#include <shlobj.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <winnt.h>
#include "Resource.h"
#include "AltTabSettings.h"
#include "Utils.h"
#include "AltTab.h"
#include "GlobalData.h"
#include <unordered_map>
#include "version.h"


HWND           g_hStaticText       = nullptr;
HWND           g_hListView         = nullptr;
HFONT          g_hFont             = nullptr;
int            g_SelectedIndex     = 0;
int            g_MouseHoverIndex   = -1;
HANDLE         g_hAltTabThread     = nullptr;
std::wstring   g_SearchString;
const int      COL_ICON_WIDTH      = 36;
const int      COL_PROCNAME_WIDTH  = 180;

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK ATAboutDlgProc        (HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AltTabWindowProc      (HWND, UINT, WPARAM, LPARAM);
bool             IsAltTabWindow        (HWND hWnd);
HWND             GetOwnerWindowHwnd    (HWND hWnd);
static void      AddListViewItem       (HWND hListView, int index, const AltTabWindowData& windowData);
static void      ContextMenuItemHandler(HWND hWnd, HMENU hSubMenu, UINT menuItemId);
static BOOL      TerminateProcessEx    (DWORD pid);
static bool      IsExcludedProcess     (const std::wstring& processName);

std::vector<AltTabWindowData> g_AltTabWindows;

HICON GetWindowIcon(HWND hWnd) {
    // Try to get the large icon
    HICON hIcon;
    // Use SendMessageTimeout to get the icon with a timeout
    LRESULT responding = SendMessageTimeout(hWnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 10, reinterpret_cast<PDWORD_PTR>(&hIcon));
    if (responding) {
        if (hIcon)
            return hIcon;

        // If the large icon is not available, try to get the small icon
        hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL2, 0);
        if (hIcon)
            return hIcon;

        hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
        if (hIcon)
            return hIcon;
    }

    // Get the class long value that contains the icon handle
    hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
    if (!hIcon) {
        // If the class does not have an icon, try to get the small icon
        hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
    }

    if (!hIcon) {
        hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        return hIcon;
    }

    return hIcon;
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    if (IsAltTabWindow(hWnd)) {
        HWND   hOwner = GetOwnerWindowHwnd(hWnd);
        DWORD  processId;
        GetWindowThreadProcessId(hWnd, &processId);

        if (processId != 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);

            if (hProcess != nullptr) {
                wchar_t szProcessPath[MAX_PATH];
                if (GetModuleFileNameEx(hProcess, nullptr, szProcessPath, MAX_PATH)) {
                    std::filesystem::path filePath = szProcessPath;

                    // Always get the title of owner window, otherwise popup window title will be displayed.
                    const int bufferSize = 256;
                    wchar_t windowTitle[bufferSize];
                    GetWindowTextW(hOwner, windowTitle, bufferSize);

                    // We get the actual window title for the process but windows adds "(Not Responding)"
                    // to the processes which are hung. So, appending "(Not Responding)" to the hung process manually.
                    std::wstring title = windowTitle;
                    if (IsHungAppWindow(hWnd)) {
                        title += L" (Not Responding)";
                    }
                    AltTabWindowData item = { hWnd,
                                              hOwner,
                                              GetWindowIcon(hOwner),
                                              title,
                                              filePath.filename().wstring(),
                                              filePath.wstring(),
                                              processId };
                    auto vItems   = (std::vector<AltTabWindowData>*)lParam;
                    bool insert   = false;
                    bool excluded = IsExcludedProcess(ToLower(item.ProcessName));

                    // If Alt+Tab is pressed, show all windows
                    if (g_IsAltTab) {
                        insert = excluded ? false : true;
                    }
                    // If Alt+Backtick is pressed, show the process of similar process groups
                    else if (g_IsAltBacktick) {
                        if (g_AltBacktickWndInfo.hWnd == nullptr) {
                            g_AltBacktickWndInfo = item;
                            AT_LOG_INFO("g_AltBacktickWndInfo: %s", WStrToUTF8(g_AltBacktickWndInfo.ProcessName).c_str());
                        }
                        insert = IsSimilarProcess(g_AltBacktickWndInfo.ProcessName, item.ProcessName);
                    }

                    // If the window is to be inserted, now apply the search string
                    if (insert && !g_SearchString.empty()) {
                        insert = InStr(item.ProcessName, g_SearchString) || InStr(item.Title, g_SearchString);

                        double matchRatio = insert ? 100 : 0;
                        // Search in process name
                        if (!insert && g_Settings.FuzzyMatchPercent != 100) {
                            matchRatio = GetPartialRatioW(g_SearchString, item.ProcessName);
                            if (matchRatio >= g_Settings.FuzzyMatchPercent) {
                                insert = true;
                            }
                        }
                        // Search in window title
                        if (!insert && g_Settings.FuzzyMatchPercent != 100) {
                            matchRatio = GetPartialRatioW(g_SearchString, item.Title);
                            if (matchRatio >= g_Settings.FuzzyMatchPercent) {
                                insert = true;
                            }
                        }
                        //AT_LOG_INFO("matchRatio = %5.1f, title = [%s]", matchRatio, WStrToUTF8(item.Title).c_str());
                    }

                    if (insert) {
                        //AT_LOG_INFO("Inserting hWnd: %0#9x, title: %s", item.hWnd, WStrToUTF8(item.Title).c_str());
                        vItems->push_back(std::move(item));
                    }
                }

                CloseHandle(hProcess);
            }
        }
    }

    return TRUE;
}

//int WINAPI ATWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
//    // Register the window class
//    const wchar_t CLASS_NAME[] = L"AltTab";
//
//    WNDCLASS wc      = {};
//    wc.lpfnWndProc   = AltTabWindowProc;
//    wc.hInstance     = hInstance;
//    wc.lpszClassName = CLASS_NAME;
//
//    RegisterClass(&wc);
//
//    DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
//    DWORD style   = WS_POPUP | WS_VISIBLE | WS_BORDER;
//
//    // Create the window
//    HWND hwnd = CreateWindowExW(
//        exStyle,    // Optional window styles
//        CLASS_NAME, // Window class
//        CLASS_NAME, // Window title
//        style,      // Styles
//        100,
//        100,
//        400,
//        300,       // Position and size
//        nullptr,   // Parent window
//        nullptr,   // Menu
//        hInstance, // Instance handle
//        nullptr    // Additional application data
//    );
//
//    if (hwnd == NULL) {
//        return 0;
//    }
//
//    // Show the window
//    ShowWindow(hwnd, nCmdShow);
//    UpdateWindow(hwnd);
//
//    // Message loop
//    MSG msg = {};
//    while (GetMessage(&msg, nullptr, 0, 0)) {
//        TranslateMessage(&msg);
//        DispatchMessage(&msg);
//    }
//
//    return 0;
//}

DWORD WINAPI AltTabThread(LPVOID pvParam) {
    AT_LOG_TRACE;
    int direction = *((int*)pvParam);
    CreateAltTabWindow();
    //ShowAltTabWindow(g_hAltTabWnd, direction);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

HWND ShowAltTabWindow(HWND& hAltTabWnd, int direction) {
#if 0
    if (g_hAltTabThread && WaitForSingleObject(g_hAltTabThread, 0) != WAIT_OBJECT_0) {
        // Move to next / previous item based on the direction
        int selectedInd = (int)SendMessageW(g_hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
        int N = (int)g_AltTabWindows.size();
        int nextInd = (selectedInd + N + direction) % N;

        ATWListViewSelectItem(nextInd);

        HWND hWnd = GetForegroundWindow();
        if (hAltTabWnd != hWnd) {
            AT_LOG_ERROR("hAltTabWnd is NOT a foreground window!");
            ActivateWindow(hAltTabWnd);
        }
        return hAltTabWnd;
    }
    if (g_hAltTabThread) {
        CloseHandle(g_hAltTabThread);
        g_hAltTabThread = nullptr;
    }

	g_hAltTabThread = CreateThread(nullptr, 0, AltTabThread, (LPVOID)(UINT_PTR)direction, CREATE_SUSPENDED, nullptr);
    if (!g_hAltTabThread)
        return nullptr;

    ResumeThread(g_hAltTabThread);

    return hAltTabWnd;
#else
    if (hAltTabWnd == nullptr) {
        // We need this to set the AltTab window active when it is brought to the foreground
        g_hFGWnd = GetForegroundWindow();
        if (!IsHungAppWindowEx(g_hFGWnd)) {
            g_idThreadAttachTo = g_hFGWnd ? GetWindowThreadProcessId(g_hFGWnd, nullptr) : 0;
            if (g_idThreadAttachTo) {
                AttachThreadInput(GetCurrentThreadId(), g_idThreadAttachTo, TRUE);
            }
        }

        hAltTabWnd = CreateAltTabWindow();

        //SetWindowPos(hAltTabWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        SetForegroundWindow(hAltTabWnd);
    }

    // Move to next / previous item based on the direction
    int selectedInd = (int)SendMessageW(g_hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
    if (selectedInd == -1) return hAltTabWnd;
    int N           = (int)g_AltTabWindows.size();
    int nextInd     = (selectedInd + N + direction) % N;

    ATWListViewSelectItem(nextInd);

    TRACKMOUSEEVENT tme;
    tme.cbSize    = sizeof(tme);
    tme.dwFlags   = TME_LEAVE;
    tme.hwndTrack = hAltTabWnd;
    TrackMouseEvent(&tme);

    return hAltTabWnd;
#endif // 0
}

void RefreshAltTabWindow() {
    AT_LOG_TRACE;

    // Clear the list
    ListView_DeleteAllItems(g_hListView);
    g_AltTabWindows.clear();

    // Enumerate windows
    EnumWindows(EnumWindowsProc, (LPARAM)(&g_AltTabWindows));

    // Create ImageList and add icons
    HIMAGELIST hImageList =
        ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), ILC_COLOR32 | ILC_MASK, 0, 1);

    for (const auto& item : g_AltTabWindows) {
        ImageList_AddIcon(hImageList, item.hIcon);
    }

    // Set the ImageList for the ListView
    ListView_SetImageList(g_hListView, hImageList, LVSIL_SMALL);

    // Add windows to ListView
    for (int i = 0; i < g_AltTabWindows.size(); ++i) {
        AddListViewItem(g_hListView, i, g_AltTabWindows[i]);
    }

    // Select the previously selected item
    ATWListViewSelectItem(g_SelectedIndex);
}

void ATWListViewSelectItem(int rowNumber) {
    if (g_AltTabWindows.empty()) {
        g_SelectedIndex = -1;
        return;
    }

    // Move to next / previous item based on the direction
    int selectedInd = (int)SendMessageW(g_hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
    LVITEM lvItem;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;
    SendMessageW(g_hListView, LVM_SETITEMSTATE, selectedInd, (LPARAM)&lvItem);

    rowNumber = max(0, min(rowNumber, (int)g_AltTabWindows.size() - 1));

    lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    SendMessageW(g_hListView, LVM_SETITEMSTATE  , rowNumber, (LPARAM)&lvItem);
    SendMessageW(g_hListView, LVM_ENSUREVISIBLE , rowNumber, (LPARAM)&lvItem);

    g_SelectedIndex = rowNumber;
}

void ATWListViewSelectPrevItem() {
    // Move to next / previous item based on the direction
    int selectedInd = (int)SendMessageW(g_hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
    if (selectedInd == -1) return;
    int N           = (int)g_AltTabWindows.size();
    int prevInd     = (selectedInd + N - 1) % N;

    LVITEM lvItem;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;
    SendMessageW(g_hListView, LVM_SETITEMSTATE, selectedInd, (LPARAM)&lvItem);

    prevInd = max(0, min(prevInd, (int)g_AltTabWindows.size() - 1));

    lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    SendMessageW(g_hListView, LVM_SETITEMSTATE  , prevInd, (LPARAM)&lvItem);
    SendMessageW(g_hListView, LVM_ENSUREVISIBLE , prevInd, (LPARAM)&lvItem);

    g_SelectedIndex = prevInd;

}
void ATWListViewSelectNextItem() {
    // Move to next / previous item based on the direction
    int selectedInd = (int)SendMessageW(g_hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
    if (selectedInd == -1) return;
    int N           = (int)g_AltTabWindows.size();
    int nextInd     = (selectedInd + N + 1) % N;

    LVITEM lvItem;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;
    SendMessageW(g_hListView, LVM_SETITEMSTATE, selectedInd, (LPARAM)&lvItem);

    nextInd = max(0, min(nextInd, (int)g_AltTabWindows.size() - 1));

    lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    SendMessageW(g_hListView, LVM_SETITEMSTATE  , nextInd, (LPARAM)&lvItem);
    SendMessageW(g_hListView, LVM_ENSUREVISIBLE , nextInd, (LPARAM)&lvItem);

    g_SelectedIndex = nextInd;
}

void ATWListViewDeleteItem(int rowNumber) {
    SendMessageW(g_hListView, LVM_DELETEITEM, rowNumber, 0);
    ATWListViewSelectItem(rowNumber);
}

int ATWListViewGetSelectedItem() {
    // Move to next / previous item based on the direction
    int selectedRow = (int)SendMessageW(g_hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
    return selectedRow;
}

void ATWListViewPageDown() {
    AT_LOG_TRACE;
    // Scroll down one page (assuming item height is the default)
    SendMessage(g_hListView, LVM_SCROLL, 0, 1);

    // Optionally, you can add a delay if needed
    Sleep(100); // Sleep for 100 milliseconds

    // Scroll up one page
    SendMessage(g_hListView, LVM_SCROLL, 0, -1);

    //SendMessage(g_hListView, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);

    //int selectedRow = (int)SendMessageW(g_hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
    //LVITEM lvItem;
    //lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    //lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    //SendMessage(g_hListView, LVN_KEYDOWN, (WPARAM)VK_NEXT, (LPARAM)&lvItem);
}

HWND CreateAltTabWindow() {
    AT_LOG_TRACE;
    // Register the window class
    const wchar_t CLASS_NAME[]  = L"__AltTab_WndCls__";
    const wchar_t WINDOW_NAME[] = L"AltTab Window";

    WNDCLASS wc      = {};
    wc.lpfnWndProc   = AltTabWindowProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hInstance     = g_hInstance;

    RegisterClass(&wc);

    DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
    DWORD style   = WS_POPUP | WS_BORDER;

    // Create the window
    HWND hWnd = CreateWindowExW(
        exStyle,            // Optional window styles
        CLASS_NAME,         // Window class
        WINDOW_NAME,        // Window title
        style,              // Styles
        0,                  // X
        0,                  // Y
        0,                  // Width
        0,                  // Height
        g_hMainWnd,         // Parent window
        nullptr,            // Menu
        g_hInstance,        // Instance handle
        nullptr             // Additional application data
    );

    if (hWnd == nullptr) {
        AT_LOG_ERROR("Failed to create AltTab Window!");
        return nullptr;
    }

    // Show the window and bring to the top
    SetForegroundWindow(hWnd);
    SetWindowPos       (hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    ShowWindow         (hWnd, SW_SHOWNORMAL);
    UpdateWindow       (hWnd);
    BringWindowToTop   (hWnd);

    return hWnd;
}

void AddListViewItem(HWND hListView, int index, const AltTabWindowData& windowData) {
    LVITEM lvItem    = {0};
    lvItem.mask      = LVIF_TEXT | LVIF_IMAGE;
    lvItem.iItem     = index;
    lvItem.iSubItem  = 0;
    lvItem.iImage    = index;

    int nIndex = ListView_InsertItem(hListView, &lvItem);
    ListView_SetItem(hListView, &lvItem);
    ImageList_AddIcon(ListView_GetImageList(hListView, LVSIL_NORMAL), windowData.hIcon);
    ListView_SetItemText(hListView, index, 1, const_cast<wchar_t*>(windowData.Title.c_str()));
    ListView_SetItemText(hListView, index, 2, const_cast<wchar_t*>(windowData.ProcessName.c_str()));
}

static void CustomizeListView(HWND hListView) {
    // Set extended style for the List View control
    DWORD dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER;
    ListView_SetExtendedListViewStyle(hListView, dwExStyle);

    const int colTitleWidth = g_Settings.WindowWidth - COL_ICON_WIDTH;

    // Add columns to the List View
    LVCOLUMN lvCol   = {0};
    lvCol.mask       = LVCF_TEXT | LVCF_WIDTH;
    lvCol.pszText    = (LPWSTR)L"#";
    lvCol.cx         = COL_ICON_WIDTH;
    ListView_InsertColumn(hListView, 0, &lvCol);

    if (g_Settings.ShowColProcessName) {
        lvCol.pszText = (LPWSTR)L"Window Title";
        lvCol.cx      = colTitleWidth - COL_PROCNAME_WIDTH;
        ListView_InsertColumn(hListView, 1, &lvCol);

        lvCol.pszText = (LPWSTR)L"Process Name";
        lvCol.cx      = COL_PROCNAME_WIDTH - 2;
        ListView_InsertColumn(hListView, 2, &lvCol);
    } else {
        lvCol.pszText = (LPWSTR)L"Window Title";
        lvCol.cx      = colTitleWidth - 2;
        ListView_InsertColumn(hListView, 1, &lvCol);
    }
}

#define FONT_POINT(hdc, p) (-MulDiv(p, GetDeviceCaps(hdc, LOGPIXELSY), 72))

HFONT CreateFontEx(HDC hdc, const std::wstring& fontName, int fontSize, const std::wstring& fontStyle) {
    std::unordered_map<std::wstring, int> fontStyleMap = {
        { L"normal"     , FW_NORMAL },
        { L"italic"     , FW_NORMAL },
        { L"bold"       , FW_BOLD   },
        { L"bold italic", FW_BOLD   },
    };

    int  fStyle  = fontStyleMap[fontStyle];
    BOOL bItalic = fontStyle.find(L"italic") != -1;

    // Create a font for the static text control
    return CreateFontW(
        FONT_POINT(hdc, fontSize), // Font height
        0,                         // Width of each character in the font
        0,                         // Angle of escapement
        0,                         // Orientation angle
        fStyle,                    // Font weight
        bItalic,                   // Italic
        FALSE,                     // Underline
        FALSE,                     // Strikeout
        DEFAULT_CHARSET,           // Character set identifier
        OUT_DEFAULT_PRECIS,        // Output precision
        CLIP_DEFAULT_PRECIS,       // Clipping precision
        DEFAULT_QUALITY,           // Output quality
        DEFAULT_PITCH | FF_SWISS,  // Pitch and family
        fontName.c_str()           // Font face name
    );
}

static void SetListViewCustomFont(HWND hListView, const std::wstring& fontName, int fontSize) {
    LOGFONT lf;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, fontName.c_str());

    // Modify the font size
    lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(hListView), LOGPIXELSY), 72);

    g_hFont = CreateFontIndirect(&lf);

    SendMessage(hListView, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), MAKELPARAM(TRUE, 0));
}

static void SetListViewCustomColors(HWND hListView, COLORREF backgroundColor, COLORREF textColor) {
    // Set the background color
    SendMessage(hListView, LVM_SETBKCOLOR,     0, (LPARAM)backgroundColor);
    SendMessage(hListView, LVM_SETTEXTBKCOLOR, 0, (LPARAM)backgroundColor);

    // Set the text color
    SendMessage(hListView, LVM_SETTEXTCOLOR,   0, (LPARAM)textColor);
}

LRESULT CALLBACK ListViewSubclassProc(
    HWND       hListView,
    UINT       uMsg,
    WPARAM     wParam,
    LPARAM     lParam,
    UINT_PTR   uIdSubclass,
    DWORD_PTR  dwRefData)
{
    //AT_LOG_INFO(std::format("uMsg: {:4}, wParam: {}, lParam: {}", uMsg, wParam, lParam).c_str());
    auto vkCode = wParam;
    bool isShiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;

    switch (uMsg) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam == VK_ESCAPE) {
            AT_LOG_INFO("VK_ESCAPE");
            DestoryAltTabWindow();
        } else if (wParam == VK_DOWN) {
            AT_LOG_INFO("Down Pressed!");
            ATWListViewSelectNextItem();
            return TRUE;
        } else if (wParam == VK_UP) {
            AT_LOG_INFO("Up Pressed!");
            ATWListViewSelectPrevItem();
            return TRUE;
        } else if (vkCode == VK_HOME || vkCode == VK_PRIOR) {
            AT_LOG_INFO("Home/PageUp Pressed!");
            if (!g_AltTabWindows.empty()) {
                ATWListViewSelectItem(0);
            }
            return TRUE;
        } else if (vkCode == VK_END || vkCode == VK_NEXT) {
            AT_LOG_INFO("End/PageDown Pressed!");
            if (!g_AltTabWindows.empty()) {
                ATWListViewSelectItem((int)g_AltTabWindows.size() - 1);
            }
            return TRUE;
        }
        else if (vkCode == VK_DELETE) {
            int  direction      = isShiftPressed ? -1 : 1;

            if (isShiftPressed) {
                AT_LOG_INFO("Shift+Delete Pressed!");
                int ind = ATWListViewGetSelectedItem();
                if (ind == -1) return TRUE;
                TerminateProcessEx(g_AltTabWindows[ind].PID);
            } else {
                AT_LOG_INFO("Delete Pressed!");

                // Send the SC_CLOSE command to the window
                int ind = ATWListViewGetSelectedItem();
                if (ind == -1) return TRUE;
                AT_LOG_INFO("Ind: %d, Title: %s", ind, WStrToUTF8(g_AltTabWindows[ind].Title).c_str());
                HWND hWnd = g_AltTabWindows[ind].hWnd;
                PostMessage(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            }
            Sleep(100);
            RefreshAltTabWindow();
            return TRUE;
        }
        else if (vkCode == VK_OEM_3) {   // 0xC0 - '`~' for US
            AT_LOG_INFO("Backtick Pressed!, g_IsAltBacktick = %d", g_IsAltBacktick);
            const int  direction    = isShiftPressed ? -1 : 1;

            // Move to next / previous same item based on the direction
            const int   selectedInd = ATWListViewGetSelectedItem();
            if (selectedInd == -1) return TRUE;

            const int   N           = (int)g_AltTabWindows.size();
            const auto& processName = g_AltTabWindows[selectedInd].ProcessName; // Selected process name
            const int   pgInd       = GetProcessGroupIndex(processName);        // Index in ProcessGroupList
            int         nextInd     = (selectedInd + N + direction) % N;        // Next index to select

            // If the AltTab window is invoked with Alt + Backtick, then we should
            // move to the next item in the list without checking the process name.
            if (g_IsAltBacktick) {
                ATWListViewSelectItem(nextInd);
                return TRUE;
            }

            // If the control comes to here, AltTab window is invoked with Alt + Tab
            // Check if the next item is similar to the selected item
            for (int i = 1; i < N; ++i) {
                nextInd = (selectedInd + N + i * direction) % N;
                if (IsSimilarProcess(pgInd, g_AltTabWindows[nextInd].ProcessName)) {
                    break;
                }
                if (EqualsIgnoreCase(processName, g_AltTabWindows[nextInd].ProcessName)) {
                    break;
                }
                nextInd = -1;
            }

            if (nextInd != -1) ATWListViewSelectItem(nextInd);
            return TRUE;
        }
        else if (vkCode == VK_F1) {
            DestoryAltTabWindow();
            if (isShiftPressed) {
                DialogBoxW(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), nullptr, ATAboutDlgProc);
            } else {
                ShowHelpWindow();
            }
            return TRUE;
        }
        else if (isShiftPressed && vkCode == VK_F1) {
            DestoryAltTabWindow();
            DialogBoxW(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), nullptr, ATAboutDlgProc);
            return TRUE;
        }
        else if (vkCode == VK_F2) {
            DestoryAltTabWindow();
            // Do NOT assign owner for this, if owner is assigned and the settings
            // dialog is not active, then it won't be displayed in alt tab windows list.
            if (g_hSetingsWnd == nullptr) {
                DialogBoxW(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), nullptr, ATSettingsDlgProc);
            } else {
                SetForegroundWindow(g_hSetingsWnd);
            }
            return TRUE;
        }
        // WM_CHAR won't be sent when ALT is pressed, this is the alternative to handle when a key is pressed.
        // And collect the chars and form a search string.
        // Ignore Backtick (`), Tab, Backspace, and non-printable characters while forming search string.
        else {
            wchar_t ch = '\0';
            bool isChar = ATMapVirtualKey((UINT)wParam, ch);
            bool update = false;
            if (!(wParam == '`' || wParam == VK_TAB || wParam == VK_BACK || ch == '\0') && isChar) {
                g_SearchString += ch;
                update = true;
            } else if (wParam == VK_BACK && !g_SearchString.empty()) {
                g_SearchString.pop_back();
                update = true;
            }
            AT_LOG_INFO("Char: %#4x, SearchString: [%s]", ch, WStrToUTF8(g_SearchString).c_str());
            update && SendMessage(g_hStaticText, WM_SETTEXT, 0, (LPARAM)(L"Search String: " + g_SearchString).c_str());
        }
        //AT_LOG_INFO("Not handled: wParam: %0#4x, iswprint: %d", wParam, iswprint((wint_t)wParam));
        break;

    case WM_NOTIFY: {
        LPNMHDR nmhdr = reinterpret_cast<LPNMHDR>(lParam);
        if (nmhdr->code == LVN_ITEMCHANGED) {
            LPNMLISTVIEW pnmListView = reinterpret_cast<LPNMLISTVIEW>(nmhdr);

            if ((pnmListView->uChanged & LVIF_STATE) && (pnmListView->uNewState & LVIS_SELECTED)) {
                // The mouse has moved over the item
                // You can show a message or perform any action here
                MessageBox(hListView, L"Mouse moved over item!", L"ListView Notification", MB_OK | MB_ICONINFORMATION);
            }
        }
    } break;

    //case WM_KILLFOCUS:
    //    AT_LOG_INFO("WM_KILLFOCUS");

    //    ActivateWindow(g_hAltTabWnd);

    //    // Handle WM_KILLFOCUS to prevent losing selection when the window loses focus
    //    LVITEM lvItem;
    //    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    //    lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    //    SendMessage(hListView, LVM_SETITEMSTATE, -1, (LPARAM)&lvItem);
    //    break;
    }

    return DefSubclassProc(hListView, uMsg, wParam, lParam);
}

void WindowResizeAndPosition(HWND hWnd, int wndWidth, int wndHeight) {
    // Get the dimensions of the screen
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Calculate the position to center the window
    int xPos = (screenWidth - wndWidth) / 2;
    int yPos = (screenHeight - wndHeight) / 2;

    // Set the window position
    SetWindowPos(hWnd, HWND_TOP, xPos, yPos, wndWidth, wndHeight, SWP_NOSIZE | SWP_SHOWWINDOW);
}

int GetColProcessNameWidth() {
    if (g_Settings.ShowColProcessName) {
        return COL_PROCNAME_WIDTH;
    }
    return 0;
}

INT_PTR CALLBACK AltTabWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    //AT_LOG_TRACE;
    //AT_LOG_INFO(std::format("uMsg: {:4}, wParam: {}, lParam: {}", uMsg, wParam, lParam).c_str());

    switch (uMsg) {
    case WM_CREATE: {
        g_hAltTabWnd = hWnd;

        // Get screen width and height
        int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        // Compute the window size (e.g., 80% of the screen width and height)
        int windowWidth  = static_cast<int>(screenWidth  * g_Settings.WidthPercentage  * 0.01);
        int windowHeight = static_cast<int>(screenHeight * g_Settings.HeightPercentage * 0.01);

        // Compute the window position (centered on the screen)
        int windowX = (screenWidth  - windowWidth) / 2;
        int windowY = (screenHeight - windowHeight) / 2;
        DWORD style = WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_SHOWSELALWAYS;
        if (!g_Settings.ShowColHeader) {
            style |= LVS_NOCOLUMNHEADER;
        }

        // Create Static control for the search string
        int staticTextHeight = 24;

        // Calculate the required height for the static control based on font size
        HDC hdc = GetDC(hWnd);
        g_hFont = CreateFontEx(hdc, g_Settings.FontName, g_Settings.FontSize, g_Settings.FontStyle);
        SelectObject(hdc, g_hFont);

        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        staticTextHeight = (int)(tm.tmHeight + tm.tmExternalLeading + 1);
        ReleaseDC(hWnd, hdc);

        // While creating static search string control, use 0 for height and
        // use -1 for the rest of the calculations while adjusting window size.
        if (!g_Settings.ShowSearchString) {
            staticTextHeight = 0;
        }

        int X      = 0;
        int Y      = 0;
        int width  = windowWidth;
        int height = staticTextHeight;

        //AT_LOG_INFO("StaticTextControl: X: %d, Y: %d, width: %d, height: %d", X, Y, width, height);

        // Create a static text control
        HWND hStaticText = CreateWindowW(
            L"Static",                          // Static text control class
            L"Search String: empty",            // Text content
            WS_CHILD | WS_VISIBLE | SS_CENTER,  // Styles
            X,                                  // X
            Y,                                  // Y
            width,                              // Width
            height,                             // Height
            hWnd,                               // Parent window
            (HMENU)nullptr,                     // Menu or control ID (set to NULL for static text)
            g_hInstance,                        // Instance handle
            nullptr                             // No window creation data
        );

        SendMessage(hStaticText, WM_SETFONT, (WPARAM)g_hFont, 0);
        g_hStaticText = hStaticText;

        // Here adding 1 pixel to the Y position to avoid the static text control overlap with the ListView control
        if (g_Settings.ShowSearchString) {
            Y += staticTextHeight + 1;
        } else {
            staticTextHeight = -1;
        }

        // Here, reducing the window width by 1 (-1) to fit the scrollbar properly in the window.
        // height is reduced by 3 (-3)
        //  1 pixel for the static text control and listView control overlap
        //  2 pixels for the upper and bottom border in listview control
        width  = windowWidth - 1; 
        height = windowHeight - staticTextHeight - 3;
        
        //AT_LOG_INFO("ListViewControl: X: %d, Y: %d, width: %d, height: %d", X, Y, width, height);

        // Create ListView control
        HWND hListView = CreateWindowExW(
            0,                                     // Optional window styles
            WC_LISTVIEW,                           // Predefined class
            L"",                                   // No window title
            style,                                 // Styles
            X,                                     // X
            Y,                                     // Y
            width,                                 // Width
            height,                                // Height
            hWnd,                                  // Parent window
            (HMENU)IDC_LISTVIEW,                   // Control identifier
            g_hInstance,                           // Instance handle
            nullptr                                // No window creation data
        );

        SendMessage(hListView, WM_SETFONT, (WPARAM)g_hFont, MAKELPARAM(TRUE, 0));
        g_hListView = hListView;

        // Subclass the ListView control
        SetWindowSubclass(hListView, ListViewSubclassProc, 1, 0);

        int wndWidth  = (int)(screenWidth  * g_Settings.WidthPercentage  * 0.01);
        int wndHeight = (int)(screenHeight * g_Settings.HeightPercentage * 0.01);

        g_Settings.WindowWidth  = wndWidth;
        g_Settings.WindowHeight = wndHeight;

        // Add header / columns
        CustomizeListView(hListView);

        // Set ListView background and font colors
        SetListViewCustomColors(hListView, g_Settings.BackgroundColor, g_Settings.FontColor);

        // Set window transparency
        SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hWnd, RGB(255, 255, 255), g_Settings.Transparency, LWA_ALPHA);

        //std::vector<AltTabWindowData> altTabWindows;
        EnumWindows(EnumWindowsProc, (LPARAM)(&g_AltTabWindows));

        // Create ImageList and add icons
        HIMAGELIST hImageList =
            ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), ILC_COLOR32 | ILC_MASK, 0, 1);

        for (const auto& item : g_AltTabWindows) {
            ImageList_AddIcon(hImageList, item.hIcon);
        }

        // Set the ImageList for the ListView
        ListView_SetImageList(hListView, hImageList, LVSIL_SMALL);

        // Add windows to ListView
        for (int i = 0; i < g_AltTabWindows.size(); ++i) {
            AddListViewItem(hListView, i, g_AltTabWindows[i]);
        }

        // Compute the required height and resize the ListView
        // Get the header control associated with the ListView
        HWND hHeader = ListView_GetHeader(g_hListView);
        int headerHeight = 0;
        if (hHeader) {
            RECT rcHeader;
            GetClientRect(hHeader, &rcHeader);
            headerHeight = rcHeader.bottom - rcHeader.top;
        }
        RECT rcListView;
        GetClientRect(g_hListView, &rcListView);
        int itemHeight     = ListView_GetItemRect(g_hListView, 0, &rcListView, LVIR_BOUNDS) ? rcListView.bottom - rcListView.top : 0;
        int itemCount      = ListView_GetItemCount(g_hListView);
        int requiredHeight = itemHeight * itemCount + headerHeight + staticTextHeight + 3;

        if (requiredHeight <= g_Settings.WindowHeight) {
            SetWindowPos(hWnd, HWND_TOPMOST, windowX, windowY, windowWidth, requiredHeight, SWP_NOZORDER);
            WindowResizeAndPosition(hWnd, wndWidth, requiredHeight);
        } else {
            int scrollBarWidth   = GetSystemMetrics(SM_CXVSCROLL);
            int processNameWidth = GetColProcessNameWidth();
            int colTitleWidth    = g_Settings.WindowWidth - (COL_ICON_WIDTH + GetColProcessNameWidth()) - scrollBarWidth - 1;
            int lvHeight         = (g_Settings.WindowHeight - itemHeight + 1) / itemHeight * itemHeight;
            int requiredHeight   = lvHeight + headerHeight + staticTextHeight + 3;

            ListView_SetColumnWidth(hListView, 1, colTitleWidth);

            // Here, reducing the window width by 1 (-1) to fit the scrollbar properly in the window.
            SetWindowPos(hListView, nullptr, 0, 0, windowWidth - 1, lvHeight, SWP_NOMOVE | SWP_NOZORDER);
            SetWindowPos(hWnd, HWND_TOPMOST, windowX, windowY, windowWidth, requiredHeight, SWP_NOZORDER);
            WindowResizeAndPosition(hWnd, wndWidth, requiredHeight);
        }

        SetForegroundWindow(hWnd);
        SetFocus(hListView);

        // Select the first row
        LVITEM lvItem;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state     = LVIS_FOCUSED | LVIS_SELECTED;
        SendMessage(hListView, LVM_SETITEMSTATE, 0, (LPARAM)&lvItem);

        // Create a timer to refresh the ListView when there is a change in windows
        SetTimer(hWnd, TIMER_WINDOW_COUNT, TIMER_WINDOW_COUNT_ELAPSE, nullptr);
    }
    break;

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic   = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;

        // Set the text color
        SetTextColor(hdcStatic, RGB(255, 0, 0)); // Red color
    }
    break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            PostQuitMessage(0);
            return (INT_PTR)TRUE;
        }
        break;

    case WM_CONTEXTMENU: {
        POINT pt;
        GetCursorPos(&pt);
        ShowContextMenu(hWnd, pt);
    }
    break;

    case WM_SYSCOMMAND:
        AT_LOG_INFO("WM_SYSCOMMAND");
        // Handle Alt+Space (system menu)
        if (wParam == SC_KEYMENU && (lParam & 0xFFFF) == VK_APPS) {
            // Alt+Space pressed, handle it here
            POINT cursorPos;
            GetCursorPos(&cursorPos);
            TrackPopupMenu(GetSystemMenu(hWnd, FALSE), 0, cursorPos.x, cursorPos.y, 0, hWnd, NULL);
            return 0;
        }
        break;

    case WM_CLOSE:
        AT_LOG_INFO("WM_CLOSE");
        // Release the font
        if (g_hFont != nullptr) {
            DeleteObject(g_hFont);
        }
        PostQuitMessage(0);
        return TRUE;

    case WM_KEYDOWN: {
        AT_LOG_INFO("WM_KEYDOWN");
        if (wParam == VK_ESCAPE) {
            // Close the window when Escape key is pressed
            PostQuitMessage(0);
            return TRUE;
        }
    }
    break;

    case WM_KILLFOCUS:
        AT_LOG_INFO("WM_KILLFOCUS");
        break;

    case WM_NOTIFY: {
        LPNMHDR nmhdr = reinterpret_cast<LPNMHDR>(lParam);
        if (nmhdr->code == LVN_HOTTRACK) {
            LPNMLISTVIEW pnmListView = reinterpret_cast<LPNMLISTVIEW>(nmhdr);

            // Check if the mouse is hovering over an item
            if (pnmListView->iItem >= 0 && (pnmListView->iItem != g_MouseHoverIndex || !g_TooltipVisible)) {
                g_MouseHoverIndex = pnmListView->iItem;

                HideCustomToolTip();

                // The mouse is over an item
                std::wstring info = std::format(
                    L"Path: {}\nPID: {}",
                    g_AltTabWindows[g_MouseHoverIndex].FullPath,
                    g_AltTabWindows[g_MouseHoverIndex].PID);
                ShowCustomToolTip(info);
            }
        }
    } break;

    case WM_MOUSEHOVER:
        AT_LOG_INFO("WM_MOUSEHOVER");
        break;

    //case WM_MOUSELEAVE:
    //    AT_LOG_INFO("WM_MOUSELEAVE");
    //    HideCustomToolTip();
    //    break;

    case WM_TIMER: {
        //AT_LOG_INFO("WM_TIMER");
        std::vector<AltTabWindowData> altTabWindows;
        EnumWindows(EnumWindowsProc, (LPARAM)(&altTabWindows));
        //AT_LOG_INFO("altTabWindows.size(): %d, g_AltTabWindows.size(): %d", altTabWindows.size(), g_AltTabWindows.size());
        if (altTabWindows.size() != g_AltTabWindows.size()) {
            RefreshAltTabWindow();
        }

    }
    break;

    case WM_DESTROY:
        AT_LOG_INFO("WM_DESTROY");
        KillTimer(hWnd, TIMER_WINDOW_COUNT);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return FALSE;
}

bool IsInvisibleWin10BackgroundAppWindow(HWND hWnd) {
    BOOL isCloaked;
    DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(BOOL));
    return isCloaked;
}

/**
 * Check if the given window handle's window is a AltTab window.
 * 
 * \param hWnd Window handle
 * \return True if the given hWnd is a AltTab window otherwise false.
 */
bool IsAltTabWindow(HWND hWnd) {
    if (!IsWindowVisible(hWnd))
        return false;

    HWND hOwner = GetOwnerWindowHwnd(hWnd);

    if (GetLastActivePopup(hOwner) != hWnd)
        return false;

    // Even the owner window is hidden we are getting the window styles, so make
    // sure that the owner window is visible before checking the window styles
    DWORD ownerES = GetWindowLong(hOwner, GWL_EXSTYLE);
    if (ownerES && IsWindowVisible(hOwner) && !((ownerES & WS_EX_TOOLWINDOW) && !(ownerES & WS_EX_APPWINDOW))
        && !IsInvisibleWin10BackgroundAppWindow(hOwner)) {
        return true;
    }

    DWORD windowES = GetWindowLong(hWnd, GWL_EXSTYLE);
    if (windowES && !((windowES & WS_EX_TOOLWINDOW) && !(windowES & WS_EX_APPWINDOW))
        && !IsInvisibleWin10BackgroundAppWindow(hWnd)) {
        return true;
    }

    if (windowES == 0 && ownerES == 0) {
        return true;
    }

    return false;
}

/**
 * Get owner window handle for the given hWnd.
 * 
 * \param hWnd Window handle
 * \return The owner window handle for the given hWnd.
 */
HWND GetOwnerWindowHwnd(HWND hWnd) {
    HWND hOwner = hWnd;
    do {
        hOwner = GetWindow(hOwner, GW_OWNER);
    } while (GetWindow(hOwner, GW_OWNER));
    hOwner = hOwner ? hOwner : hWnd;
    return hOwner;
}

/*!
 * \brief Show AltTab window's context menu at the center of the selected item.
 */
void ShowContextMenuAtItemCenter()
{
    // Get the position of the selected item
    // Get the bounding rectangle of the selected item
    RECT itemRect;
    ListView_GetItemRect(g_hListView, g_SelectedIndex, &itemRect, LVIR_BOUNDS);

    // Calculate the center of the entire row
    POINT center;
    center.x = (itemRect.left + itemRect.right) / 2;
    center.y = (itemRect.top + itemRect.bottom) / 2;

    // Convert to screen coordinates if needed
    ClientToScreen(g_hListView, &center);

    ShowContextMenu(g_hAltTabWnd, center);
}

// ----------------------------------------------------------------------------
// Show AltTab context menu
// ----------------------------------------------------------------------------
void ShowContextMenu(HWND hWnd, POINT pt) {
    HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_CONTEXTMENU));
    if (hMenu) {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu) {
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

            ContextMenuItemHandler(hWnd, hSubMenu, menuItemId);
        }
        DestroyMenu(hMenu);
    }
}

void SetAltTabActiveWindow() {
    SetForegroundWindow(g_hAltTabWnd);
    SetActiveWindow(g_hAltTabWnd);
    SetFocus(g_hListView);
}

void CopyToClipboard(const std::wstring& text) {
    if (OpenClipboard(g_hAltTabWnd)) {
        EmptyClipboard();
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
        if (hGlobal) {
            wchar_t* pGlobal = (wchar_t*)GlobalLock(hGlobal);
            if (pGlobal) {
                wcscpy_s(pGlobal, text.size() + 1, text.c_str());
                GlobalUnlock(hGlobal);
                SetClipboardData(CF_UNICODETEXT, hGlobal);
            }
        }
        CloseClipboard();
    } else {
        AT_LOG_ERROR("OpenClipboard failed");
    }
}

void BrowseToFile(const std::wstring& filepath) {
    if (!filepath.empty()) {
        LPITEMIDLIST pidl = ILCreateFromPathW(filepath.c_str());
        if (pidl) {
            SHOpenFolderAndSelectItems(pidl, 0, nullptr, 0);
            ILFree(pidl);
        }
    }
}

// ----------------------------------------------------------------------------
// AltTab window context menu handler
// ----------------------------------------------------------------------------
void ContextMenuItemHandler(HWND hWnd, HMENU hSubMenu, UINT menuItemId) {
    switch (menuItemId) {
    case ID_CONTEXTMENU_CLOSE_WINDOW: {
        // Send the SC_CLOSE command to the window
        int ind = ATWListViewGetSelectedItem();
        if (ind != -1) {
            PostMessage(g_AltTabWindows[ind].hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        }
    }
    break;

    case ID_CONTEXTMENU_KILL_PROCESS: {
        int ind = ATWListViewGetSelectedItem();
        if (ind != -1) {
            TerminateProcessEx(g_AltTabWindows[ind].PID);
        }
    }
    break;

    case ID_CONTEXTMENU_CLOSEALLWINDOWS: {
        AT_LOG_INFO("ID_CONTEXTMENU_CLOSEALLWINDOWS");

        // Here, this MessageBox is also displayed in AltTab windows list. Did
        // not find a way to avoid this. So, turning off the timer.
        KillTimer(hWnd, TIMER_WINDOW_COUNT);

        bool closeAllWindows = true;
        if (g_Settings.PromptTerminateAll) {
            int result = MessageBoxW(
                hWnd,
                L"Are you sure you want to close all windows?",
                AT_PRODUCT_NAMEW L": Close All Windows",
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
            closeAllWindows = (result == IDYES);
        }

        if (closeAllWindows) {
            for (auto& g_AltTabWindow : g_AltTabWindows) {
                PostMessage(g_AltTabWindow.hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
            }
        } else {
            SetAltTabActiveWindow();
        }

        SetTimer(hWnd, TIMER_WINDOW_COUNT, TIMER_WINDOW_COUNT_ELAPSE, nullptr);
    }
    break;

    case ID_CONTEXTMENU_KILLALLPROCESSES: {
        AT_LOG_INFO("ID_CONTEXTMENU_KILLALLPROCESSES");

        // Here, this MessageBox is also displayed in AltTab windows list. Did
        // not find a way to avoid this. So, turning off the timer.
        KillTimer(hWnd, TIMER_WINDOW_COUNT);

        bool terminateAllWindows = true;
        if (g_Settings.PromptTerminateAll) {
            int result = MessageBoxW(
                hWnd,
                L"Are you sure you want to terminate all windows?",
                AT_PRODUCT_NAMEW L": Close All Windows",
                MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
            terminateAllWindows = (result == IDYES);
        }
        if (terminateAllWindows) {
            for (auto& g_AltTabWindow : g_AltTabWindows) {
                TerminateProcessEx(g_AltTabWindow.PID);
            }
        } else {
            SetAltTabActiveWindow();
        }

        SetTimer(hWnd, TIMER_WINDOW_COUNT, TIMER_WINDOW_COUNT_ELAPSE, nullptr);
    }
    break;

    case ID_CONTEXTMENU_OPEN_PATH: {
        AT_LOG_INFO("ID_CONTEXTMENU_OPEN_PATH");
        const auto filepath = g_AltTabWindows[ATWListViewGetSelectedItem()].FullPath;
        DestoryAltTabWindow();
        BrowseToFile(filepath);
    } break;
    
    case ID_CONTEXTMENU_COPY_PATH: {
        AT_LOG_INFO("ID_CONTEXTMENU_COPY_PATH");
        const auto filepath = g_AltTabWindows[ATWListViewGetSelectedItem()].FullPath;
        DestoryAltTabWindow();
        CopyToClipboard(filepath);
    } break;

    case ID_CONTEXTMENU_ABOUTALTTAB: {
        AT_LOG_INFO("ID_CONTEXTMENU_ABOUTALTTAB");
        DestoryAltTabWindow();
        DialogBoxW(g_hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), g_hMainWnd, ATAboutDlgProc);
    }
    break;

    case ID_CONTEXTMENU_SETTINGS: {
        AT_LOG_INFO("ID_CONTEXTMENU_SETTINGS");
        DestoryAltTabWindow();
        DialogBoxW(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS), g_hMainWnd, ATSettingsDlgProc);
    }
    break;

    case ID_CONTEXTMENU_EXIT:
        AT_LOG_INFO("ID_CONTEXTMENU_EXIT");
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

/*!
 * Terminate the given process id
 * 
 * \param pid  ProcessID
 * \return True if terminated successfully otherwise false.
 */
BOOL TerminateProcessEx(DWORD pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess) {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
        return TRUE;
    }
    return FALSE;
}

bool ATMapVirtualKey(UINT uCode, wchar_t& vkCode) {
    wchar_t ch = static_cast<wchar_t>(uCode);
    //AT_LOG_INFO("uCode: %c, ch: %c", uCode, ch);

    if (!iswprint(uCode)) {
        vkCode = '\0';
        return false;
    }

    bool isShiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    // Check for alphabetic and digit keys
    if ((uCode >= 'A' && uCode <= 'Z') || (uCode >= '0' && uCode <= '9')) {
        ch = MapVirtualKey(uCode, MAPVK_VK_TO_CHAR);

        // Adjust the case based on the Shift key state for alphabets
        if (!isShiftPressed && ch >= L'A' && ch <= L'Z') {
            ch = ch - L'A' + L'a';
        }

        if (isShiftPressed && ch >= L'0' && ch <= L'9') {
            ch = L")!@#$%^&*("[ch - L'0'];
        }

        vkCode = ch;
        return true;
    }

    // Handle other special characters
    switch (uCode) {
        case VK_SPACE:        vkCode = L' ';                          return true;
        case VK_OEM_MINUS:    vkCode = isShiftPressed ? L'_' : L'-';  return true;
        case VK_OEM_PLUS:     vkCode = isShiftPressed ? L'=' : L'+';  return true;
        case VK_OEM_1:        vkCode = isShiftPressed ? L':' : L';';  return true;
        case VK_OEM_2:        vkCode = isShiftPressed ? L'?' : L'/';  return true;
        case VK_OEM_3:        vkCode = isShiftPressed ? L'~' : L'`';  return true;
        case VK_OEM_4:        vkCode = isShiftPressed ? L'{' : L'[';  return true;
        case VK_OEM_5:        vkCode = isShiftPressed ? L'|' : L'\\'; return true;
        case VK_OEM_6:        vkCode = isShiftPressed ? L'}' : L']';  return true;
        case VK_OEM_7:        vkCode = isShiftPressed ? L'"' : L'\''; return true;
        case VK_OEM_COMMA:    vkCode = isShiftPressed ? L'<' : L',';  return true;
        case VK_OEM_PERIOD:   vkCode = isShiftPressed ? L'>' : L'.';  return true;
        case VK_OEM_102:      vkCode = isShiftPressed ? L'>' : L'<';  return true;
    }
    return false;
}

/*!
 * Check if the process name is in the exclusion list
 * 
 * \param processName Process name, should be in lower case
 * \return True if the processName is in the exclusion list otherwise false
 */
bool IsExcludedProcess(const std::wstring& processName) {
    bool excluded = false;
    if (g_Settings.ProcessExclusionsEnabled) {
        excluded = std::find(
           g_Settings.ProcessExclusionList.begin(),
           g_Settings.ProcessExclusionList.end(),
           processName) != g_Settings.ProcessExclusionList.end();
    }
    return excluded;
}
