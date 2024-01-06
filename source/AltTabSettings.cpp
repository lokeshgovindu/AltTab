#include "AltTabSettings.h"
#include "Resource.h"
#include <windowsx.h>
#include <CommCtrl.h>
#include "Utils.h"
#include "Logger.h"
#include <ShlObj_core.h>
#include "version.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "GlobalData.h"
#include "AltTabWindow.h"

#define WM_SETTEXTCOLOR (WM_USER + 1)

// ----------------------------------------------------------------------------
// Default settings
// ----------------------------------------------------------------------------
AltTabSettings g_Settings = { 
    DEFAULT_FONT_NAME,              // FontName
    DEFAULT_WIDTH,                  // WidthPercentage
    DEFAULT_HEIGHT,                 // HeightPercentage
    0,                              // WindowWidth
    0,                              // WindowHeight
    DEFAULT_FONT_COLOR,             // FontColor
    DEFAULT_BG_COLOR,               // BackgroundColor
    DEFAULT_TRANSPARENCY,           // WindowTransparency
    DEFAULT_SIMILARPROCESSGROUPS,   // SimilarProcessGroups
    {},                             // ProcessGroupsList
    DEFAULT_CHECKFORUPDATES,        // CheckForUpdates
    DEFAULT_PROMPTTERMINATEALL,     // PromptTerminateAll
    false,                          // DisableAltTab
    DEFAULT_SHOW_COL_HEADER,        // ShowColHeader
    DEFAULT_SHOW_COL_PROCESSNAME,   // ShowColProcessName
};

AltTabWindowData g_AltBacktickWndInfo;

// ----------------------------------------------------------------------------
// Settings dialog procedure
// ----------------------------------------------------------------------------
INT_PTR CALLBACK ATSettingsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG: {
        SetDlgItemText    (hDlg, IDC_EDIT_SETTINGS_FILEPATH       , ATSettingsFilePath().c_str());
        SetDlgItemText    (hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS  , g_Settings.SimilarProcessGroups.c_str());

        HFONT    hFont     = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Lucida Console");
        COLORREF fontColor = RGB(0, 0, 255); // Change RGB values as needed
        HWND     hEditBox  = GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS);

        SendMessage(hEditBox, WM_SETFONT     , (WPARAM)hFont    , TRUE);
        SendMessage(hEditBox, WM_SETTEXTCOLOR, (WPARAM)fontColor, FALSE);

        CheckDlgButton    (hDlg, IDC_CHECK_PROMPT_TERMINATE_ALL   , g_Settings.PromptTerminateAll ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton    (hDlg, IDC_CHECK_SHOW_COL_HEADER        , g_Settings.ShowColHeader      ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton    (hDlg, IDC_CHECK_SHOW_COL_PROCESSNAME   , g_Settings.ShowColProcessName ? BST_CHECKED : BST_UNCHECKED);

        EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SHOW_PREVIEW)     , FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_EDIT_PROCESS_EXCLUSIONS), FALSE);

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

        SetWindowPos(hDlg, HWND_TOPMOST, posX, posY, 0, 0, SWP_NOSIZE);
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
        {
            bool settingsModified = AreSettingsModified(hDlg);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY), settingsModified);
        }

        if (LOWORD(wParam) == IDC_BUTTON_APPLY) {
            AT_LOG_INFO("IDC_BUTTON_APPLY: Apply Settings");
            ATApplySettings(hDlg);
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDOK)
        {
            ATApplySettings(hDlg);
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDCANCEL)
        {
            AT_LOG_INFO("IDCANEL: Cancel Pressed!");
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        if (LOWORD(wParam) == IDC_BUTTON_RESET)
        {
            AT_LOG_INFO("IDC_BUTTON_RESET Pressed!");
            return (INT_PTR)TRUE;
        }

        break;

    case WM_DESTROY: {
        HWND  hEditBox = GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS);
        HFONT hFont    = (HFONT)SendMessage(hEditBox, WM_GETFONT, 0, 0);
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
    if (EqualsIgnoreCase(processNameA, processNameB))
        return true;

    int index = GetProcessGroupIndex(processNameA);
    if (index == -1)
        return false;

    return g_Settings.ProcessGroupsList[index].contains(ToLower(processNameB));
}

std::wstring ATSettingsDirPath() {
    // Get the path to the Roaming AppData folder
    wchar_t szPath[MAX_PATH] = { 0 };
    SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, szPath);
    std::filesystem::path settingsDirPath = szPath;
    settingsDirPath.append(AT_PRODUCT_NAMEW);
    if (!std::filesystem::exists(settingsDirPath)) {
        if (!std::filesystem::create_directory(settingsDirPath)) {
            throw std::exception("Failed to create AltTab directory in APPDATA");
        }
    }
    return settingsDirPath.wstring();
}

// ----------------------------------------------------------------------------
// AltTab settings file path
// ----------------------------------------------------------------------------
std::wstring ATSettingsFilePath() {
    std::filesystem::path settingsFilePath = ATSettingsDirPath();
    settingsFilePath.append(SETTINGS_INI_FILENAME);
    if (!std::filesystem::exists(settingsFilePath)) {
        std::ofstream fs(settingsFilePath);
        if (!fs.is_open()) {
            throw std::exception("Failed to create AltTab.ini file in APPDATA/AltTab");
        }
        fs << "; -----------------------------------------------------------------------------" << std::endl;
        fs << "; Configuration/settings file for AltTab." << std::endl;
        fs << "; Notes:" << std::endl;
        fs << ";   1. Do NOT edit manually if you are not familiar with settings." << std::endl;
        fs << ";   2. Color Format is RGB(0xAA, 0xBB, 0xCC) => 0xAABBCC, in hex format." << std::endl;
        fs << ";      0xAA : Red component" << std::endl;
        fs << ";      0xBB : Green component" << std::endl;
        fs << ";      0xCC : Blue component" << std::endl;
        fs << "; -----------------------------------------------------------------------------" << std::endl;
        fs.close();
        ATSettingsCreateDefault(settingsFilePath.wstring());
    }
    return settingsFilePath.wstring();
}

template<typename T>
void WriteSetting(LPCTSTR iniFile, LPCTSTR section, LPCTSTR keyName, const T& value) {
    WritePrivateProfileStringW(section, keyName, std::to_wstring(value).c_str(), iniFile);
}

template<>
void WriteSetting(LPCTSTR iniFile, LPCTSTR section, LPCTSTR keyName, const std::wstring& value) {
    WritePrivateProfileStringW(section, keyName, value.c_str(), iniFile);
}

void ATSettingsCreateDefault(const std::wstring& settingsFilePath) {
    WriteSetting(settingsFilePath.c_str(), L"Backtick", L"SimilarProcessGroups"  , g_Settings.SimilarProcessGroups);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"PromptTerminateAll"    , g_Settings.PromptTerminateAll);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"WindowTransparency"    , g_Settings.Transparency);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"WindowWidthPercentage" , g_Settings.WidthPercentage);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"WindowHeightPercentage", g_Settings.HeightPercentage);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"CheckForUpdates"       , g_Settings.CheckForUpdates);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"ShowColHeader"         , g_Settings.ShowColHeader);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"ShowColProcessName"    , g_Settings.ShowColProcessName);
}

void ATLoadSettings() {
    AT_LOG_TRACE;
    std::wstring iniFile = ATSettingsFilePath();

    const int bufferSize = 4096;    // Initial buffer size
    wchar_t buffer[bufferSize];     // Buffer to store the retrieved string

    GetPrivateProfileStringW(L"Backtick", L"SimilarProcessGroups", g_Settings.SimilarProcessGroups.c_str(), buffer, bufferSize, iniFile.c_str());
    g_Settings.SimilarProcessGroups = buffer;

    g_Settings.PromptTerminateAll = GetPrivateProfileInt(L"General", L"PromptTerminateAll"    , DEFAULT_PROMPTTERMINATEALL  , iniFile.c_str());
    g_Settings.Transparency       = GetPrivateProfileInt(L"General", L"WindowTransparency"    , DEFAULT_TRANSPARENCY        , iniFile.c_str());
    g_Settings.WidthPercentage    = GetPrivateProfileInt(L"General", L"WindowWidthPercentage" , DEFAULT_WIDTH               , iniFile.c_str());
    g_Settings.HeightPercentage   = GetPrivateProfileInt(L"General", L"WindowHeightPercentage", DEFAULT_HEIGHT              , iniFile.c_str());
    g_Settings.ShowColHeader      = GetPrivateProfileInt(L"General", L"ShowColHeader"         , DEFAULT_SHOW_COL_HEADER     , iniFile.c_str());
    g_Settings.ShowColProcessName = GetPrivateProfileInt(L"General", L"ShowColProcessName"    , DEFAULT_SHOW_COL_PROCESSNAME, iniFile.c_str());

    GetPrivateProfileStringW(L"General", L"CheckForUpdates", L"Startup", buffer, bufferSize, iniFile.c_str());
    g_Settings.CheckForUpdates = buffer;

    // Clear the previous ProcessGroupsList
    g_Settings.ProcessGroupsList.clear();

    auto vs = Split(g_Settings.SimilarProcessGroups, L"|");
    for (auto& item : vs) {
        auto processes = Split(item, L"/");
        for (auto& processName : processes)
            processName = ToLower(processName);
        g_Settings.ProcessGroupsList.emplace_back(processes.begin(), processes.end());
    }

    // Initialize additional settings
    g_AltBacktickWndInfo.hWnd   = nullptr;
    g_AltBacktickWndInfo.hOwner = nullptr;
}

void ATSaveSettings() {
    AT_LOG_TRACE;
    std::wstring settingsFilePath = ATSettingsFilePath();
    WriteSetting(settingsFilePath.c_str(), L"Backtick", L"SimilarProcessGroups"  , g_Settings.SimilarProcessGroups);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"PromptTerminateAll"    , g_Settings.PromptTerminateAll);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"WindowTransparency"    , g_Settings.Transparency);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"WindowWidthPercentage" , g_Settings.WidthPercentage);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"WindowHeightPercentage", g_Settings.HeightPercentage);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"CheckForUpdates"       , g_Settings.CheckForUpdates);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"ShowColHeader"         , g_Settings.ShowColHeader);
    WriteSetting(settingsFilePath.c_str(), L"General" , L"ShowColProcessName"    , g_Settings.ShowColProcessName);
}

void ATApplySettings(HWND hDlg) {
    AT_LOG_TRACE;

    int textLength                  = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS));
    wchar_t* buffer                 = new wchar_t[textLength + 1];
    GetDlgItemTextW(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS, buffer, textLength + 1);

    g_Settings.SimilarProcessGroups = buffer;
    g_Settings.PromptTerminateAll   = IsDlgButtonChecked(hDlg, IDC_CHECK_PROMPT_TERMINATE_ALL) == BST_CHECKED;
    g_Settings.ShowColHeader        = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_COL_HEADER)      == BST_CHECKED;
    g_Settings.ShowColProcessName   = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_COL_PROCESSNAME) == BST_CHECKED;
    g_Settings.Transparency         = GetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_TRANSPARENCY     , nullptr, FALSE);
    g_Settings.WidthPercentage      = GetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_WIDTH_PERCENTAGE , nullptr, FALSE);
    g_Settings.HeightPercentage     = GetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_HEIGHT_PERCENTAGE, nullptr, FALSE);

    delete[] buffer;

    // Save settings
    ATSaveSettings();

    // Load settings to reconstruct the ProcessGroupsList
    ATLoadSettings();

    // Disable Apply button after saving settings
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY), false);
}

bool AreSettingsModified(HWND hDlg) {
    int textLength                  = GetWindowTextLength(GetDlgItem(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS));
    wchar_t* buffer                 = new wchar_t[textLength + 1];
    GetDlgItemTextW(hDlg, IDC_EDIT_SIMILAR_PROCESS_GROUPS, buffer, textLength + 1);

    AltTabSettings settings;
    settings.SimilarProcessGroups = buffer;
    settings.PromptTerminateAll   = IsDlgButtonChecked(hDlg, IDC_CHECK_PROMPT_TERMINATE_ALL) == BST_CHECKED;
    settings.ShowColHeader        = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_COL_HEADER)      == BST_CHECKED;
    settings.ShowColProcessName   = IsDlgButtonChecked(hDlg, IDC_CHECK_SHOW_COL_PROCESSNAME) == BST_CHECKED;
    settings.Transparency         = GetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_TRANSPARENCY     , nullptr, FALSE);
    settings.WidthPercentage      = GetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_WIDTH_PERCENTAGE , nullptr, FALSE);
    settings.HeightPercentage     = GetDlgItemInt     (hDlg, IDC_EDIT_WINDOW_HEIGHT_PERCENTAGE, nullptr, FALSE);

    delete[] buffer;

    bool modified =
        settings.Transparency         != g_Settings.Transparency         ||
        settings.WidthPercentage      != g_Settings.WidthPercentage      ||
        settings.HeightPercentage     != g_Settings.HeightPercentage     ||
        settings.SimilarProcessGroups != g_Settings.SimilarProcessGroups ||
        settings.ShowColHeader        != g_Settings.ShowColHeader        ||
        settings.ShowColProcessName   != g_Settings.ShowColProcessName   ||
        settings.PromptTerminateAll   != g_Settings.PromptTerminateAll;

    return modified;
}
