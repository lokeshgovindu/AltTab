// AltTabWindow.cpp : Defines the entry point for the application.
//

#include "AltTabWindow.h"

#include "Logger.h"
#include "framework.h"

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

#pragma comment(lib, "comctl32.lib")

#pragma comment(                                                                                                       \
        linker,                                                                                                        \
            "/manifestdependency:\"type='win32' "                                                                      \
            "name='Microsoft.Windows.Common-Controls' "                                                                \
            "version='6.0.0.0' "                                                                                       \
            "processorArchitecture='*' "                                                                               \
            "publicKeyToken='6595b64144ccf1df' "                                                                       \
            "language='*' "                                                                                            \
            "\"")

HWND    g_hListView = nullptr;
HFONT   g_hFont     = nullptr;

const int COL_ICON_WIDTH     = 36;
const int COL_PROCNAME_WIDTH = 180;

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK ATAboutDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AltTabWindowProc(HWND, UINT, WPARAM, LPARAM);
bool             IsAltTabWindow(HWND hWnd);
static HWND      CreateAltTabWindow();

std::vector<AltTabWindowData> g_AltTabWindows;

HICON GetWindowIcon(HWND hWnd) {
    // Try to get the large icon
    HICON hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_BIG, 0);
    if (hIcon)
        return hIcon;

    // If the large icon is not available, try to get the small icon
    hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL2, 0);
    if (hIcon)
        return hIcon;

    hIcon = (HICON)SendMessage(hWnd, WM_GETICON, ICON_SMALL, 0);
    if (hIcon)
        return hIcon;

    // Get the class long value that contains the icon handle
    hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
    if (!hIcon) {
        // If the class does not have an icon, try to get the small icon
        hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICONSM);
        if (!hIcon) {
            hIcon = LoadIcon(nullptr, IDI_APPLICATION);
            return hIcon;
        }
    }
    return hIcon;
}

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    if (IsAltTabWindow(hWnd)) {
        DWORD processId;
        GetWindowThreadProcessId(hWnd, &processId);

        if (processId != 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);

            if (hProcess != nullptr) {
                TCHAR szProcessPath[MAX_PATH];
                if (GetModuleFileNameEx(hProcess, nullptr, szProcessPath, MAX_PATH)) {
                    std::filesystem::path filePath = szProcessPath;

                    const int bufferSize = 256; // adjust the buffer size as needed
                    TCHAR windowTitle[bufferSize];
                    int length = GetWindowText(hWnd, windowTitle, bufferSize);
                    if (length == 0)
                        return TRUE;

                    AltTabWindowData item = { hWnd, GetWindowIcon(hWnd), windowTitle, filePath.filename().wstring() };
                    auto vItems = (std::vector<AltTabWindowData>*)lParam;
                    bool insert = false;

                    if (vItems->empty() || g_IsAltTab) {
                        insert = true;
                    }
                    else if (g_IsAltBacktick) {
                        insert = IsSimilarProcess(vItems->at(0).ProcessName, item.ProcessName);
                    }

                    if (insert) { vItems->push_back(std::move(item)); }
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

HWND ShowAltTabWindow(HWND& hAltTabWnd, int direction) {
    if (hAltTabWnd == nullptr) {
        hAltTabWnd = CreateAltTabWindow();
    }

    // Move to next / previous item based on the direction
    int selectedInd = (int)SendMessageW(g_hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
    int N           = (int)g_AltTabWindows.size();
    int nextInd     = (selectedInd + N + direction) % N;

    ATWListViewSelectItem(nextInd);

    return hAltTabWnd;
}

void ATWListViewSelectItem(int rowNumber) {
    // Move to next / previous item based on the direction
    int selectedRow = (int)SendMessageW(g_hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

    LVITEM lvItem;
    lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.state = 0;
    SendMessageW(g_hListView, LVM_SETITEMSTATE, selectedRow, (LPARAM)&lvItem);

    rowNumber = max(0, min(rowNumber, (int)g_AltTabWindows.size() - 1));

    lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    SendMessageW(g_hListView, LVM_SETITEMSTATE  , rowNumber, (LPARAM)&lvItem);
    SendMessageW(g_hListView, LVM_ENSUREVISIBLE , rowNumber, (LPARAM)&lvItem);
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
    const wchar_t CLASS_NAME[] = L"AltTab";

    WNDCLASS wc      = {};
    wc.lpfnWndProc   = AltTabWindowProc;
    wc.lpszClassName = CLASS_NAME;
    wc.hInstance     = g_hInstance;

    RegisterClass(&wc);

    DWORD exStyle = WS_EX_TOOLWINDOW | WS_EX_TOPMOST;
    DWORD style   = WS_POPUP | WS_VISIBLE | WS_BORDER;

    // Create the window
    HWND hWnd = CreateWindowExW(
        exStyle,     // Optional window styles
        CLASS_NAME,  // Window class
        CLASS_NAME,  // Window title
        style,       // Styles
        100,         // X
        100,         // Y
        900,         // Width
        700,         // Height
        nullptr,     // Parent window
        nullptr,     // Menu
        g_hInstance, // Instance handle
        nullptr      // Additional application data
    );

    if (hWnd == nullptr) {
        AT_LOG_ERROR("Failed to create AltTab Window!");
        return nullptr;
    }

    // Show the window
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    SetForegroundWindow(hWnd);

    return hWnd;
}

static void AddListViewItem(HWND hListView, int index, const AltTabWindowData& windowData) {
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

    const int colTitleWidth = g_Settings.WindowWidth - COL_ICON_WIDTH - COL_PROCNAME_WIDTH;

    // Add columns to the List View
    LVCOLUMN lvCol   = {0};
    lvCol.mask       = LVCF_TEXT | LVCF_WIDTH;
    lvCol.pszText    = (LPWSTR)L"#";
    lvCol.cx         = COL_ICON_WIDTH;
    ListView_InsertColumn(hListView, 0, &lvCol);

    lvCol.pszText    = (LPWSTR)L"Window Title";
    lvCol.cx         = colTitleWidth;
    ListView_InsertColumn(hListView, 1, &lvCol);

    lvCol.pszText    = (LPWSTR)L"Process Name";
    lvCol.cx         = COL_PROCNAME_WIDTH;
    ListView_InsertColumn(hListView, 2, &lvCol);
}

static void SetCustomFont(HWND hListView, int fontSize) {
    LOGFONT lf;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"Lucida Handwriting");

    // Modify the font size
    lf.lfHeight = -MulDiv(fontSize, GetDeviceCaps(GetDC(hListView), LOGPIXELSY), 72);

    g_hFont = CreateFontIndirect(&lf);

    SendMessage(hListView, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFont), MAKELPARAM(TRUE, 0));
}

static void SetCustomColors(HWND hListView, COLORREF backgroundColor, COLORREF textColor) {
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
    //AT_LOG_TRACE;
    //AT_LOG_INFO(std::format("uMsg: {:4}, wParam: {}, lParam: {}", uMsg, wParam, lParam).c_str());
    switch (uMsg) {
    case WM_KEYDOWN:
        AT_LOG_INFO("WM_KEYDOWN");

        if (wParam == VK_ESCAPE) {
            AT_LOG_INFO("VK_ESCAPE");
            PostQuitMessage(0);
        } else if (wParam == VK_DOWN) {
            AT_LOG_INFO("VK_DOWN");

            int selectedRow = (int)SendMessageW(hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
            int N = (int)g_AltTabWindows.size();

            LVITEM lvItem;
            lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
            if (selectedRow == N - 1) {
                lvItem.state = 0;
                SendMessageW(hListView, LVM_SETITEMSTATE, selectedRow, (LPARAM)&lvItem);

                lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
                SendMessageW(hListView, LVM_SETITEMSTATE, 0, (LPARAM)&lvItem);
                return 0;
            }
        } else if (wParam == VK_UP) {
            AT_LOG_INFO("VK_UP");

            int selectedRow = (int)SendMessageW(hListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
            int N = (int)g_AltTabWindows.size();

            LVITEM lvItem;
            lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
            if (selectedRow == 0) {
                lvItem.state = 0;
                SendMessage(hListView, LVM_SETITEMSTATE, selectedRow, (LPARAM)&lvItem);

                lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
                SendMessage(hListView, LVM_SETITEMSTATE, N - 1, (LPARAM)&lvItem);
                return 0;
            }
        }
        break;

    case WM_KILLFOCUS:
        AT_LOG_INFO("WM_KILLFOCUS");

        // Handle WM_KILLFOCUS to prevent losing selection when the window loses focus
        LVITEM lvItem;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
        SendMessage(hListView, LVM_SETITEMSTATE, -1, (LPARAM)&lvItem);
        break;
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
    SetWindowPos(hWnd, HWND_TOP, xPos, yPos, wndWidth, wndHeight, SWP_NOSIZE);
}

INT_PTR CALLBACK AltTabWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    //AT_LOG_TRACE;
    //AT_LOG_INFO(std::format("uMsg: {:4}, wParam: {}, lParam: {}", uMsg, wParam, lParam).c_str());

    switch (uMsg) {
    case WM_CREATE: {
        // Get screen width and height
        int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        // Compute the window size (e.g., 80% of the screen width and height)
        int windowWidth  = static_cast<int>(screenWidth  * g_Settings.WidthPercentage  * 0.01);
        int windowHeight = static_cast<int>(screenHeight * g_Settings.HeightPercentage * 0.01);

        // Compute the window position (centered on the screen)
        int windowX = (screenWidth  - windowWidth) / 2;
        int windowY = (screenHeight - windowHeight) / 2;

        // Create ListView control
        HWND hListView = CreateWindowExW(
            0,                                  // Optional window styles
            WC_LISTVIEW,                        // Predefined class
            L"",                                // No window title
            WS_VISIBLE | WS_CHILD | LVS_REPORT, // Styles
            0,
            0,
            windowWidth,
            windowHeight,                       // Position and size
            hWnd,                               // Parent window
            (HMENU)IDC_LISTVIEW,                // Control identifier
            GetModuleHandle(nullptr),           // Instance handle
            nullptr                             // No window creation data
        );

        g_hListView = hListView;

        // Subclass the ListView control
        //SetWindowSubclass(hListView, ListViewSubclassProc, 1, 0);

        int wndWidth  = (int)(screenWidth  * g_Settings.WidthPercentage  * 0.01);
        int wndHeight = (int)(screenHeight * g_Settings.HeightPercentage * 0.01);

        g_Settings.WindowWidth  = wndWidth;
        g_Settings.WindowHeight = wndHeight;

        CustomizeListView(hListView);
        SetCustomFont    (hListView, 11);
        SetCustomColors  (hListView, g_Settings.BackgroundColor, g_Settings.FontColor);

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
        int itemHeight =
            ListView_GetItemRect(g_hListView, 0, &rcListView, LVIR_BOUNDS) ? rcListView.bottom - rcListView.top : 0;
        int itemCount = ListView_GetItemCount(g_hListView);
        int requiredHeight = itemHeight * itemCount + headerHeight;

        if (requiredHeight <= g_Settings.WindowHeight) {
            SetWindowPos(hWnd, nullptr, windowX, windowY, windowWidth, requiredHeight, SWP_NOZORDER);
            WindowResizeAndPosition(hWnd, wndWidth, requiredHeight);
        } else {
            int scrollBarWidth = GetSystemMetrics(SM_CXVSCROLL);
            int colTitleWidth = g_Settings.WindowWidth - (COL_ICON_WIDTH + COL_PROCNAME_WIDTH) - scrollBarWidth;
            ListView_SetColumnWidth(hListView, 1, colTitleWidth);
            SetWindowPos(hWnd, nullptr, windowX, windowY, windowWidth, windowHeight, SWP_NOZORDER);
            WindowResizeAndPosition(hWnd, wndWidth, g_Settings.WindowHeight);
        }

        SetFocus(hListView);

        // Select the first row
        LVITEM lvItem;
        lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
        lvItem.state     = LVIS_FOCUSED | LVIS_SELECTED;
        SendMessage(hListView, LVM_SETITEMSTATE, 0, (LPARAM)&lvItem);

        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            PostQuitMessage(0);
            return (INT_PTR)TRUE;
        }
        break;

    case WM_CLOSE:
        // Release the font
        if (g_hFont != nullptr) {
            DeleteObject(g_hFont);
        }
        PostQuitMessage(0);
        return TRUE;

    case WM_KEYDOWN:
        AT_LOG_INFO("WM_KEYDOWN");
        if (wParam == VK_ESCAPE) {
            // Close the window when Escape key is pressed
            PostQuitMessage(0);
            return TRUE;
        }
        break;

    case WM_KILLFOCUS:
        AT_LOG_INFO("WM_KILLFOCUS");
        break;

    //case WM_NOTIFY:
    //    AT_LOG_INFO("WM_NOTIFY");
    //    break;

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

bool IsAltTabWindow(HWND hWnd) {
    //return sws_WindowHelpers_IsAltTabWindow(hWnd);
    if (!IsWindowVisible(hWnd))
        return false;

    HWND hOwner = hWnd;

    do {
        hOwner = GetWindow(hOwner, GW_OWNER);
    } while (GetWindow(hOwner, GW_OWNER));

    hOwner = hOwner ? hOwner : hWnd;

    if (GetLastActivePopup(hOwner) != hWnd)
        return false;

    DWORD windowES = GetWindowLong(hWnd, GWL_EXSTYLE);
    if (windowES && !((windowES & WS_EX_TOOLWINDOW) && !(windowES & WS_EX_APPWINDOW)) &&
        !IsInvisibleWin10BackgroundAppWindow(hOwner)) {
        return true;
    }

    DWORD ownerES = GetWindowLong(hOwner, GWL_EXSTYLE);
    if (windowES == 0 && ownerES == 0) {
        return true;
    }

    return false;
}

std::vector<AltTabWindowData> GetAltTabWindowsList() {
    EnumWindows(EnumWindowsProc, 0);
    return {};
}
