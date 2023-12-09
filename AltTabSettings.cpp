#include "AltTabSettings.h"
#include "Resource.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include "Utils.h"

#define WM_SETTEXTCOLOR (WM_USER + 1)

AltTabSettings g_Settings = { 
    L"Lucida Handwriting",    // FontName
    45,                       // WidthPercentage
    45,                       // HeightPercentage
    0,                        // WindowWidth
    0,                        // WindowHeight
    RGB(0xFF, 0xFF, 0xFF),    // FontColor
    RGB(0x00, 0x00, 0x00),    // BackgroundColor
    222,                      // WindowTransparency
    L"Code.exe/notepad.exe/notepad++.exe|iexplore.exe/chrome.exe/firefox.exe|explorer.exe/xplorer2_lite.exe/xplorer2.exe/xplorer2_64.exe|cmd.exe/conemu.exe/conemu64.exe",
    {},                       // ProcessGroupsList
    L"Startup",               // CheckForUpdates
    true                      // PromptTerminateAll
};

INT_PTR CALLBACK ATSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: {
        SetDlgItemText    (hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS  , g_Settings.SimilarProcessGroups.c_str());

        HFONT hFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Lucida Console");
        COLORREF fontColor = RGB(0, 0, 255); // Change RGB values as needed
        HWND hEditBox = GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS);
        SendMessage(hEditBox, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hEditBox, WM_SETTEXTCOLOR, (WPARAM)fontColor, 0);

        CheckDlgButton    (hDlg, IDC_CHECK_PROMPT_TERMINATE_ALL   , g_Settings.PromptTerminateAll? BST_CHECKED : BST_UNCHECKED);

        SetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_TRANSPARENCY     , g_Settings.Transparency     , FALSE);
        SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_TRANSPARENCY     , UDM_SETRANGE                , 0, MAKELPARAM(255, 0));
        SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_TRANSPARENCY     , UDM_SETPOS                  , 0, MAKELPARAM(g_Settings.Transparency, 0));

        SetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_WIDTH_PERCENTAGE , g_Settings.WidthPercentage  , FALSE);
        SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_WIDTH_PERCENTAGE , UDM_SETRANGE                , 0, MAKELPARAM(90, 10));
        SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_WIDTH_PERCENTAGE , UDM_SETPOS                  , 0, MAKELPARAM(g_Settings.WidthPercentage, 0));

        SetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_HEIGHT_PERCENTAGE, g_Settings.HeightPercentage , FALSE);
        SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_HEIGHT_PERCENTAGE, UDM_SETRANGE                , 0, MAKELPARAM(90, 10));
        SendDlgItemMessage(hDlg, IDC_SPIN_WINDOW_HEIGHT_PERCENTAGE, UDM_SETPOS                  , 0, MAKELPARAM(g_Settings.HeightPercentage, 0));

        HWND hComboBox = GetDlgItem(hDlg, IDC_CHECK_FOR_UPDATES);
        ComboBox_AddString(hComboBox, L"Startup");
        ComboBox_AddString(hComboBox, L"Daily");
        ComboBox_AddString(hComboBox, L"Weekly");
        ComboBox_AddString(hComboBox, L"Never");
        ComboBox_SetCurSel(hComboBox, 0);

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
    }
    return (INT_PTR)TRUE;

    case WM_CTLCOLOREDIT:
    {
        HDC  hdcEdit = (HDC)wParam;
        HWND hEdit   = (HWND)lParam;
        if (GetDlgCtrlID(hEdit) == IDC_EDIT_SIMILAR_PROCESS_GROUPS) {
            SetTextColor(hdcEdit, RGB(0, 0, 255));   // Blue text color
        }
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    }
    break;

    //case WM_CTLCOLORSTATIC:
    //{
    //    HDC  hdcEdit = (HDC)wParam;
    //    HWND hEdit   = (HWND)lParam;
    //    SetTextColor(hdcEdit, RGB(255, 0, 255));
    //    return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
    //}
    //break;

    //case WM_CTLCOLORSTATIC: {
    //    // Handle WM_CTLCOLORSTATIC to customize the text color of the group box
    //    HDC hdcStatic = (HDC)wParam;
    //    HWND hStatic = (HWND)lParam;

    //    // Check if the control is a group box (BS_GROUPBOX style)
    //    if (GetWindowLong(hStatic, GWL_STYLE) & BS_GROUPBOX) {
    //        // Set the text color for the group box
    //        SetTextColor(hdcStatic, RGB(255, 0, 0));           // Red text color
    //        SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE)); // Use the default system background color

    //        // Return a handle to the brush for the background
    //        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    //    }
    //} break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK)
        {
            int textLength                  = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS));
            wchar_t* buffer                 = new wchar_t[textLength + 1];
            GetDlgItemTextW(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS, buffer, textLength + 1);

            g_Settings.SimilarProcessGroups = buffer;
            g_Settings.PromptTerminateAll   = IsDlgButtonChecked(hDlg, IDC_CHECK_PROMPT_TERMINATE_ALL) == BST_CHECKED;
            g_Settings.Transparency         = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_TRANSPARENCY     , nullptr, FALSE);
            g_Settings.WidthPercentage      = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_WIDTH_PERCENTAGE , nullptr, FALSE);
            g_Settings.HeightPercentage     = GetDlgItemInt(hDlg, IDC_EDIT_WINDOW_HEIGHT_PERCENTAGE, nullptr, FALSE);

            delete[] buffer;
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        break;

    case WM_DESTROY: {
        HWND hEditBox = GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS);
        HFONT hFont = (HFONT)SendMessage(hEditBox, WM_GETFONT, 0, 0);
        if (hFont) {
            DeleteObject(hFont);
        }
    }
    break;

    }
    return (INT_PTR)FALSE;
}

int GetProcessGroupIndex(const std::wstring& processName) {
    const std::wstring& processNameL = ToLower(processName);
    for (size_t ind = 0; ind < g_Settings.ProcessGroupsList.size(); ++ind) {
        if (g_Settings.ProcessGroupsList[ind].contains(processNameL)) {
            return (int)ind;
        }
    }
    return -1;
}

bool IsSimilarProcess(int index, const std::wstring& processName) {
    if (index == -1)
        return false;
    return g_Settings.ProcessGroupsList[index].contains(ToLower(processName));
}

bool IsSimilarProcess(const std::wstring& processNameA, const std::wstring& processNameB) {
    int index = GetProcessGroupIndex(processNameA);
    if (index == -1)
        return false;
    return g_Settings.ProcessGroupsList[index].contains(ToLower(processNameB));
}
